/**
 * SSI-FS: Virtual File System Backends
 *
 * Multiple backend implementations:
 *   - MEMFS: In-memory filesystem (fast, non-persistent)
 *   - NODEFS: Node.js fs wrapper (host filesystem access)
 *   - OPFS: Origin Private File System (browser persistent storage)
 *   - IDBFS: IndexedDB-backed filesystem (persistent in browser)
 */

import { SsiErrorCode } from '../../core/src/index';
export { SsiErrorCode };

// =========================================================================
// Backend Interface
// =========================================================================

export interface SsiFsBackend {
  readonly name: string;

  // File operations
  open(path: string, flags: number): SsiErrorCode;
  close(fd: number): SsiErrorCode;
  read(fd: number, size: number): { data: ArrayBuffer | null; error: SsiErrorCode };
  write(fd: number, data: ArrayBuffer): SsiErrorCode;
  seek(fd: number, offset: number, whence: number): SsiErrorCode;
  tell(fd: number): { position: number; error: SsiErrorCode };

  // Directory & metadata
  stat(path: string): { stat: SsiFileStat | null; error: SsiErrorCode };
  mkdir(path: string, mode: number): SsiErrorCode;
  readdir(path: string): { entries: SsiDirEntry[] | null; error: SsiErrorCode };
  unlink(path: string): SsiErrorCode;
  rename(oldPath: string, newPath: string): SsiErrorCode;

  // Lifecycle
  init(config: Record<string, unknown>): SsiErrorCode;
  destroy(): void;
}

export interface SsiFileStat {
  size: number;
  mode: number;
  isDirectory: boolean;
  isFile: boolean;
  createdAt: number;
  modifiedAt: number;
}

export interface SsiDirEntry {
  name: string;
  type: number;  // 0=file, 1=dir
  size: number;
}

// =========================================================================
// File Flags (mirrors POSIX subset)
// =========================================================================

export const O_RDONLY = 0;
export const O_WRONLY = 1;
export const O_RDWR = 2;
export const O_CREAT = 64;
export const O_TRUNC = 512;
export const O_APPEND = 1024;

export const SEEK_SET = 0;
export const SEEK_CUR = 1;
export const SEEK_END = 2;

// =========================================================================
// MEMFS — In-Memory Backend
// =========================================================================

interface MemNode {
  data: ArrayBuffer;
  mode: number;
  isDirectory: boolean;
  createdAt: number;
  modifiedAt: number;
  children: Map<string, MemNode>;
}

interface MemFileDesc {
  path: string;
  flags: number;
  position: number;
}

export class MemFsBackend implements SsiFsBackend {
  readonly name = 'memfs';

  private root: MemNode;
  private files = new Map<number, MemFileDesc>();
  private nextFd = 3; // 0,1,2 = stdin/stdout/stderr
  private maxSize: number;

  constructor(maxSize: number = 100 * 1024 * 1024) {
    this.maxSize = maxSize;
    this.root = this.createDirNode(0o755);
  }

  init(config: Record<string, unknown>): SsiErrorCode {
    if (typeof config.maxSize === 'number') {
      this.maxSize = config.maxSize;
    }
    return SsiErrorCode.OK;
  }

  destroy(): void {
    this.root = this.createDirNode(0o755);
    this.files.clear();
  }

  // ── File Operations ──

  open(path: string, flags: number): SsiErrorCode {
    const normalized = this.normalize(path);
    const node = this.resolveNode(normalized);

    if (!node) {
      if (!(flags & O_CREAT)) return SsiErrorCode.NOT_FOUND;
      return this.createFile(normalized, flags);
    }

    if (node.isDirectory) return SsiErrorCode.INVALID;

    const fd = this.nextFd++;
    this.files.set(fd, {
      path: normalized,
      flags,
      position: (flags & O_APPEND) ? node.data.byteLength : 0,
    });

    return SsiErrorCode.OK;
  }

  close(fd: number): SsiErrorCode {
    if (!this.files.has(fd)) return SsiErrorCode.NOT_FOUND;
    this.files.delete(fd);
    return SsiErrorCode.OK;
  }

  read(fd: number, size: number): { data: ArrayBuffer | null; error: SsiErrorCode } {
    const desc = this.files.get(fd);
    if (!desc) return { data: null, error: SsiErrorCode.NOT_FOUND };
    if (desc.flags === O_WRONLY) return { data: null, error: SsiErrorCode.PERMISSION };

    const node = this.resolveNode(desc.path);
    if (!node || node.isDirectory) return { data: null, error: SsiErrorCode.NOT_FOUND };

    const data = node.data;
    const start = desc.position;
    const end = Math.min(start + size, data.byteLength);
    const slice = data.slice(start, end);
    desc.position = end;

    return { data: slice, error: SsiErrorCode.OK };
  }

  write(fd: number, data: ArrayBuffer): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;
    if (desc.flags === O_RDONLY) return SsiErrorCode.PERMISSION;

    const node = this.resolveNode(desc.path);
    if (!node || node.isDirectory) return SsiErrorCode.NOT_FOUND;

    const newSize = desc.position + data.byteLength;
    if (newSize > this.maxSize) return SsiErrorCode.MEMORY;

    // Extend or replace the buffer
    const existing = node.data;
    const combined = new Uint8Array(Math.max(newSize, existing.byteLength));
    combined.set(new Uint8Array(existing), 0);
    combined.set(new Uint8Array(data), desc.position);
    node.data = combined.buffer as ArrayBuffer;
    node.modifiedAt = Date.now();
    desc.position = newSize;

    return SsiErrorCode.OK;
  }

  seek(fd: number, offset: number, whence: number): SsiErrorCode {
    const desc = this.files.get(fd);
    if (!desc) return SsiErrorCode.NOT_FOUND;

    const node = this.resolveNode(desc.path);
    if (!node) return SsiErrorCode.NOT_FOUND;

    let newPos: number;
    switch (whence) {
      case SEEK_SET: newPos = offset; break;
      case SEEK_CUR: newPos = desc.position + offset; break;
      case SEEK_END: newPos = node.data.byteLength + offset; break;
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

  // ── Directory & Metadata ──

  stat(path: string): { stat: SsiFileStat | null; error: SsiErrorCode } {
    const node = this.resolveNode(this.normalize(path));
    if (!node) return { stat: null, error: SsiErrorCode.NOT_FOUND };

    return {
      stat: {
        size: node.isDirectory ? 0 : node.data.byteLength,
        mode: node.mode,
        isDirectory: node.isDirectory,
        isFile: !node.isDirectory,
        createdAt: node.createdAt,
        modifiedAt: node.modifiedAt,
      },
      error: SsiErrorCode.OK,
    };
  }

  mkdir(path: string, mode: number): SsiErrorCode {
    const normalized = this.normalize(path);
    if (this.resolveNode(normalized)) return SsiErrorCode.BUSY;

    const parentPath = this.dirname(normalized);
    const parent = this.resolveNode(parentPath);
    if (!parent || !parent.isDirectory) return SsiErrorCode.NOT_FOUND;

    const name = this.basename(normalized);
    parent.children.set(name, this.createDirNode(mode));
    parent.modifiedAt = Date.now();
    return SsiErrorCode.OK;
  }

  readdir(path: string): { entries: SsiDirEntry[] | null; error: SsiErrorCode } {
    const node = this.resolveNode(this.normalize(path));
    if (!node || !node.isDirectory) return { entries: [], error: SsiErrorCode.NOT_FOUND };

    const entries: SsiDirEntry[] = [
      { name: '.', type: 1, size: 0 },
      { name: '..', type: 1, size: 0 },
    ];

    for (const [name, child] of node.children) {
      entries.push({
        name,
        type: child.isDirectory ? 1 : 0,
        size: child.isDirectory ? 0 : child.data.byteLength,
      });
    }

    return { entries, error: SsiErrorCode.OK };
  }

  unlink(path: string): SsiErrorCode {
    const normalized = this.normalize(path);
    const node = this.resolveNode(normalized);
    if (!node) return SsiErrorCode.NOT_FOUND;
    if (node.isDirectory) return SsiErrorCode.INVALID;

    const parentPath = this.dirname(normalized);
    const parent = this.resolveNode(parentPath);
    if (parent) {
      parent.children.delete(this.basename(normalized));
      parent.modifiedAt = Date.now();
    }

    return SsiErrorCode.OK;
  }

  rename(oldPath: string, newPath: string): SsiErrorCode {
    const oldNorm = this.normalize(oldPath);
    const newNorm = this.normalize(newPath);

    const node = this.resolveNode(oldNorm);
    if (!node) return SsiErrorCode.NOT_FOUND;
    if (this.resolveNode(newNorm)) return SsiErrorCode.BUSY;

    const oldParentPath = this.dirname(oldNorm);
    const oldParent = this.resolveNode(oldParentPath);
    const newParentPath = this.dirname(newNorm);
    const newParent = this.resolveNode(newParentPath);

    if (!oldParent || !newParent) return SsiErrorCode.NOT_FOUND;

    const name = this.basename(oldNorm);
    oldParent.children.delete(name);
    newParent.children.set(this.basename(newNorm), node);

    return SsiErrorCode.OK;
  }

  // ── Internal ──

  private normalize(path: string): string {
    // Collapse '/./' and '//', resolve '..'
    const parts = path.split('/').filter(p => p && p !== '.');
    const result: string[] = [];
    for (const p of parts) {
      if (p === '..') { result.pop(); }
      else { result.push(p); }
    }
    return '/' + result.join('/');
  }

  private resolveNode(path: string): MemNode | null {
    if (path === '/') return this.root;
    const parts = path.split('/').filter(p => p);
    let current = this.root;
    for (const p of parts) {
      if (!current.children.has(p)) return null;
      current = current.children.get(p)!;
    }
    return current;
  }

  private dirname(path: string): string {
    const parts = path.split('/').filter(p => p);
    parts.pop();
    return '/' + parts.join('/');
  }

  private basename(path: string): string {
    const parts = path.split('/').filter(p => p);
    return parts[parts.length - 1] || '';
  }

  private createFile(path: string, flags: number): SsiErrorCode {
    const parentPath = this.dirname(path);
    const parent = this.resolveNode(parentPath);
    if (!parent || !parent.isDirectory) return SsiErrorCode.NOT_FOUND;

    const name = this.basename(path);
    const now = Date.now();
    parent.children.set(name, {
      data: new ArrayBuffer(0),
      mode: 0o644,
      isDirectory: false,
      createdAt: now,
      modifiedAt: now,
      children: new Map(),
    });
    parent.modifiedAt = now;

    const fd = this.nextFd++;
    this.files.set(fd, {
      path,
      flags,
      position: 0,
    });

    return SsiErrorCode.OK;
  }

  private createDirNode(mode: number): MemNode {
    const now = Date.now();
    return {
      data: new ArrayBuffer(0),
      mode,
      isDirectory: true,
      createdAt: now,
      modifiedAt: now,
      children: new Map(),
    };
  }
}
export { SsiErrorCode } from '../../core/src/index';
