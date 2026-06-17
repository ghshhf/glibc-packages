/**
 * SkyNet SSI-KRN: WASM Runtime Kernel
 *
 * Standard WASM Runtime implementation of the SSI-KRN interface.
 * Loads, manages, and executes .swbn standard components across all
 * platforms (browser, Node.js, Web Workers).
 *
 * This is the **runtime kernel** of SkyNet (天网) — it provides the
 * WASM execution environment that all system components run on top of.
 *
 * SSI Interfaces implemented:
 *   - SSI-CORE: Component lifecycle (init → start → suspend/resume → stop)
 *   - SSI-KRN:  WASM module loading, process management, memory management
 *
 * Usage:
 * ```typescript
 * import { SsiRuntime } from '@glibc-packages/browser-runtime';
 *
 * // Initialize the SSI runtime kernel
 * const kernel = SsiRuntime.init({ logLevel: 'info' });
 *
 * // Load a .swbn component
 * const component = await kernel.loadModule({
 *   wasmUrl: '/components/browser-engine.wasm',
 *   name: 'browser-engine',
 *   interface: 'SSI-BR',
 * });
 *
 * // Initialize via SSI-CORE lifecycle
 * component.init(config);
 * component.start();
 * ```
 */

import { WasmModuleManager, loadWasm } from './wasm-loader';
import { VirtualFS } from './virtual-fs';
import { BrowserNetworkImpl } from './networking';
import { ThreadPool, WasmMutex, WasmConditionVariable } from './threading';
import {
  Logger,
  RuntimeConfig,
  DEFAULT_RUNTIME_CONFIG,
  detectEnvironment,
  formatBytes,
} from './types';

export {
  // SSI-KRN: WASM Module loader
  WasmModuleManager, loadWasm,

  // SSI-FS: Virtual filesystem
  VirtualFS,

  // SSI-NET: Networking
  BrowserNetworkImpl,

  // SSI-KRN: Threading
  ThreadPool, WasmMutex, WasmConditionVariable,

  // SSI-CORE: Runtime kernel
  SsiRuntime, SSI_COMPONENT_STATE,

  // Types
  Logger, formatBytes, detectEnvironment, RuntimeConfig,
};

export type {
  SsiComponentHandle,
  WasmModuleOptions, WasmModuleResult, WasmModuleHooks,
  VirtualFileSystem, FileStat, FsBackend,
  BrowserNetwork,
  ThreadPoolConfig, ThreadHandle,
} from './types';

/**
 * AI-TP OS SSI-KRN Runtime Kernel
 *
 * Singleton that manages all subsystems:
 * - WASM module loading/caching (SSI-KRN)
 * - Virtual filesystem (SSI-FS)
 * - Networking (SSI-NET frontend)
 * - Thread pool (SSI-KRN threading)
 * - SSI-CORE component lifecycle
 * - .swbn component loading
 */
export class SsiRuntime {
  private static instance: SsiRuntime;

  /** Logger instance */
  public log: Logger;

  /** Runtime configuration */
  public config: RuntimeConfig;

  /** WASM module manager */
  public wasm: WasmModuleManager;

  /** Virtual filesystem */
  public fs: VirtualFS;

  /** Networking abstraction */
  public net: BrowserNetworkImpl;

  /** Thread pool */
  public threads: ThreadPool;

  /** Environment detection result */
  public readonly environment = detectEnvironment();

  /** Runtime initialized flag */
  private _initialized = false;

  private constructor(config?: Partial<RuntimeConfig>) {
    this.config = { ...DEFAULT_RUNTIME_CONFIG, ...config };
    this.log = new Logger(this.config.logLevel);

    // Initialize subsystems (lazy)
    this.wasm = WasmModuleManager.getInstance(config);
    this.fs = VirtualFS.getInstance();
    this.net = BrowserNetworkImpl.getInstance();
    this.threads = ThreadPool.getInstance({
      useSharedArrayBuffer: this.checkFeature('sharedArrayBuffer'),
    });
  }

  /**
   * Initialize the SSI runtime kernel.
   * Call once at application startup.
   * Implements SSI-CORE.init()
   */
  static init(config?: Partial<RuntimeConfig>): SsiRuntime {
    if (!SsiRuntime.instance) {
      SsiRuntime.instance = new SsiRuntime(config);
      SsiRuntime.instance._initialize();
    }
    return SsiRuntime.instance;
  }

  /** Get the existing runtime instance (throws if not initialized) */
  static getInstance(): SsiRuntime {
    if (!SsiRuntime.instance) {
      throw new Error('SsiRuntime not initialized. Call SsiRuntime.init() first.');
    }
    return SsiRuntime.instance;
  }

  /** Internal initialization */
  private _initialize(): void {
    if (this._initialized) return;

    this.log.info(`SkyNet SSI-KRN Runtime v1.0.0`);
    this.log.info(`Environment: ${this.environment}`);
    this.log.info(`Config: ${JSON.stringify(this.config)}`);

    // Report feature support
    this.reportFeatureSupport();

    this._initialized = true;
  }

  /** Check if a browser feature is available */
  private checkFeature(feature: string): boolean {
    const env = this.environment;
    if (env !== 'browser' && env !== 'worker') return false;

    try {
      switch (feature) {
        case 'sharedArrayBuffer':
          return typeof SharedArrayBuffer !== 'undefined';
        case 'webAssembly':
          return typeof WebAssembly !== 'undefined' &&
                 typeof WebAssembly.instantiate === 'function';
        case 'webSocket':
          return typeof WebSocket !== 'undefined';
        case 'webRTC':
          return typeof RTCPeerConnection !== 'undefined';
        case 'opfs':
          return typeof navigator !== 'undefined' &&
                 typeof navigator.storage?.getDirectory === 'function';
        case 'webWorker':
          return typeof Worker !== 'undefined';
        case 'simd':
          return WebAssembly.validate(new Uint8Array([
            0, 97, 115, 109, 1, 0, 0, 0,  // wasm header
            1, 5, 1, 96, 0, 1, 123,        // type section: v128 return
            3, 2, 1, 0,                     // function section
            10, 10, 1, 8, 0, 65, 0, 253, 15, 253, 15, 11  // i8x16.splat
          ]));
        default:
          return false;
      }
    } catch {
      return false;
    }
  }

  /** Report available features */
  private reportFeatureSupport(): void {
    const features = [
      'webAssembly', 'sharedArrayBuffer', 'webSocket', 'webRTC',
      'opfs', 'webWorker', 'simd'
    ] as const;

    const platformFeatures = {
      webAssembly: 'WASM',
      sharedArrayBuffer: 'SAB',
      webSocket: 'WebSocket',
      webRTC: 'WebRTC',
      opfs: 'OPFS',
      webWorker: 'WebWorker',
      simd: 'SIMD',
    } as const;

    const support: string[] = [];
    const missing: string[] = [];

    for (const feat of features) {
      if (this.checkFeature(feat)) {
        support.push(platformFeatures[feat]);
      } else {
        missing.push(platformFeatures[feat]);
      }
    }

    this.log.info(`Features: [+${support.join(',+')}]`);
    if (missing.length > 0) {
      this.log.warn(`Missing: [-${missing.join(',-')}]`);
    }
  }

  /** Check if the runtime has been initialized */
  get isInitialized(): boolean {
    return this._initialized;
  }

  /** Get initialization summary */
  getSummary(): string {
    const lines = [
      '╔═══════════════════════════════════════╗',
      '║  SkyNet SSI-KRN Runtime              ║',
      '╠═══════════════════════════════════════╣',
      `║  Environment: ${this.environment.padEnd(24)}║`,
      `║  WASM Modules: ${String(this.wasm.cacheSize).padEnd(21)}║`,
      `║  FS: ${this.fs.isConnected ? 'connected' : 'standalone'.padEnd(20)}║`,
      `║  Memory: ${formatBytes(this.wasm.totalMemoryBytes).padEnd(25)}║`,
      '╚═══════════════════════════════════════╝',
    ];
    return lines.join('\n');
  }

  /** Clean up all resources (SSI-CORE.destroy) */
  dispose(): void {
    this.log.info('Disposing SsiRuntime...');
    this.wasm.clearCache();
    this.threads.terminate();
    this.net.abortAll();
    this.net.closeAllWebSockets();
    this._initialized = false;
    this.log.info('SsiRuntime disposed');
  }
}

/** SSI Component state enum */
export enum SSI_COMPONENT_STATE {
  UNINITIALIZED = 0,
  INITIALIZING,
  READY,
  RUNNING,
  SUSPENDED,
  ERROR,
  TERMINATED
}

/** SSI Component handle — returned by loadComponent() */
export interface SsiComponentHandle {
  id: string;
  module: any;
  exports: any;
  state: SSI_COMPONENT_STATE;
  initTime: number;
  memorySize: number;
  init(config?: any): Promise<void>;
  start(): Promise<void>;
  suspend(): Promise<void>;
  resume(): Promise<void>;
  stop(): Promise<void>;
}

/** Convenience default export */
export default SsiRuntime;
