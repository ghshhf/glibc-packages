/**
 * SSI-FS: NODEFS Backend — Node.js 原生文件系统桥接
 *
 * 封装 Node.js `fs` / `fs/promises` 模块，实现 SsiFsBackend 接口。
 * 将 SSI 虚拟文件系统操作翻译为宿主机文件系统调用。
 *
 * 使用场景:
 *   - 桌面端 (Electron) — 本地文件读写
 *   - 服务端 (Node.js) — 容器挂载卷访问
 *   - Android Termux — 原生文件系统
 *
 * flags 映射:
 *   O_RDONLY(0)  → 'r'
 *   O_WRONLY(1)  → 'w' / 'a' (O_APPEND)
 *   O_RDWR(2)    → 'r+' / 'w+' / 'a+'
 *   O_CREAT(64)  → 自动创建
 *   O_TRUNC(512) → 截断
 *   O_APPEND(1024) → 追加模式
 */

import * as fs from 'node:fs';
import * as fsp from 'node:fs/promises';
import * as path from 'node:path';

import { SsiErrorCode } from '../../core/src/index';
import { SsiFsBackend, SsiFileStat, SsiDirEntry } from './backends';
import { O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, O_APPEND, SEEK_SET, SEEK_CUR, SEEK_END } from './backends';

// =========================================================================
// 内部类型
// =========================================================================

/** 文件描述符记录：SSI 虚似 fd → Node.js 真实 fd */
interface NodeFileDesc {
  /** Node.js 真实文件描述符数值 */
  realFd: number;
  /** 打开时的路径（用于 stat/tell 回退） */
  path: string;
  /** 打开时的 flags */
  flags: number;
  /** 当前读取位置（写模式下不维护，用 fs.stat 推断） */
  position: number;
  /** 追加模式标记 */
  appendMode: boolean;
}

/**
 * Node.js flag 字符串转换表
 *
 * POSIX       Node.js    说明
 * O_RDONLY    'r'        只读，不存在则报错
 * O_RDWR      'r+'       读写，不存在则报错
 * O_WRONLY|O_CREAT|O_TRUNC    'w'     写，创建并截断
 * O_WRONLY|O_CREAT|O_APPEND   'a'     写，创建，追加
 * O_RDWR|O_CREAT|O_TRUNC      'w+'    读写，创建并截断
 * O_RDWR|O_CREAT|O_APPEND     'a+'    读写，创建，追加
 * O_RDWR|O_CREAT              'r+'    (无 trunc/append, 相当于 r+)
 * O_WRONLY|O_CREAT            'wx'     ? 这种情况少见
 */
function flagsToNodeFlag(flags: number): string {
  const readonly = (flags & 3) === O_RDONLY;
  const writable = (flags & 3) === O_WRONLY;
  const readwrite = (flags & 3) === O_RDWR;
  const creat = !!(flags & O_CREAT);
  const trunc = !!(flags & O_TRUNC);
  const append = !!(flags & O_APPEND);

  if (readonly) return 'r';
  if (readwrite) {
    if (creat) {
      if (trunc) return 'w+';
      if (append) return 'a+';
      return 'r+';
    }
    return 'r+';
  }
  // O_WRONLY
  if (creat) {
    if (trunc) return 'w';
    if (append) return 'a';
    return 'wx';
  }
  return 'r'; // fallback
}

// =========================================================================
// NODEFS Backend
// =========================================================================

export class NodeFsBackend implements SsiFsBackend {
  readonly name = 'nodefs';

  /** 挂载根目录：所有路径都将解析到此目录下 */
  private root: string = '/';

  /** SSI fd → NodeFileDesc 映射 */
  private files = new Map<number, NodeFileDesc>();
  private nextFd = 3;

  /** 当前工作目录（用于相对路径解析） */
  private cwd: string = '/';

  constructor(root?: string) {
    if (root) this.root = path.resolve(root);
  }

  // =========================================================================
  // Lifecycle
  // =========================================================================

  init(config: Record<string, unknown>): SsiErrorCode {
    if (typeof config.root === 'string') {
      this.root = path.resolve(config.root);
    }
    if (typeof config.cwd === 'string') {
      this.cwd = config.cwd;
    }
        // 确保根目录存在
    try {
      const rootPath = this.resolvePath('/');
      if (rootPath === null) return SsiErrorCode.PERMISSION;
      if (!fs.existsSync(rootPath)) {
        fs.mkdirSync(rootPath, { recursive: true });
      }
    } catch {
      return SsiErrorCode.PERMISSION;
    }
    return SsiErrorCode.OK;
  }

  destroy(): void {
    // 关闭所有打开的文件
    for (const [fd, desc] of this.files) {
      try { fs.closeSync(desc.realFd); } catch { /* ignore */ }
    }
    this.files.clear();
  }

  // =========================================================================
  // File Operations
  // =========================================================================

  open(filePath: string, flags: number): SsiErrorCode {
    const absPath = this.resolvePath(filePath);
    if (absPath === null) return SsiErrorCode.NOT_FOUND;

    try {
      // 构建 Node.js  flags
      let nodeFlags = 0;
      const accessMode = flags & 3;
      if (accessMode === O_RDONLY) nodeFlags |= fs.constants.O_RDONLY;
      if (accessMode === O_WRONLY) nodeFlags |= fs.constants.O_WRONLY;
      if (accessMode === O_RDWR) nodeFlags |= fs.constants.O_RDWR;
      if (flags & O_CREAT) nodeFlags |= fs.constants.O_CREAT;
      if (flags & O_TRUNC) nodeFlags |= fs.constants.O_TRUNC;
      if (flags & O_APPEND) nodeFlags |= fs.constants.O_APPEND;

      const realFd = fs.openSync(absPath, nodeFlags, 0o644);
      const stat = fs.fstatSync(realFd);
      const fd = this.nextFd++;

      this.files.set(fd, {
        realFd,
        path: absPath,
        flags,
        position: (flags & O_APPEND) ? stat.size : 0,
        appendMode: !!(flags & O_APPEND),
      });

      return SsiErrorCode.OK;
    } catch (err: unknown) {
      const nodeErr = err as NodeJS.ErrnoException;
      if (nodeErr.code === 'ENOENT') return SsiErrorCode.NOT_FOUND;
      if (nodeErr.code === 'EACCES' || nodeErr.code === 'EPERM') return SsiErrorCode.PERMISSION;
      if (nodeErr.code === 'EEXIST') return SsiErrorCode.BUSY;
      if (nodeErr.code === 'EISDIR') return SsiErrorCode.INVALID;
      return SsiErrorCode.GENERIC;
    }
  }

  close(fd: number): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;

    try {
      fs.closeSync(desc.realFd);
    } catch { /* ignore cleanup errors */ }
    this.files.delete(fd);
    return SsiErrorCode.OK;
  }

  read(fd: number, size: number): { data: ArrayBuffer | null; error: SsiErrorCode } {
    const desc = this.files.get(fd);
    if (!desc) return { data: null, error: SsiErrorCode.NOT_FOUND };
    if ((desc.flags & 3) === O_WRONLY) return { data: null, error: SsiErrorCode.PERMISSION };

    try {
      // 使用 read  + position 避免 lseek + read 两次系统调用
      const buf = Buffer.alloc(size);
      const bytesRead = fs.readSync(desc.realFd, buf, 0, size, desc.position);
      desc.position += bytesRead;

      if (bytesRead === 0) {
        return { data: new ArrayBuffer(0), error: SsiErrorCode.OK };
      }

      return { data: buf.subarray(0, bytesRead).buffer as ArrayBuffer, error: SsiErrorCode.OK };
    } catch {
      return { data: null, error: SsiErrorCode.GENERIC };
    }
  }

  write(fd: number, data: ArrayBuffer): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;
    if ((desc.flags & 3) === O_RDONLY) return SsiErrorCode.PERMISSION;

    try {
      const buf = Buffer.from(data);
      let offset = 0;
      let remaining = buf.length;

      while (remaining > 0) {
        const written = fs.writeSync(desc.realFd, buf, offset, remaining, desc.appendMode ? null : desc.position);
        offset += written;
        remaining -= written;
        if (!desc.appendMode) desc.position += written;
      }

      return SsiErrorCode.OK;
    } catch {
      return SsiErrorCode.GENERIC;
    }
  }

  seek(fd: number, offset: number, whence: number): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;

    try {
      let newPos: number;
      switch (whence) {
        case SEEK_SET:
          newPos = offset;
          break;
        case SEEK_CUR:
          newPos = desc.position + offset;
          break;
        case SEEK_END: {
          const stat = fs.fstatSync(desc.realFd);
          newPos = stat.size + offset;
          break;
        }
        default:
          return SsiErrorCode.INVALID;
      }

      if (newPos < 0) return SsiErrorCode.INVALID;

      // 使用 lseek 同步内核态位置
      fs.fstatSync(desc.realFd); // 确保 fd 有效
      // 不真正 seek，只更新我们维护的位置（read/write 都使用 position 参数）
      desc.position = newPos;
      return SsiErrorCode.OK;
    } catch {
      return SsiErrorCode.GENERIC;
    }
  }

  tell(fd: number): { position: number; error: SsiErrorCode } {
    const desc = this.files.get(fd);
    if (!desc) return { position: 0, error: SsiErrorCode.NOT_FOUND };
    return { position: desc.position, error: SsiErrorCode.OK };
  }

  // =========================================================================
  // Directory & Metadata
  // =========================================================================

  stat(filePath: string): { stat: SsiFileStat | null; error: SsiErrorCode } {
    const absPath = this.resolvePath(filePath);
    if (absPath === null) return { stat: null, error: SsiErrorCode.NOT_FOUND };


    try {
      const st = fs.statSync(absPath);
      return {
        stat: {
          size: st.size,
          mode: st.mode,
          isDirectory: st.isDirectory(),
          isFile: st.isFile(),
          createdAt: Math.floor(st.birthtimeMs),
          modifiedAt: Math.floor(st.mtimeMs),
        },
        error: SsiErrorCode.OK,
      };
    } catch (err: unknown) {
      const nodeErr = err as NodeJS.ErrnoException;
      if (nodeErr.code === 'ENOENT') return { stat: null, error: SsiErrorCode.NOT_FOUND };
      return { stat: null, error: SsiErrorCode.GENERIC };
    }
  }

  mkdir(filePath: string, mode: number): SsiErrorCode {
    const absPath = this.resolvePath(filePath);
    if (absPath === null) return SsiErrorCode.NOT_FOUND;


    try {
      fs.mkdirSync(absPath, { recursive: true, mode });
      return SsiErrorCode.OK;
    } catch (err: unknown) {
      const nodeErr = err as NodeJS.ErrnoException;
      if (nodeErr.code === 'EEXIST') return SsiErrorCode.BUSY;
      if (nodeErr.code === 'EACCES') return SsiErrorCode.PERMISSION;
      if (nodeErr.code === 'ENOENT') return SsiErrorCode.NOT_FOUND;
      return SsiErrorCode.GENERIC;
    }
  }

  readdir(filePath: string): { entries: SsiDirEntry[] | null; error: SsiErrorCode } {
    const absPath = this.resolvePath(filePath);
    if (absPath === null) return { entries: null, error: SsiErrorCode.NOT_FOUND };

    try {
      const names = fs.readdirSync(absPath);
      const entries: SsiDirEntry[] = [];

      for (const name of names) {
        try {
          const st = fs.statSync(path.join(absPath, name));
          entries.push({
            name,
            type: st.isDirectory() ? 1 : 0,
            size: st.size,
          });
        } catch {
          // 跳过无法 stat 的条目（权限/竞争）
          entries.push({ name, type: 0, size: 0 });
        }
      }

      return { entries, error: SsiErrorCode.OK };
    } catch (err: unknown) {
      const nodeErr = err as NodeJS.ErrnoException;
      if (nodeErr.code === 'ENOENT') return { entries: [], error: SsiErrorCode.NOT_FOUND };
      if (nodeErr.code === 'EACCES') return { entries: [], error: SsiErrorCode.PERMISSION };
      return { entries: [], error: SsiErrorCode.GENERIC };
    }
  }

  unlink(filePath: string): SsiErrorCode {
    const absPath = this.resolvePath(filePath);
    if (absPath === null) return SsiErrorCode.NOT_FOUND;


    try {
      const st = fs.statSync(absPath);
      if (st.isDirectory()) {
        fs.rmdirSync(absPath);
      } else {
        fs.unlinkSync(absPath);
      }
      return SsiErrorCode.OK;
    } catch (err: unknown) {
      const nodeErr = err as NodeJS.ErrnoException;
      if (nodeErr.code === 'ENOENT') return SsiErrorCode.NOT_FOUND;
      if (nodeErr.code === 'EACCES') return SsiErrorCode.PERMISSION;
      if (nodeErr.code === 'ENOTEMPTY') return SsiErrorCode.BUSY;
      return SsiErrorCode.GENERIC;
    }
  }

  rename(oldPath: string, newPath: string): SsiErrorCode {
    const absOld = this.resolvePath(oldPath);
    if (absOld === null) return SsiErrorCode.NOT_FOUND;
    const absNew = this.resolvePath(newPath);
    if (absNew === null) return SsiErrorCode.NOT_FOUND;

    try {
      fs.renameSync(absOld, absNew);
      return SsiErrorCode.OK;
    } catch (err: unknown) {
      const nodeErr = err as NodeJS.ErrnoException;
      if (nodeErr.code === 'ENOENT') return SsiErrorCode.NOT_FOUND;
      if (nodeErr.code === 'EACCES') return SsiErrorCode.PERMISSION;
      if (nodeErr.code === 'EEXIST') return SsiErrorCode.BUSY;
      if (nodeErr.code === 'EXDEV') return SsiErrorCode.NOT_SUPPORTED; // 跨设备
      return SsiErrorCode.GENERIC;
    }
  }

  // =========================================================================
  // Internal Helpers
  // =========================================================================

  /**
   * 将 SSI 虚拟路径解析为宿主机绝对路径。
   *
   * 安全约束:
   *   - 所有路径在 root 下
   *   - 禁止 path traversal (../ 逃逸)
   */
  private resolvePath(ssiPath: string): string | null {
    // 拼接 root 和路径，然后规范化
    const joined = path.join(this.root, ssiPath);
    const resolved = path.resolve(joined);

    // 安全校验：确保解析后的路径在 root 下
    if (!resolved.startsWith(path.resolve(this.root))) {
      return null;
    }

    return resolved;
  }
}