/**
 * SSI-FS: OPFS Backend — 浏览器 Origin Private File System
 *
 * 基于浏览器原生 OPFS API 的持久化文件系统后端。
 * 数据存储在浏览器的 origin 级隔离存储中，清除浏览器数据会丢失。
 *
 * 使用限制:
 *   - 仅在 HTTPS / localhost 环境下可用
 *   - 主线程只能使用异步 FileSystemFileHandle
 *   - Web Worker 中可使用 createSyncAccessHandle() 实现同步读写
 *
 * 设计策略:
 *   - init() 时获取根目录句柄并缓存目录/文件句柄树
 *   - read/write 通过句柄的 File API (getFile → arrayBuffer) 实现
 *   - seek/tell 维护一个内存位置指针（因为 OPFS 读写不维护内核位置）
 *
 * 参考:
 *   - https://developer.mozilla.org/en-US/docs/Web/API/File_System_API/Origin_private_file_system
 *   - https://fs.spec.whatwg.org/
 */

import { SsiErrorCode } from '../../core/src/index';
import { SsiFsBackend, SsiFileStat, SsiDirEntry } from './backends';
import { O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, O_APPEND, SEEK_SET, SEEK_CUR, SEEK_END } from './backends';

// =========================================================================
// 全局检测
// =========================================================================

/** 当前环境是否支持 OPFS */
export function opfsSupported(): boolean {
  return typeof navigator !== 'undefined'
    && 'storage' in navigator
    && typeof (navigator.storage as any).getDirectory === 'function';
}

/** 获取 OPFS 根目录句柄 */
async function getOpfsRoot(): Promise<FileSystemDirectoryHandle | null> {
  try {
    return await navigator.storage.getDirectory();
  } catch {
    return null;
  }
}

// =========================================================================
// 内部类型
// =========================================================================

/** 缓存的句柄树节点 */
interface OpfsHandleNode {
  handle: FileSystemDirectoryHandle | FileSystemFileHandle;
  isDirectory: boolean;
  /** 文件/目录名 */
  name: string;
  /** 子节点（仅目录） */
  children?: Map<string, OpfsHandleNode>;
  /** 最后刷新的时间戳 */
  refreshedAt: number;
}

/** 文件描述符 */
interface OpfsFileDesc {
  /** 文件句柄 */
  handle: FileSystemFileHandle;
  /** 打开时的路径 */
  path: string;
  /** 打开时的 flags */
  flags: number;
  /** 当前读取位置 */
  position: number;
  /** 追加模式 */
  appendMode: boolean;
  /** 缓存的文件大小（打开时获取） */
  size: number;
  /** 是否被修改过 */
  dirty: boolean;
}

// =========================================================================
// OPFS Backend
// =========================================================================

export class OpfsBackend implements SsiFsBackend {
  readonly name = 'opfs';

  /** OPFS 根目录句柄 */
  private rootHandle: FileSystemDirectoryHandle | null = null;

  /** 句柄缓存：路径 → 句柄节点 */
  private handleCache = new Map<string, OpfsHandleNode>();

  /** 文件描述符表 */
  private files = new Map<number, OpfsFileDesc>();
  private nextFd = 3;

  /** 缓存 TTL（毫秒），默认 5 秒后重新查询 */
  private cacheTtl = 5000;

  /** 初始化是否已完成 */
  private initialized = false;

  // =========================================================================
  // Lifecycle
  // =========================================================================

  init(config: Record<string, unknown>): SsiErrorCode {
    // 这里不能 await，因为 SsiFsBackend 接口是同步 init
    // 实际初始化在下一次操作时 lazy 完成
    if (typeof config.cacheTtl === 'number') {
      this.cacheTtl = config.cacheTtl;
    }

    // 同步检测环境
    if (!opfsSupported()) {
      return SsiErrorCode.NOT_SUPPORTED;
    }

    // 启动异步初始化（不阻塞 init 返回）
    this.lazyInit();

    this.initialized = true;
    return SsiErrorCode.OK;
  }

  destroy(): void {
    this.handleCache.clear();
    this.files.clear();
    this.rootHandle = null;
    this.initialized = false;
  }

  /**
   * 异步初始化：获取根句柄并缓存
   */
  private async lazyInit(): Promise<void> {
    if (this.rootHandle) return;
    this.rootHandle = await getOpfsRoot();
    if (this.rootHandle) {
      this.cacheHandle('/', {
        handle: this.rootHandle,
        isDirectory: true,
        name: '',
        children: new Map(),
        refreshedAt: Date.now(),
      });
    }
  }

  /**
   * 确保根句柄可用
   */
  private ensureReady(): boolean {
    return this.initialized && this.rootHandle !== null;
  }

  // =========================================================================
  // Internal Helpers (async → sync bridge via eager caching)
  // =========================================================================

  /**
   * 将路径按 '/' 分割
   */
  private splitPath(p: string): string[] {
    return p.split('/').filter(s => s.length > 0);
  }

  /**
   * 获取父目录路径
   */
  private parentPath(p: string): string {
    const parts = this.splitPath(p);
    parts.pop();
    return '/' + parts.join('/');
  }

  /**
   * 获取文件名（basename）
   */
  private baseName(p: string): string {
    const parts = this.splitPath(p);
    return parts[parts.length - 1] || '';
  }

  /**
   * 缓存句柄
   */
  private cacheHandle(path: string, node: OpfsHandleNode): void {
    this.handleCache.set(path, node);
  }

  /**
   * 从缓存中获取句柄
   */
  private getCachedHandle(path: string): OpfsHandleNode | undefined {
    return this.handleCache.get(path);
  }

  /**
   * 从缓存中清除路径及所有子路径
   */
  private invalidateCache(path: string): void {
    const prefix = path === '/' ? '/' : path + '/';
    for (const [key] of this.handleCache) {
      if (key === path || key.startsWith(prefix)) {
        this.handleCache.delete(key);
      }
    }
  }

  // =========================================================================
  // 异步句柄解析（同步接口的异步桥接）
  //
  // 由于 SsiFsBackend 是同步接口而 OPFS 是异步 API，
  // 我们采用"预加载 + 缓存"策略：
  //   1. open / stat / readdir 等操作先查找缓存
  //   2. 缓存未命中时，抛出 NOT_FOUND 或返回缓存过期的标记
  //   3. 实际文件内容通过 createSyncAccessHandle 读取
  // =========================================================================

  /**
   * 尝试解析路径到句柄（从缓存）
   */
  private resolveHandle(path: string): OpfsHandleNode | null {
    return this.getCachedHandle(path) || null;
  }

  /**
   * 确保目录被缓存（递归展开子目录）
   * 返回 false 表示操作不可用（需要异步刷新）
   */
  private ensureDirCached(path: string): boolean {
    const cached = this.getCachedHandle(path);
    return cached !== undefined && cached.isDirectory;
  }

  // =========================================================================
  // File Operations (同步接口，基于缓存 + 同步访问句柄)
  // =========================================================================

  open(filePath: string, flags: number): SsiErrorCode {
    if (!this.ensureReady()) return SsiErrorCode.NOT_SUPPORTED;

    const normalized = filePath.replace(/\/+/g, '/').replace(/\/$/, '') || '/';
    const parent = this.parentPath(normalized);
    const name = this.baseName(normalized);
    const existing = this.getCachedHandle(normalized);

    // 文件已存在
    if (existing) {
      if (existing.isDirectory) return SsiErrorCode.INVALID;

      const handle = existing.handle as FileSystemFileHandle;

      // 同步获取文件信息
      try {
        // 这里我们使用缓存的大小
        // 在 OPFS 中无法在同步上下文中获取最新大小
        const desc: OpfsFileDesc = {
          handle,
          path: normalized,
          flags,
          position: (flags & O_APPEND) ? 0 : 0, // 需要先获取文件大小
          appendMode: !!(flags & O_APPEND),
          size: 0,
          dirty: false,
        };

        if (flags & O_TRUNC) {
          // 无法同步截断 OPFS 文件 — 标记为 dirty，写入时覆盖
          desc.dirty = true;
        }

        const fd = this.nextFd++;
        this.files.set(fd, desc);
        return SsiErrorCode.OK;
      } catch {
        return SsiErrorCode.GENERIC;
      }
    }

    // 文件不存在，需要 O_CREAT
    if (!(flags & O_CREAT)) return SsiErrorCode.NOT_FOUND;

    // 需要父目录缓存
    if (!this.ensureDirCached(parent)) return SsiErrorCode.NOT_FOUND;

    // 在 OPFS 中创建文件需要异步操作
    // 由于接口是同步的，我们返回一个特殊状态
    // 用户需要调用 stat 确认文件已创建
    return SsiErrorCode.NOT_SUPPORTED;
  }

  close(fd: number): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;

    this.files.delete(fd);
    return SsiErrorCode.OK;
  }

  read(fd: number, size: number): { data: ArrayBuffer | null; error: SsiErrorCode } {
    const desc = this.files.get(fd);
    if (!desc) return { data: null, error: SsiErrorCode.NOT_FOUND };
    if ((desc.flags & 3) === O_WRONLY) return { data: null, error: SsiErrorCode.PERMISSION };

    // OPFS 不支持同步读取 — 需要 createSyncAccessHandle (仅 Worker)
    // 主线程中无法实现，返回 NOT_SUPPORTED
    return { data: null, error: SsiErrorCode.NOT_SUPPORTED };
  }

  write(fd: number, data: ArrayBuffer): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;
    if ((desc.flags & 3) === O_RDONLY) return SsiErrorCode.PERMISSION;

    // 同样需要异步 API
    return SsiErrorCode.NOT_SUPPORTED;
  }

  seek(fd: number, offset: number, whence: number): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;

    let newPos: number;
    switch (whence) {
      case SEEK_SET: newPos = offset; break;
      case SEEK_CUR: newPos = desc.position + offset; break;
      case SEEK_END: newPos = desc.size + offset; break;
      default: return SsiErrorCode.INVALID;
    }

    if (newPos < 0) return SsiErrorCode.INVALID;
    desc.position = newPos;
    return SsiErrorCode.OK;
  }

  tell(fd: number): { position: number; error: SsiErrorCode } {
    const desc = this.files.get(fd);
    if (!desc) return { position: 0, error: SsiErrorCode.NOT_FOUND };
    return { position: desc.position, error: SsiErrorCode.OK };
  }

  // =========================================================================
  // Directory & Metadata (尽可能从缓存返回)
  // =========================================================================

  stat(filePath: string): { stat: SsiFileStat | null; error: SsiErrorCode } {
    if (!this.ensureReady()) {
      return { stat: null, error: SsiErrorCode.NOT_SUPPORTED };
    }

    const normalized = filePath.replace(/\/+/g, '/').replace(/\/$/, '') || '/';
    const cached = this.getCachedHandle(normalized);

    if (!cached) {
      return { stat: null, error: SsiErrorCode.NOT_FOUND };
    }

    return {
      stat: {
        size: 0, // OPFS 无法在同步上下文中获取文件大小
        mode: cached.isDirectory ? 0o755 : 0o644,
        isDirectory: cached.isDirectory,
        isFile: !cached.isDirectory,
        createdAt: 0,
        modifiedAt: cached.refreshedAt,
      },
      error: SsiErrorCode.OK,
    };
  }

  mkdir(filePath: string, mode: number): SsiErrorCode {
    if (!this.ensureReady()) return SsiErrorCode.NOT_SUPPORTED;

    const normalized = filePath.replace(/\/+/g, '/').replace(/\/$/, '');
    if (this.getCachedHandle(normalized)) return SsiErrorCode.BUSY;

    // OPFS 创建目录需要异步 API
    // 同步接口下只能返回 NOT_SUPPORTED
    return SsiErrorCode.NOT_SUPPORTED;
  }

  readdir(filePath: string): { entries: SsiDirEntry[]; error: SsiErrorCode } {
    if (!this.ensureReady()) {
      return { entries: [], error: SsiErrorCode.NOT_SUPPORTED };
    }

    const normalized = filePath.replace(/\/+/g, '/').replace(/\/$/, '') || '/';
    const cached = this.getCachedHandle(normalized);

    if (!cached) return { entries: [], error: SsiErrorCode.NOT_FOUND };
    if (!cached.isDirectory) return { entries: [], error: SsiErrorCode.INVALID };

    const entries: SsiDirEntry[] = [];
    if (cached.children) {
      for (const [name, node] of cached.children) {
        entries.push({
          name,
          type: node.isDirectory ? 1 : 0,
          size: 0,
        });
      }
    }

    return { entries, error: SsiErrorCode.OK };
  }

  unlink(filePath: string): SsiErrorCode {
    if (!this.ensureReady()) return SsiErrorCode.NOT_SUPPORTED;
    // 需要异步 API
    return SsiErrorCode.NOT_SUPPORTED;
  }

  rename(oldPath: string, newPath: string): SsiErrorCode {
    if (!this.ensureReady()) return SsiErrorCode.NOT_SUPPORTED;
    // 需要异步 API
    return SsiErrorCode.NOT_SUPPORTED;
  }
}

// =========================================================================
// 异步包装器 — OPFS 操作的真正实现
//
// 适用于可以 await 的上下文（如 SSI 组件层、async handlers）
// =========================================================================

export class OpfsAsyncWrapper {
  private rootHandle: FileSystemDirectoryHandle | null = null;
  private cache = new Map<string, FileSystemHandle>();

  async init(): Promise<SsiErrorCode> {
    if (!opfsSupported()) return SsiErrorCode.NOT_SUPPORTED;
    this.rootHandle = await getOpfsRoot();
    return this.rootHandle ? SsiErrorCode.OK : SsiErrorCode.NOT_FOUND;
  }

  async createFile(path: string): Promise<SsiErrorCode> {
    if (!this.rootHandle) return SsiErrorCode.NOT_SUPPORTED;
    try {
      const parts = path.split('/').filter(Boolean);
      let dir = this.rootHandle;

      for (let i = 0; i < parts.length - 1; i++) {
        dir = await dir.getDirectoryHandle(parts[i], { create: true });
      }

      const fileName = parts[parts.length - 1];
      if (!fileName) return SsiErrorCode.INVALID;

      await dir.getFileHandle(fileName, { create: true });
      return SsiErrorCode.OK;
    } catch {
      return SsiErrorCode.GENERIC;
    }
  }

  async createDirectory(path: string): Promise<SsiErrorCode> {
    if (!this.rootHandle) return SsiErrorCode.NOT_SUPPORTED;
    try {
      const parts = path.split('/').filter(Boolean);
      let dir = this.rootHandle;

      for (const part of parts) {
        dir = await dir.getDirectoryHandle(part, { create: true });
      }

      return SsiErrorCode.OK;
    } catch {
      return SsiErrorCode.GENERIC;
    }
  }

  async readFile(path: string): Promise<{ data: ArrayBuffer | null; error: SsiErrorCode }> {
    if (!this.rootHandle) return { data: null, error: SsiErrorCode.NOT_SUPPORTED };

    try {
      const parts = path.split('/').filter(Boolean);
      let dir = this.rootHandle;

      for (let i = 0; i < parts.length - 1; i++) {
        dir = await dir.getDirectoryHandle(parts[i], { create: false });
      }

      const fileName = parts[parts.length - 1];
      if (!fileName) return { data: null, error: SsiErrorCode.INVALID };

      const fileHandle = await dir.getFileHandle(fileName);
      const file = await fileHandle.getFile();
      const data = await file.arrayBuffer();

      return { data, error: SsiErrorCode.OK };
    } catch (err: unknown) {
      const e = err as DOMException;
      if (e.name === 'NotFoundError') return { data: null, error: SsiErrorCode.NOT_FOUND };
      return { data: null, error: SsiErrorCode.GENERIC };
    }
  }

  async writeFile(path: string, data: ArrayBuffer): Promise<SsiErrorCode> {
    if (!this.rootHandle) return SsiErrorCode.NOT_SUPPORTED;

    try {
      const parts = path.split('/').filter(Boolean);
      let dir = this.rootHandle;

      for (let i = 0; i < parts.length - 1; i++) {
        dir = await dir.getDirectoryHandle(parts[i], { create: true });
      }

      const fileName = parts[parts.length - 1];
      if (!fileName) return SsiErrorCode.INVALID;

      const fileHandle = await dir.getFileHandle(fileName, { create: true });
      const writable = await fileHandle.createWritable();
      await writable.write(data);
      await writable.close();

      return SsiErrorCode.OK;
    } catch {
      return SsiErrorCode.GENERIC;
    }
  }

  async deleteFile(path: string): Promise<SsiErrorCode> {
    if (!this.rootHandle) return SsiErrorCode.NOT_SUPPORTED;

    try {
      const parts = path.split('/').filter(Boolean);
      let dir = this.rootHandle;

      for (let i = 0; i < parts.length - 1; i++) {
        dir = await dir.getDirectoryHandle(parts[i], { create: false });
      }

      const fileName = parts[parts.length - 1];
      if (!fileName) return SsiErrorCode.INVALID;

      await dir.removeEntry(fileName);
      return SsiErrorCode.OK;
    } catch {
      return SsiErrorCode.GENERIC;
    }
  }

  async listDirectory(path: string): Promise<{ entries: SsiDirEntry[]; error: SsiErrorCode }> {
    if (!this.rootHandle) return { entries: [], error: SsiErrorCode.NOT_SUPPORTED };

    try {
      const parts = path.split('/').filter(Boolean);
      let dir = this.rootHandle;

      for (const part of parts) {
        dir = await dir.getDirectoryHandle(part, { create: false });
      }

      const entries: SsiDirEntry[] = [];
      for await (const [name, handle] of dir.entries()) {
        entries.push({
          name,
          type: handle.kind === 'directory' ? 1 : 0,
          size: 0,
        });
      }

      return { entries, error: SsiErrorCode.OK };
    } catch {
      return { entries: [], error: SsiErrorCode.GENERIC };
    }
  }

  destroy(): void {
    this.rootHandle = null;
    this.cache.clear();
  }
}