/**
 * glibc-packages Browser Runtime — Core Types
 *
 * Defines the common types used across the WASM module loader,
 * virtual filesystem, networking, and threading modules.
 */

/** WebAssembly module initialization options */
export interface WasmModuleOptions {
  /** Path or URL to the .wasm file */
  wasmUrl: string;
  /** Path or URL to the Emscripten JS glue code */
  jsUrl?: string;
  /** Module name used for caching and debugging */
  name?: string;
  /** Initial memory size in MB (default: 64) */
  initialMemory?: number;
  /** Maximum memory size in MB (default: 512) */
  maximumMemory?: number;
  /** Enable POSIX filesystem via Emscripten (default: true) */
  enableFilesystem?: boolean;
  /** Enable pthreads via Web Workers (default: false) */
  enableThreads?: boolean;
  /** File system backend to use */
  fsBackend?: 'memfs' | 'opfs' | 'nodefs' | 'idbfs';
  /** Preload files into the virtual filesystem */
  preloadFiles?: VirtualFileEntry[];
  /** Environment variables to set inside the WASM runtime */
  env?: Record<string, string>;
  /** Module callback hooks */
  hooks?: WasmModuleHooks;
  /** Timeout for module initialization in ms (default: 30000) */
  timeout?: number;
}

/** A file to preload into the virtual filesystem */
export interface VirtualFileEntry {
  /** Absolute path inside the virtual filesystem (e.g., /usr/local/etc/config.ini) */
  path: string;
  /** File content as string, ArrayBuffer, or URL to fetch */
  content: string | ArrayBuffer | URL;
  /** File permissions (default: 0644) */
  mode?: number;
  /** Encoding for string content */
  encoding?: 'utf8' | 'binary';
}

/** Lifecycle hooks for WASM modules */
export interface WasmModuleHooks {
  /** Called before module initialization */
  onBeforeInit?: (name: string) => void;
  /** Called after module successfully initialized */
  onReady?: (name: string, exports: any) => void;
  /** Called on initialization error */
  onError?: (name: string, error: Error) => void;
  /** Called when module is being unloaded */
  onUnload?: (name: string) => void;
  /** Progress callback during loading (0-1) */
  onProgress?: (name: string, progress: number) => void;
}

/** Result of a WASM module initialization */
export interface WasmModuleResult {
  /** Module name */
  name: string;
  /** Emscripten module instance */
  module: any;
  /** WASM exports (functions, memory, etc.) */
  exports: WebAssembly.Exports;
  /** Module initialization time in ms */
  initTime: number;
  /** WASM memory size in bytes */
  memorySize: number;
}

/** File system abstraction layer */
export interface VirtualFileSystem {
  /** Write a file to the virtual filesystem */
  writeFile(path: string, data: Uint8Array | string): Promise<void>;
  /** Read a file from the virtual filesystem */
  readFile(path: string, encoding?: 'utf8' | 'binary'): Promise<Uint8Array | string>;
  /** Check if a path exists */
  exists(path: string): Promise<boolean>;
  /** Create a directory (recursive) */
  mkdir(path: string): Promise<void>;
  /** List directory contents */
  readdir(path: string): Promise<string[]>;
  /** Delete a file or empty directory */
  unlink(path: string): Promise<void>;
  /** Get file/directory stats */
  stat(path: string): Promise<FileStat>;
  /** Mount a new filesystem at the given path */
  mount(path: string, backend: FsBackend, options?: any): Promise<void>;
  /** Unmount a filesystem */
  unmount(path: string): Promise<void>;
  /** Get free/used space info */
  df(): Promise<DiskSpaceInfo>;
}

export interface FileStat {
  size: number;
  isDirectory: boolean;
  isFile: boolean;
  mode: number;
  mtime: Date;
  atime: Date;
  ctime: Date;
}

export type FsBackend = 'memfs' | 'opfs' | 'idbfs' | 'nodefs';

export interface DiskSpaceInfo {
  total: number;
  used: number;
  free: number;
}

/** Network abstraction for browser environments */
export interface BrowserNetwork {
  /** HTTP/HTTPS fetch wrapper that works with Emscripten's virtual socket */
  fetch(url: string, options?: RequestInit): Promise<Response>;
  /** Create a WebSocket connection */
  connectWebSocket(url: string, protocols?: string[]): WebSocket;
  /** Create a WebRTC peer connection */
  createPeerConnection(config?: RTCConfiguration): RTCPeerConnection;
  /** Check network connectivity */
  isOnline(): Promise<boolean>;
  /** Get estimated network speed (bytes per second) */
  getEstimatedSpeed(): Promise<number>;
}

/** Thread pool configuration */
export interface ThreadPoolConfig {
  /** Number of worker threads (default: navigator.hardwareConcurrency / 2) */
  threadCount?: number;
  /** URL to the worker script (if separate from main thread) */
  workerScriptUrl?: string;
  /** Stack size per thread in KB (default: 256) */
  stackSize?: number;
  /** Enable SharedArrayBuffer (requires COOP/COEP headers) */
  useSharedArrayBuffer?: boolean;
}

/** Runtime environment detection */
export type RuntimeEnvironment = 'browser' | 'node' | 'worker' | 'service-worker' | 'unknown';

/** Global runtime configuration */
export interface RuntimeConfig {
  /** Log level */
  logLevel: 'debug' | 'info' | 'warn' | 'error' | 'silent';
  /** Default WASM loading timeout in ms */
  defaultTimeout: number;
  /** Enable diagnostic logging */
  diagnostics: boolean;
  /** Cache WASM modules after loading */
  cacheModules: boolean;
  /** Maximum number of simultaneous WASM module loads */
  maxConcurrentLoads: number;
}

export const DEFAULT_RUNTIME_CONFIG: RuntimeConfig = {
  logLevel: 'info',
  defaultTimeout: 30000,
  diagnostics: false,
  cacheModules: true,
  maxConcurrentLoads: 4,
};

/** Detect the current JavaScript runtime environment */
export function detectEnvironment(): RuntimeEnvironment {
  if (typeof window !== 'undefined' && typeof window.document !== 'undefined') {
    return 'browser';
  }
  if (typeof WorkerGlobalScope !== 'undefined' && self instanceof WorkerGlobalScope) {
    return 'worker';
  }
  if (typeof ServiceWorkerGlobalScope !== 'undefined') {
    return 'service-worker';
  }
  if (typeof process !== 'undefined' && process.versions?.node) {
    return 'node';
  }
  return 'unknown';
}

/** Format bytes to human-readable string */
export function formatBytes(bytes: number): string {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(2))} ${sizes[i]}`;
}

/** Logger utility */
export class Logger {
  private level: number;
  private readonly levels = { debug: 0, info: 1, warn: 2, error: 3, silent: 4 };

  constructor(level: keyof typeof Logger.prototype.levels = 'info') {
    this.level = this.levels[level] ?? 1;
  }

  private log(level: keyof typeof Logger.prototype.levels, msg: string, ...args: any[]) {
    if (this.levels[level] >= this.level) {
      const prefix = `[glibc-runtime:${level.toUpperCase()}]`;
      const fn = level === 'error' ? console.error
               : level === 'warn'  ? console.warn
               : level === 'debug' ? console.debug
               : console.log;
      fn(prefix, msg, ...args);
    }
  }

  debug(msg: string, ...args: any[]) { this.log('debug', msg, ...args); }
  info(msg: string, ...args: any[])  { this.log('info', msg, ...args); }
  warn(msg: string, ...args: any[])  { this.log('warn', msg, ...args); }
  error(msg: string, ...args: any[]) { this.log('error', msg, ...args); }
}
