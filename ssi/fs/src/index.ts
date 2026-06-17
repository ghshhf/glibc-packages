/**
 * SSI-FS: Virtual File System Interface
 *
 * Implements the IFileSystem interface from SPEC-INTERFACE.md §9.
 * Provides file operations with multi-backend support.
 *
 * SSI Interfaces implemented:
 *   - SSI-FS: Virtual file system with MEMFS, NODEFS, OPFS, IDBFS backends
 */

import {
  SsiBaseComponent,
  SsiComponentConfig,
  SsiErrorCode,
} from '../../core/src/index';

import {
  SsiFsBackend,
  SsiFileStat,
  SsiDirEntry,
  MemFsBackend,
} from './backends';

export { SsiFsBackend, SsiFileStat, SsiDirEntry, MemFsBackend } from './backends';
export { SsiErrorCode } from '../../core/src/index';
export { O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, O_APPEND, SEEK_SET, SEEK_CUR, SEEK_END } from './backends';

// =========================================================================
// SSI-FS Mount Entry
// =========================================================================

interface MountEntry {
  path: string;
  backend: SsiFsBackend;
}

// =========================================================================
// FileSystem — SSI-FS Implementation
// =========================================================================

export class FileSystem extends SsiBaseComponent {
  private mounts: MountEntry[] = [];
  private defaultBackend: SsiFsBackend;

  constructor() {
    super({ name: 'filesystem', type: 'fs', version: '1.0.0', vendor: 'AI-TP' });
    this.defaultBackend = new MemFsBackend(100 * 1024 * 1024);
  }

  // =========================================================================
  // File Operations
  // =========================================================================

  /** Open a file */
  open(path: string, flags: number): SsiErrorCode {
    const { backend, relPath } = this.resolveBackend(path);
    return backend.open(relPath, flags);
  }

  /** Close a file descriptor */
  close(fd: number): SsiErrorCode {
    // Try all backends
    for (const mount of this.mounts) {
      const err = mount.backend.close(fd);
      if (err !== SsiErrorCode.NOT_FOUND) return err;
    }
    return this.defaultBackend.close(fd);
  }

  /** Read from a file */
  read(fd: number, size: number): { data: ArrayBuffer | null; error: SsiErrorCode } {
    for (const mount of this.mounts) {
      const result = mount.backend.read(fd, size);
      if (result.error !== SsiErrorCode.NOT_FOUND) return result;
    }
    return this.defaultBackend.read(fd, size);
  }

  /** Write to a file */
  write(fd: number, data: ArrayBuffer): SsiErrorCode {
    for (const mount of this.mounts) {
      const err = mount.backend.write(fd, data);
      if (err !== SsiErrorCode.NOT_FOUND) return err;
    }
    return this.defaultBackend.write(fd, data);
  }

  /** Seek to a position */
  seek(fd: number, offset: number, whence: number): SsiErrorCode {
    for (const mount of this.mounts) {
      const err = mount.backend.seek(fd, offset, whence);
      if (err !== SsiErrorCode.NOT_FOUND) return err;
    }
    return this.defaultBackend.seek(fd, offset, whence);
  }

  /** Get current file position */
  tell(fd: number): { position: number; error: SsiErrorCode } {
    for (const mount of this.mounts) {
      const result = mount.backend.tell(fd);
      if (result.error !== SsiErrorCode.NOT_FOUND) return result;
    }
    return this.defaultBackend.tell(fd);
  }

  // =========================================================================
  // Directory & Metadata Operations
  // =========================================================================

  /** Get file/directory stats */
  stat(path: string): { stat: SsiFileStat | null; error: SsiErrorCode } {
    const { backend, relPath } = this.resolveBackend(path);
    return backend.stat(relPath);
  }

  /** Create a directory */
  mkdir(path: string, mode: number = 0o755): SsiErrorCode {
    const { backend, relPath } = this.resolveBackend(path);
    return backend.mkdir(relPath, mode);
  }

  /** Read directory contents */
  readdir(path: string): { entries: SsiDirEntry[]; error: SsiErrorCode } {
    const { backend, relPath } = this.resolveBackend(path);
    return backend.readdir(relPath);
  }

  /** Delete a file */
  unlink(path: string): SsiErrorCode {
    const { backend, relPath } = this.resolveBackend(path);
    return backend.unlink(relPath);
  }

  /** Rename/move a file or directory */
  rename(oldPath: string, newPath: string): SsiErrorCode {
    // Cross-backend rename not supported — both paths must be on same backend
    const oldResolve = this.resolveBackend(oldPath);
    const newResolve = this.resolveBackend(newPath);
    if (oldResolve.backend !== newResolve.backend) {
      return SsiErrorCode.NOT_SUPPORTED;
    }
    return oldResolve.backend.rename(oldResolve.relPath, newResolve.relPath);
  }

  // =========================================================================
  // Filesystem Mounting
  // =========================================================================

  /** Mount a backend at a path */
  mount(path: string, backend: SsiFsBackend, config: Record<string, unknown> = {}): SsiErrorCode {
    const normalized = this.normalizePath(path);

    // Check for conflicts
    for (const m of this.mounts) {
      if (m.path === normalized) return SsiErrorCode.BUSY;
    }

    const err = backend.init(config);
    if (err !== SsiErrorCode.OK) return err;

    // Insert sorted by path depth (deepest first for matching)
    const depth = normalized.split('/').filter(Boolean).length;
    let i = 0;
    while (i < this.mounts.length) {
      const existingDepth = this.mounts[i].path.split('/').filter(Boolean).length;
      if (depth > existingDepth) break;
      i++;
    }
    this.mounts.splice(i, 0, { path: normalized, backend });

    this.log(`Mounted ${backend.name} at ${normalized}`);
    return SsiErrorCode.OK;
  }

  /** Unmount a backend from a path */
  unmount(path: string): SsiErrorCode {
    const normalized = this.normalizePath(path);
    const idx = this.mounts.findIndex(m => m.path === normalized);
    if (idx < 0) return SsiErrorCode.NOT_FOUND;

    this.mounts[idx].backend.destroy();
    this.mounts.splice(idx, 1);
    this.log(`Unmounted ${normalized}`);
    return SsiErrorCode.OK;
  }

  // =========================================================================
  // Virtual Filesystem Management
  // =========================================================================

  /** Create a named MEMFS */
  createMemfs(name: string, maxSize: number = 100 * 1024 * 1024): SsiErrorCode {
    const path = `/memfs/${name}`;
    for (const m of this.mounts) {
      if (m.path === path) return SsiErrorCode.BUSY;
    }

    const backend = new MemFsBackend(maxSize);
    return this.mount(path, backend, { maxSize });
  }

  /** Mount OPFS backend (browser only) — placeholder */
  mountOpfs(path: string): SsiErrorCode {
    return SsiErrorCode.NOT_SUPPORTED;
  }

  /** Mount IDBFS backend (browser only) — placeholder */
  mountIdbfs(path: string, _dbName: string): SsiErrorCode {
    return SsiErrorCode.NOT_SUPPORTED;
  }

  // =========================================================================
  // Lifecycle Hooks
  // =========================================================================

  protected async onInit(_config: SsiComponentConfig): Promise<void> {
    this.defaultBackend.init({});
    this.log('FileSystem initializing');
  }

  protected async onStart(): Promise<void> {
    this.log('FileSystem started');
  }

  protected async onStop(): Promise<void> {
    for (const mount of this.mounts) {
      mount.backend.destroy();
    }
    this.mounts = [];
    this.defaultBackend.destroy();
    this.log('FileSystem stopped');
  }

  protected onDestroy(): void {
    for (const mount of this.mounts) {
      mount.backend.destroy();
    }
    this.mounts = [];
    this.defaultBackend.destroy();
  }

  // =========================================================================
  // Internal
  // =========================================================================

  private resolveBackend(path: string): { backend: SsiFsBackend; relPath: string } {
    const normalized = this.normalizePath(path);

    // Find most specific mount
    let bestMatch: MountEntry | null = null;
    let bestDepth = -1;

    for (const mount of this.mounts) {
      if (normalized === mount.path || normalized.startsWith(mount.path + '/')) {
        const depth = mount.path.split('/').filter(Boolean).length;
        if (depth > bestDepth) {
          bestMatch = mount;
          bestDepth = depth;
        }
      }
    }

    if (bestMatch) {
      const relPath = normalized === bestMatch.path
        ? '/'
        : '/' + normalized.slice(bestMatch.path.length + 1);
      return { backend: bestMatch.backend, relPath };
    }

    return { backend: this.defaultBackend, relPath: normalized };
  }

  private normalizePath(path: string): string {
    const parts = path.split('/').filter(p => p && p !== '.');
    const result: string[] = [];
    for (const p of parts) {
      if (p === '..') { result.pop(); }
      else { result.push(p); }
    }
    return '/' + result.join('/');
  }
}
