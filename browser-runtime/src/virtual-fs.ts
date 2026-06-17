/**
 * Virtual File System for Browser Environments
 *
 * Provides a POSIX-like filesystem abstraction that works across all
 * JavaScript environments (browser, Node.js, Web Workers).
 *
 * Backend options:
 * - MEMFS: In-memory filesystem (fast, non-persistent)
 * - OPFS: Origin Private File System (persistent, browser only)
 * - IDBFS: IndexedDB-backed filesystem (persistent, browser only)
 * - NODEFS: Node.js native filesystem (Node.js only)
 */

import { Logger, detectEnvironment, VirtualFileSystem, FileStat, FsBackend, DiskSpaceInfo, formatBytes } from './types';

export type { VirtualFileSystem, FileStat, FsBackend, DiskSpaceInfo };

/**
 * Virtual File System implementation
 */
export class VirtualFS implements VirtualFileSystem {
  private static instance: VirtualFS;
  private log: Logger;

  /** Emscripten FS instance (set after WASM module init) */
  private FS: any = null;

  /** Backend registry */
  private backends: Map<string, any> = new Map();

  /** Mount points */
  private mounts: Map<string, { backend: string; options: any }> = new Map();

  /** OPFS root directory handle (browser only) */
  private opfsRoot: FileSystemDirectoryHandle | null = null;

  /** IndexedDB database for IDBFS */
  private idbDatabase: IDBDatabase | null = null;

  private constructor() {
    this.log = new Logger('info');
  }

  static getInstance(): VirtualFS {
    if (!VirtualFS.instance) {
      VirtualFS.instance = new VirtualFS();
    }
    return VirtualFS.instance;
  }

  /** Connect to an Emscripten FS instance */
  setFS(FS: any): void {
    this.FS = FS;
    this.log.info('VirtualFS connected to Emscripten FS');
  }

  /** Check if Emscripten FS is connected */
  get isConnected(): boolean {
    return this.FS !== null;
  }

  // =========================================================================
  // Core file operations
  // =========================================================================

  async writeFile(path: string, data: Uint8Array | string): Promise<void> {
    const env = detectEnvironment();
    const backend = this.getBackendForPath(path);

    if (backend === 'nodefs' && env === 'node') {
      await this.nodeWriteFile(path, data);
    } else if (backend === 'opfs' && env === 'browser') {
      await this.opfsWriteFile(path, data);
    } else if (this.FS) {
      // Use Emscripten FS
      this.ensureDir(path);
      if (typeof data === 'string') {
        this.FS.writeFile(path, data);
      } else {
        this.FS.writeFile(path, data);
      }
    } else {
      throw new Error(`No filesystem available for ${path}`);
    }
  }

  async readFile(path: string, encoding?: 'utf8' | 'binary'): Promise<Uint8Array | string> {
    const env = detectEnvironment();
    const backend = this.getBackendForPath(path);

    if (backend === 'nodefs' && env === 'node') {
      return this.nodeReadFile(path, encoding);
    } else if (backend === 'opfs' && env === 'browser') {
      return this.opfsReadFile(path, encoding);
    } else if (this.FS) {
      if (encoding === 'utf8') {
        return this.FS.readFile(path, { encoding: 'utf8' });
      }
      return this.FS.readFile(path);
    } else {
      throw new Error(`No filesystem available for ${path}`);
    }
  }

  async exists(path: string): Promise<boolean> {
    if (this.FS) {
      try {
        this.FS.analyzePath(path);
        return true;
      } catch {
        return false;
      }
    }
    return false;
  }

  async mkdir(path: string): Promise<void> {
    if (this.FS) {
      try {
        this.FS.mkdirTree(path);
        this.log.debug(`Created directory: ${path}`);
      } catch (e: any) {
        if (e.errno !== 20) { // EEXIST
          throw e;
        }
      }
    }
  }

  async readdir(path: string): Promise<string[]> {
    if (this.FS) {
      try {
        return this.FS.readdir(path).filter((n: string) => n !== '.' && n !== '..');
      } catch (e) {
        this.log.warn(`readdir failed: ${path}`, e);
        return [];
      }
    }
    return [];
  }

  async unlink(path: string): Promise<void> {
    if (this.FS) {
      try {
        const stat = this.FS.stat(path);
        if (this.FS.isDir(stat.mode)) {
          this.FS.rmdir(path);
        } else {
          this.FS.unlink(path);
        }
      } catch (e: any) {
        if (e.errno !== 44) { // ENOENT
          throw e;
        }
      }
    }
  }

  async stat(path: string): Promise<FileStat> {
    if (this.FS) {
      const s = this.FS.stat(path);
      return {
        size: s.size,
        isDirectory: this.FS.isDir(s.mode),
        isFile: this.FS.isFile(s.mode),
        mode: s.mode,
        mtime: new Date(s.mtime * 1000),
        atime: new Date(s.atime * 1000),
        ctime: new Date(s.ctime * 1000),
      };
    }
    throw new Error(`FS not available`);
  }

  async mount(path: string, backend: FsBackend, options?: any): Promise<void> {
    if (this.FS) {
      // Create mount point
      if (!this.FS.analyzePath(path).exists) {
        this.FS.mkdirTree(path);
      }
      this.mounts.set(path, { backend, options: options || {} });
      this.log.info(`Mounted ${backend} at ${path}`);
    }
  }

  async unmount(path: string): Promise<void> {
    if (this.FS && this.mounts.has(path)) {
      this.mounts.delete(path);
      this.log.info(`Unmounted ${path}`);
    }
  }

  async df(): Promise<DiskSpaceInfo> {
    // In-memory FS: approximate via performance.memory if available
    const env = detectEnvironment();
    if (env === 'browser' && (performance as any).memory) {
      const mem = (performance as any).memory;
      return {
        total: mem.jsHeapSizeLimit,
        used: mem.usedJSHeapSize,
        free: mem.jsHeapSizeLimit - mem.usedJSHeapSize,
      };
    }
    return { total: 512 * 1024 * 1024, used: 0, free: 512 * 1024 * 1024 };
  }

  // =========================================================================
  // Backend-specific helpers
  // =========================================================================

  private getBackendForPath(path: string): string {
    for (const [mountPoint, config] of this.mounts) {
      if (path.startsWith(mountPoint)) {
        return config.backend;
      }
    }
    return 'memfs'; // default
  }

  private ensureDir(path: string): void {
    if (!this.FS) return;
    const parts = path.split('/').filter(Boolean);
    let current = '';
    for (const part of parts.slice(0, -1)) {
      current += '/' + part;
      try {
        if (!this.FS.analyzePath(current).exists) {
          this.FS.mkdir(current);
        }
      } catch {
        // directory already exists
      }
    }
  }

  private async nodeWriteFile(path: string, data: Uint8Array | string): Promise<void> {
    const fs = await import('fs');
    const dir = path.substring(0, path.lastIndexOf('/'));
    if (dir) {
      fs.mkdirSync(dir, { recursive: true });
    }
    if (typeof data === 'string') {
      fs.writeFileSync(path, data, 'utf-8');
    } else {
      fs.writeFileSync(path, Buffer.from(data));
    }
  }

  private async nodeReadFile(path: string, encoding?: 'utf8' | 'binary'): Promise<Uint8Array | string> {
    const fs = await import('fs');
    if (encoding === 'utf8') {
      return fs.readFileSync(path, 'utf-8');
    }
    return new Uint8Array(fs.readFileSync(path));
  }

  private async getOpfsRoot(): Promise<FileSystemDirectoryHandle> {
    if (!this.opfsRoot) {
      if (!navigator.storage?.getDirectory) {
        throw new Error('OPFS not supported in this browser');
      }
      this.opfsRoot = await navigator.storage.getDirectory();
    }
    return this.opfsRoot;
  }

  private async opfsEnsureDir(dirPath: string): Promise<FileSystemDirectoryHandle> {
    const root = await this.getOpfsRoot();
    if (!dirPath || dirPath === '/') return root;

    const parts = dirPath.split('/').filter(Boolean);
    let current = root;
    for (const part of parts) {
      current = await current.getDirectoryHandle(part, { create: true });
    }
    return current;
  }

  private async opfsWriteFile(path: string, data: Uint8Array | string): Promise<void> {
    const root = await this.getOpfsRoot();
    const parts = path.split('/').filter(Boolean);
    const fileName = parts.pop()!;
    const dirPath = parts.join('/');

    const dir = dirPath ? await this.opfsEnsureDir(dirPath) : root;
    const fileHandle = await dir.getFileHandle(fileName, { create: true });
    const writable = await fileHandle.createWritable();

    if (typeof data === 'string') {
      await writable.write(new TextEncoder().encode(data));
    } else {
      await writable.write(data);
    }
    await writable.close();
  }

  private async opfsReadFile(path: string, encoding?: 'utf8' | 'binary'): Promise<Uint8Array | string> {
    const root = await this.getOpfsRoot();
    const parts = path.split('/').filter(Boolean);
    const fileName = parts.pop()!;
    const dirPath = parts.join('/');

    const dir = dirPath ? await this.opfsEnsureDir(dirPath) : root;
    const fileHandle = await dir.getFileHandle(fileName);
    const file = await fileHandle.getFile();
    const buffer = await file.arrayBuffer();

    if (encoding === 'utf8') {
      return new TextDecoder().decode(buffer);
    }
    return new Uint8Array(buffer);
  }

  /** Create a temporary directory */
  async createTempDir(prefix: string = 'tmp'): Promise<string> {
    const tmpPath = `/tmp/${prefix}_${Date.now()}`;
    await this.mkdir(tmpPath);
    return tmpPath;
  }
}

export default VirtualFS;
