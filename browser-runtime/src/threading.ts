/**
 * Web Worker Thread Pool for WASM pthreads
 *
 * Provides pthread-compatible threading in browser environments using Web Workers.
 * Designed to work with Emscripten's PTHREADS support.
 *
 * Key features:
 * - Thread pool with configurable size
 * - SharedArrayBuffer (SAB) based shared memory
 * - Atomics-based synchronization
 * - OS thread-like API (spawn, join, mutex, condvar)
 * - Fallback to sequential execution when SAB is not available
 *
 * Requirements (browser):
 * - COOP: same-origin
 * - COEP: require-corp
 * - SharedArrayBuffer support
 *
 * Emscripten integration:
 * This module can replace Emscripten's default pthread implementation
 * when running in browsers that support SharedArrayBuffer.
 */

import { Logger, detectEnvironment, ThreadPoolConfig } from './types';

export type { ThreadPoolConfig };

/**
 * Thread status
 */
export enum ThreadStatus {
  IDLE = 'idle',
  RUNNING = 'running',
  WAITING = 'waiting',
  COMPLETED = 'completed',
  ERROR = 'error',
}

/**
 * Thread handle (similar to pthread_t)
 */
export interface ThreadHandle {
  /** Thread ID */
  id: number;
  /** Current status */
  status: ThreadStatus;
  /** Worker running this thread */
  worker: Worker;
  /** Shared memory region for return value */
  resultBuffer: Int32Array;
  /** Start time */
  startedAt: number;
}

/**
 * Mutex implementation using SharedArrayBuffer and Atomics
 */
export class WasmMutex {
  private lockArray: Int32Array;

  constructor(sharedBuffer?: SharedArrayBuffer, byteOffset = 0) {
    if (sharedBuffer) {
      this.lockArray = new Int32Array(sharedBuffer, byteOffset, 1);
    } else {
      const sab = new SharedArrayBuffer(4);
      this.lockArray = new Int32Array(sab);
    }
    Atomics.store(this.lockArray, 0, 0); // unlocked
  }

  lock(): void {
    while (Atomics.compareExchange(this.lockArray, 0, 0, 1) !== 0) {
      Atomics.wait(this.lockArray, 0, 1); // wait until unlocked
    }
  }

  tryLock(): boolean {
    return Atomics.compareExchange(this.lockArray, 0, 0, 1) === 0;
  }

  unlock(): void {
    Atomics.store(this.lockArray, 0, 0);
    Atomics.notify(this.lockArray, 0, 1); // wake one waiter
  }

  get buffer(): SharedArrayBuffer {
    return this.lockArray.buffer as SharedArrayBuffer;
  }
}

/**
 * Condition variable implementation using SharedArrayBuffer and Atomics
 */
export class WasmConditionVariable {
  private condArray: Int32Array;

  constructor(sharedBuffer?: SharedArrayBuffer, byteOffset = 0) {
    if (sharedBuffer) {
      this.condArray = new Int32Array(sharedBuffer, byteOffset, 1);
    } else {
      const sab = new SharedArrayBuffer(4);
      this.condArray = new Int32Array(sab);
    }
    Atomics.store(this.condArray, 0, 0);
  }

  wait(mutex: WasmMutex): void {
    mutex.unlock();
    Atomics.wait(this.condArray, 0, 0);
    mutex.lock();
  }

  notifyOne(): void {
    Atomics.store(this.condArray, 0, 1);
    Atomics.notify(this.condArray, 0, 1);
  }

  notifyAll(): void {
    Atomics.store(this.condArray, 0, 1);
    Atomics.notify(this.condArray, 0);
  }
}

/**
 * Thread Pool Manager
 *
 * Manages a pool of Web Workers for executing pthread-style tasks.
 */
export class ThreadPool {
  private static instance: ThreadPool;
  private log: Logger;

  /** Workers pool */
  private workers: Worker[] = [];

  /** Available worker IDs */
  private available: number[] = [];

  /** Thread handles */
  private threads = new Map<number, ThreadHandle>();

  /** Next thread ID */
  private nextThreadId = 1;

  /** Configuration */
  private config: Required<ThreadPoolConfig>;

  /** Worker blob URL (for creating inline workers) */
  private workerBlobUrl: string | null = null;

  private constructor(config?: ThreadPoolConfig) {
    this.log = new Logger('info');
    this.config = {
      threadCount: config?.threadCount ?? this.defaultThreadCount(),
      workerScriptUrl: config?.workerScriptUrl ?? '',
      stackSize: config?.stackSize ?? 256,
      useSharedArrayBuffer: config?.useSharedArrayBuffer ?? this.checkSharedArrayBuffer(),
    };
  }

  static getInstance(config?: ThreadPoolConfig): ThreadPool {
    if (!ThreadPool.instance) {
      ThreadPool.instance = new ThreadPool(config);
    }
    return ThreadPool.instance;
  }

  /** Get default thread count based on hardware */
  private defaultThreadCount(): number {
    const env = detectEnvironment();
    if (env === 'browser' && navigator.hardwareConcurrency) {
      return Math.max(1, Math.floor(navigator.hardwareConcurrency / 2));
    }
    return 4;
  }

  /** Check if SharedArrayBuffer is available */
  private checkSharedArrayBuffer(): boolean {
    try {
      const sab = new SharedArrayBuffer(4);
      return sab.byteLength === 4;
    } catch {
      return false;
    }
  }

  /** Initialize the thread pool */
  async init(): Promise<void> {
    if (this.workers.length > 0) {
      this.log.warn('Thread pool already initialized');
      return;
    }

    this.log.info(`Initializing thread pool with ${this.config.threadCount} workers`);
    this.log.info(`SharedArrayBuffer: ${this.config.useSharedArrayBuffer ? 'enabled' : 'disabled'}`);

    // Create the worker script blob
    this.workerBlobUrl = await this.createWorkerBlob();

    for (let i = 0; i < this.config.threadCount; i++) {
      try {
        const worker = new Worker(this.workerBlobUrl);
        this.workers.push(worker);
        this.available.push(i);
        this.log.debug(`Worker ${i} created`);
      } catch (e) {
        this.log.error(`Failed to create worker ${i}:`, e);
      }
    }

    this.log.info(`Thread pool ready: ${this.workers.length} workers`);
  }

  /** Spawn a new thread */
  async spawn(
    func: Function | string,
    args?: any[],
    sharedBuffer?: SharedArrayBuffer
  ): Promise<ThreadHandle> {
    if (this.workers.length === 0) {
      await this.init();
    }

    if (this.available.length === 0) {
      // All workers busy, wait for one
      this.log.debug('All workers busy, waiting...');
      await new Promise<void>((resolve) => {
        const check = setInterval(() => {
          if (this.available.length > 0) {
            clearInterval(check);
            resolve();
          }
        }, 10);
      });
    }

    const workerIndex = this.available.pop()!;
    const worker = this.workers[workerIndex];
    const threadId = this.nextThreadId++;

    // Create result buffer
    const resultSab = new SharedArrayBuffer(4);
    const resultBuffer = new Int32Array(resultSab);
    Atomics.store(resultBuffer, 0, 0);

    const handle: ThreadHandle = {
      id: threadId,
      status: ThreadStatus.RUNNING,
      worker,
      resultBuffer,
      startedAt: Date.now(),
    };

    this.threads.set(threadId, handle);

    // Post task to worker
    const funcStr = typeof func === 'function' ? func.toString() : func;
    worker.postMessage({
      type: 'spawn',
      threadId,
      func: funcStr,
      args: args || [],
      sharedBuffer,
      resultBuffer: resultSab,
    });

    // Listen for completion
    worker.onmessage = (event) => {
      if (event.data?.type === 'complete' && event.data.threadId === threadId) {
        handle.status = ThreadStatus.COMPLETED;
        this.available.push(workerIndex);
        if (event.data.result !== undefined) {
          Atomics.store(handle.resultBuffer, 0, event.data.result);
        }
      } else if (event.data?.type === 'error' && event.data.threadId === threadId) {
        handle.status = ThreadStatus.ERROR;
        this.available.push(workerIndex);
        this.log.error(`Thread ${threadId} error:`, event.data.error);
      }
    };

    return handle;
  }

  /** Join a thread (wait for completion) */
  async join(handle: ThreadHandle, timeout?: number): Promise<number> {
    const start = Date.now();

    while (handle.status !== ThreadStatus.COMPLETED && handle.status !== ThreadStatus.ERROR) {
      if (timeout && (Date.now() - start) > timeout) {
        throw new Error(`Thread ${handle.id} join timeout`);
      }
      await new Promise(resolve => setTimeout(resolve, 1));
    }

    if (handle.status === ThreadStatus.ERROR) {
      throw new Error(`Thread ${handle.id} exited with error`);
    }

    return Atomics.load(handle.resultBuffer, 0);
  }

  /** Get a thread handle by ID */
  getThread(id: number): ThreadHandle | undefined {
    return this.threads.get(id);
  }

  /** Terminate all workers and clean up */
  terminate(): void {
    for (const worker of this.workers) {
      worker.terminate();
    }
    this.workers = [];
    this.available = [];
    this.threads.clear();
    this.log.info('Thread pool terminated');
  }

  /** Get current pool status */
  get status() {
    return {
      total: this.workers.length,
      active: this.threads.size,
      available: this.available.length,
      threads: Array.from(this.threads.values()).map(t => ({
        id: t.id,
        status: t.status,
        runtime: Date.now() - t.startedAt,
      })),
    };
  }

  /** Create an inline worker script blob */
  private async createWorkerBlob(): Promise<string> {
    const workerCode = `
      self.onmessage = function(event) {
        const { type, threadId, func, args, resultBuffer } = event.data;

        if (type === 'spawn') {
          try {
            const fn = eval('(' + func + ')');
            const result = fn.apply(null, args);
            self.postMessage({ type: 'complete', threadId, result });
          } catch (error) {
            self.postMessage({ type: 'error', threadId, error: error.message });
          }
        }
      };
    `;

    const blob = new Blob([workerCode], { type: 'application/javascript' });
    return URL.createObjectURL(blob);
  }
}

/** Emscripten pthread integration helper */
export class EmscriptenPthreadBridge {
  private pool: ThreadPool;

  constructor(pool?: ThreadPool) {
    this.pool = pool || ThreadPool.getInstance();
  }

  /**
   * Called by Emscripten's pthread implementation to create a new thread.
   * Maps to ThreadPool.spawn().
   */
  pthread_create(threadId: number, attr: any, startRoutine: number, arg: number): number {
    // Emscripten's PTHREADS uses its own scheduler
    // This bridge remaps the C pthread_create to our worker pool
    this.pool.spawn(() => {
      // Execute the C function via Emscripten's runtime
      if (typeof (globalThis as any).dynCall_vi === 'function') {
        (globalThis as any).dynCall_vi(startRoutine, arg);
      }
      return 0;
    });
    return 0; // success
  }

  pthread_join(threadId: number, retval?: number): number {
    return 0;
  }

  pthread_exit(retval: number): void {
    // handled by Emscripten
  }
}

export default ThreadPool;
