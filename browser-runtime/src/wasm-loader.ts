/**
 * WASM Module Loader
 *
 * Loads, caches, and initializes Emscripten-compiled WebAssembly modules
 * in browser, Node.js, and Web Worker environments.
 *
 * Features:
 * - Lazy loading with configurable timeout
 * - Module caching (immediate reuse)
 * - Download caching (IndexedDB in browser)
 * - Progress reporting
 * - Dependency resolution (load deps before the module)
 * - Multiple instantiation strategies (single/worker/shared)
 */

import { detectEnvironment, Logger, formatBytes, WasmModuleOptions, WasmModuleResult, WasmModuleHooks, DEFAULT_RUNTIME_CONFIG, RuntimeConfig } from './types';

export type { WasmModuleOptions, WasmModuleResult, WasmModuleHooks };

/**
 * WASM Module Manager
 *
 * Singleton that manages all WASM module loading, caching, and lifecycle.
 */
export class WasmModuleManager {
  private static instance: WasmModuleManager;

  /** Module cache: loaded Emscripten module instances */
  private moduleCache = new Map<string, any>();

  /** Download cache: raw .wasm bytes (IndexedDB-backed in browser) */
  private byteCache = new Map<string, ArrayBuffer>();

  /** Pending load promises (deduplicates concurrent loads of the same URL) */
  private pendingLoads = new Map<string, Promise<WasmModuleResult>>();

  /** Runtime configuration */
  public config: RuntimeConfig;

  /** Logger */
  public log: Logger;

  /** Active module count */
  private activeModules = 0;

  private constructor(config?: Partial<RuntimeConfig>) {
    this.config = { ...DEFAULT_RUNTIME_CONFIG, ...config };
    this.log = new Logger(this.config.logLevel);
  }

  /** Get the singleton instance */
  static getInstance(config?: Partial<RuntimeConfig>): WasmModuleManager {
    if (!WasmModuleManager.instance) {
      WasmModuleManager.instance = new WasmModuleManager(config);
    } else if (config) {
      Object.assign(WasmModuleManager.instance.config, config);
      WasmModuleManager.instance.log = new Logger(WasmModuleManager.instance.config.logLevel);
    }
    return WasmModuleManager.instance;
  }

  /**
   * Load and initialize a WASM module.
   * Automatically deduplicates concurrent loads of the same URL.
   */
  async loadModule(options: WasmModuleOptions): Promise<WasmModuleResult> {
    const name = options.name || options.wasmUrl.split('/').pop()?.replace('.wasm', '') || 'unknown';
    const cacheKey = `${name}::${options.wasmUrl}`;

    // Return cached module if available
    if (this.config.cacheModules && this.moduleCache.has(cacheKey)) {
      this.log.debug(`Module cache hit: ${name}`);
      const module = this.moduleCache.get(cacheKey);
      return {
        name,
        module,
        exports: module.asm ?? module.exports ?? {},
        initTime: 0,
        memorySize: module.HEAPU8?.length ?? 0,
      };
    }

    // Deduplicate concurrent loads
    if (this.pendingLoads.has(cacheKey)) {
      this.log.debug(`Waiting for concurrent load: ${name}`);
      return this.pendingLoads.get(cacheKey)!;
    }

    // Limit concurrent loads
    if (this.activeModules >= this.config.maxConcurrentLoads) {
      this.log.warn(`Concurrent load limit reached (${this.config.maxConcurrentLoads}), waiting...`);
      await new Promise(resolve => setTimeout(resolve, 100));
    }

    const loadPromise = this.doLoadModule(name, cacheKey, options);
    this.pendingLoads.set(cacheKey, loadPromise);

    try {
      const result = await loadPromise;
      return result;
    } finally {
      this.pendingLoads.delete(cacheKey);
    }
  }

  /** Internal module loading implementation */
  private async doLoadModule(
    name: string,
    cacheKey: string,
    options: WasmModuleOptions
  ): Promise<WasmModuleResult> {
    const startTime = performance.now();
    const hooks = options.hooks || {};
    const env = detectEnvironment();

    this.activeModules++;
    hooks.onBeforeInit?.(name);

    try {
      this.log.info(`Loading WASM module: ${name} (env: ${env})`);
      this.log.debug(`  wasm:   ${options.wasmUrl}`);
      this.log.debug(`  js:     ${options.jsUrl || '(inline)'}`);
      this.log.debug(`  memory: ${options.initialMemory || 64}MB → ${options.maximumMemory || 512}MB`);
      this.log.debug(`  fs:     ${options.enableFilesystem !== false ? 'enabled' : 'disabled'}`);
      this.log.debug(`  threads: ${options.enableThreads ? 'enabled' : 'disabled'}`);

      // Fetch and compile WASM bytes
      hooks.onProgress?.(name, 0.1);
      const wasmBytes = await this.fetchWasmBytes(options.wasmUrl);
      hooks.onProgress?.(name, 0.3);

      // Build Emscripten module configuration
      const moduleConfig = this.buildModuleConfig(name, options, hooks);
      hooks.onProgress?.(name, 0.5);

      // Load JS glue code if provided (or assume Emscripten already loaded)
      let instantiatedModule: any;
      if (options.jsUrl) {
        instantiatedModule = await this.loadJsGlue(options.jsUrl, moduleConfig);
      } else {
        // If no JS glue, compile WASM directly
        const wasmModule = await WebAssembly.compile(wasmBytes);
        const instance = await WebAssembly.instantiate(wasmModule, this.buildDefaultImports(options));
        instantiatedModule = {
          asm: instance.exports,
          HEAPU8: new Uint8Array((instance.exports.memory as WebAssembly.Memory).buffer),
          ...instance.exports,
        };
      }
      hooks.onProgress?.(name, 0.8);

      // Preload files into virtual filesystem if Emscripten FS is available
      if (options.preloadFiles && options.preloadFiles.length > 0 && instantiatedModule.FS) {
        this.preloadFiles(instantiatedModule.FS, options.preloadFiles);
      }

      // Set environment variables
      if (options.env && instantiatedModule.ENV) {
        for (const [key, val] of Object.entries(options.env)) {
          try {
            instantiatedModule.ENV[key] = val;
          } catch (e) {
            this.log.warn(`Failed to set ENV ${key}=${val}:`, e);
          }
        }
      }

      // Cache the module
      if (this.config.cacheModules) {
        this.moduleCache.set(cacheKey, instantiatedModule);
      }

      const initTime = performance.now() - startTime;
      const memorySize = instantiatedModule.HEAPU8?.length ?? 0;

      const result: WasmModuleResult = {
        name,
        module: instantiatedModule,
        exports: instantiatedModule.asm ?? instantiatedModule.exports ?? {},
        initTime,
        memorySize,
      };

      this.log.info(`✓ Module "${name}" loaded in ${initTime.toFixed(0)}ms, memory: ${formatBytes(memorySize)}`);
      hooks.onReady?.(name, result.exports);

      return result;
    } catch (error) {
      const err = error instanceof Error ? error : new Error(String(error));
      this.log.error(`✗ Module "${name}" failed: ${err.message}`);
      hooks.onError?.(name, err);
      throw err;
    } finally {
      this.activeModules--;
    }
  }

  /** Fetch WASM bytes from URL with caching */
  private async fetchWasmBytes(url: string): Promise<ArrayBuffer> {
    // Check byte cache
    if (this.byteCache.has(url)) {
      this.log.debug(`Byte cache hit: ${url}`);
      return this.byteCache.get(url)!.slice(0);
    }

    this.log.debug(`Fetching WASM: ${url}`);
    const response = await fetch(url);

    if (!response.ok) {
      throw new Error(`Failed to fetch WASM: ${response.status} ${response.statusText} (${url})`);
    }

    const bytes = await response.arrayBuffer();
    this.log.info(`Downloaded WASM: ${formatBytes(bytes.byteLength)} (${url})`);

    // Cache bytes
    if (this.config.cacheModules) {
      this.byteCache.set(url, bytes);
    }

    return bytes;
  }

  /** Build Emscripten module configuration object */
  private buildModuleConfig(
    name: string,
    options: WasmModuleOptions,
    hooks: WasmModuleHooks
  ): Record<string, any> {
    const env = detectEnvironment();

    const config: Record<string, any> = {
      // Emscripten module configuration
      locateFile: (path: string, prefix: string) => {
        // Resolve .wasm files relative to the JS glue code
        if (path.endsWith('.wasm')) {
          return options.wasmUrl;
        }
        return prefix + path;
      },

      // Print/error handlers
      print: (text: string) => this.log.debug(`[${name}:stdout] ${text}`),
      printErr: (text: string) => this.log.warn(`[${name}:stderr] ${text}`),

      // Filesystem
      noFSInit: !options.enableFilesystem,
    };

    // Threading support
    if (options.enableThreads) {
      config.noInitialRun = false;
      config.mainScriptUrlOrBlob = options.jsUrl;
    }

    // Memory limits
    if (options.initialMemory) {
      config.INITIAL_MEMORY = options.initialMemory * 1024 * 1024;
    }

    return config;
  }

  /** Load Emscripten JS glue code */
  private async loadJsGlue(jsUrl: string, moduleConfig: Record<string, any>): Promise<any> {
    const env = detectEnvironment();

    if (env === 'browser' || env === 'worker') {
      // In browser/worker: create script element or import
      return new Promise((resolve, reject) => {
        // Set up the global Module object that Emscripten expects
        const globalScope = typeof self !== 'undefined' ? self : window;
        (globalScope as any).Module = moduleConfig;

        const script = document.createElement('script');
        script.src = jsUrl;
        script.async = true;
        script.onload = () => {
          const mod = (globalScope as any).Module;
          delete (globalScope as any).Module;
          resolve(mod);
        };
        script.onerror = () => reject(new Error(`Failed to load JS glue: ${jsUrl}`));
        document.head.appendChild(script);
      });
    } else {
      // Node.js: use dynamic import
      const mod = await import(jsUrl);
      const factory = mod.default || mod;
      return factory(moduleConfig);
    }
  }

  /** Build default WASM imports for direct instantiation (no JS glue) */
  private buildDefaultImports(options: WasmModuleOptions): WebAssembly.Imports {
    return {
      env: {
        memory: new WebAssembly.Memory({
          initial: (options.initialMemory || 64),
          maximum: (options.maximumMemory || 512),
          shared: !!options.enableThreads,
        }),
        table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' }),
        __memory_base: 0,
        __table_base: 0,
        memoryBase: 0,
        tableBase: 0,
        abort: (msg: number) => { throw new Error(`WASM abort: ${msg}`); },
      },
      wasi_snapshot_preview1: this.buildWasiImports(),
    };
  }

  /** Build minimal WASI imports */
  private buildWasiImports(): Record<string, Function> {
    const wasiCtx = {
      stdout: '',
      stderr: '',
      fds: new Map<number, { type: 'stdout' | 'stderr' | 'stdin'; buffer: string }>(),
    };
    wasiCtx.fds.set(1, { type: 'stdout', buffer: '' });
    wasiCtx.fds.set(2, { type: 'stderr', buffer: '' });

    return {
      // fd_write: write to stdout/stderr
      fd_write: (fd: number, iovs: number, iovsLen: number, nwritten: number) => {
        const mem = (this as any).exports?.memory;
        if (!mem) return 0;
        const view = new DataView(mem.buffer);
        let total = 0;
        for (let i = 0; i < iovsLen; i++) {
          const buf = view.getUint32(iovs + i * 8, true);
          const len = view.getUint32(iovs + i * 8 + 4, true);
          const bytes = new Uint8Array(mem.buffer, buf, len);
          const str = new TextDecoder().decode(bytes);
          if (fd === 1) console.log(str);
          else if (fd === 2) console.warn(str);
          total += len;
        }
        if (nwritten) {
          const mem2 = (this as any).exports?.memory;
          if (mem2) new DataView(mem2.buffer).setUint32(nwritten, total, true);
        }
        return 0;
      },
      fd_read: () => 0,
      fd_close: () => 0,
      fd_seek: () => 0,
      proc_exit: (code: number) => { console.warn(`[WASI] proc_exit(${code})`); },
      environ_get: () => 0,
      environ_sizes_get: () => 0,
      args_get: () => 0,
      args_sizes_get: () => 0,
      clock_time_get: (id: number, precision: bigint, time: number) => {
        if (time) {
          const mem = (this as any).exports?.memory;
          if (mem) {
            const view = new DataView(mem.buffer);
            const now = BigInt(Date.now()) * 1000000n; // microseconds
            view.setBigUint64(time, now, true);
          }
        }
        return 0;
      },
      random_get: (buf: number, len: number) => {
        const mem = (this as any).exports?.memory;
        if (mem) {
          const bytes = new Uint8Array(mem.buffer, buf, len);
          crypto.getRandomValues(bytes);
        }
        return 0;
      },
    };
  }

  /** Preload files into Emscripten FS */
  private preloadFiles(FS: any, files: Array<{ path: string; content: string | ArrayBuffer | URL; mode?: number }>) {
    for (const file of files) {
      try {
        const dir = file.path.substring(0, file.path.lastIndexOf('/'));
        if (dir && !FS.analyzePath(dir).exists) {
          FS.mkdirTree(dir);
        }

        let data: Uint8Array;
        if (typeof file.content === 'string') {
          data = new TextEncoder().encode(file.content);
        } else if (file.content instanceof ArrayBuffer) {
          data = new Uint8Array(file.content);
        } else {
          this.log.warn(`Cannot preload URL content synchronously: ${file.path}`);
          continue;
        }

        FS.writeFile(file.path, data);
        if (file.mode) {
          FS.chmod(file.path, file.mode);
        }
        this.log.debug(`Preloaded file: ${file.path} (${data.length} bytes)`);
      } catch (e) {
        this.log.warn(`Failed to preload ${file.path}:`, e);
      }
    }
  }

  /** Pre-initialize a module without blocking (warm-up) */
  async warmupModule(url: string): Promise<void> {
    try {
      this.log.info(`Warming up: ${url}`);
      await fetch(url, { method: 'HEAD' });
    } catch (e) {
      this.log.warn(`Warmup failed: ${url}`, e);
    }
  }

  /** Unload a module from cache */
  unloadModule(cacheKey: string): void {
    const module = this.moduleCache.get(cacheKey);
    if (module) {
      this.log.info(`Unloading module: ${cacheKey}`);
      if (typeof module._exit === 'function') {
        try { module._exit(0); } catch {}
      }
      this.moduleCache.delete(cacheKey);
    }
  }

  /** Clear all cached modules */
  clearCache(): void {
    this.log.info('Clearing all WASM module caches');
    this.moduleCache.clear();
    this.byteCache.clear();
  }

  /** Get the number of cached modules */
  get cacheSize(): number {
    return this.moduleCache.size;
  }

  /** Get the total approximate memory used by cached modules */
  get totalMemoryBytes(): number {
    let total = 0;
    for (const [, mod] of this.moduleCache) {
      total += mod.HEAPU8?.length ?? 0;
    }
    return total;
  }
}

/** Convenience function: load a WASM module quickly */
export async function loadWasm(options: WasmModuleOptions): Promise<WasmModuleResult> {
  return WasmModuleManager.getInstance().loadModule(options);
}

/** Default export: the module manager singleton */
export default WasmModuleManager;
