/**
 * SSI-CORE: Base Component
 *
 * Reference implementation of the ISsiComponent interface.
 * All system components should extend this class.
 *
 * Usage:
 * ```typescript
 * class BrowserEngine extends SsiBaseComponent {
 *   constructor() {
 *     super({
 *       name: 'browser-engine',
 *       type: 'browser',
 *       version: '1.0.0',
 *       vendor: 'AI-TP Foundation',
 *     });
 *   }
 *   async onInit(config) { /* setup * / }
 *   async onStart() { /* begin work * / }
 * }
 * ```
 */

import {
  ISsiComponent,
  SsiComponentId,
  SsiComponentConfig,
  SsiComponentState,
  SsiComponentStatus,
  SsiErrorCode,
  SsiEvent,
  SsiEventCallback,
  SsiMessage,
  UUID,
  uuidv4,
  timestampUs,
} from './types';

export abstract class SsiBaseComponent implements ISsiComponent {
  public readonly id: SsiComponentId;
  protected state: SsiComponentState = SsiComponentState.UNINITIALIZED;
  protected startedAt: number = 0;
  protected errorCount: number = 0;
  protected memoryUsed: number = 0;
  protected memoryMax: number = 512 * 1024 * 1024; // 512MB default
  protected config: SsiComponentConfig = {};

  private eventListeners = new Map<number, Set<SsiEventCallback>>();
  private messageQueue: SsiMessage[] = [];
  private messageResolvers: Array<(msg: SsiMessage) => void> = [];

  constructor(id: Partial<SsiComponentId>) {
    this.id = {
      uuid: id.uuid || uuidv4(),
      name: id.name || 'unnamed',
      version: id.version || '0.0.0',
      type: id.type || 'generic',
      apiVersion: id.apiVersion || 1,
      vendor: id.vendor || 'unknown',
    };
  }

  // =========================================================================
  // ISsiComponent Lifecycle
  // =========================================================================

  async init(config?: SsiComponentConfig): Promise<SsiErrorCode> {
    if (this.state !== SsiComponentState.UNINITIALIZED && this.state !== SsiComponentState.ERROR) {
      return SsiErrorCode.BUSY;
    }
    this.state = SsiComponentState.INITIALIZING;
    this.config = config || {};

    try {
      await this.onInit(this.config);
      this.state = SsiComponentState.READY;
      this.log(`Init complete (${this.id.name} v${this.id.version})`);
      return SsiErrorCode.OK;
    } catch (e: any) {
      this.state = SsiComponentState.ERROR;
      this.errorCount++;
      this.log(`Init failed: ${e.message}`, 'error');
      return SsiErrorCode.GENERIC;
    }
  }

  async start(): Promise<SsiErrorCode> {
    if (this.state !== SsiComponentState.READY) {
      return SsiErrorCode.BUSY;
    }
    this.state = SsiComponentState.RUNNING;
    this.startedAt = Date.now();

    try {
      await this.onStart();
      this.log(`Start complete`);
      return SsiErrorCode.OK;
    } catch (e: any) {
      this.state = SsiComponentState.ERROR;
      this.errorCount++;
      this.log(`Start failed: ${e.message}`, 'error');
      return SsiErrorCode.GENERIC;
    }
  }

  async suspend(): Promise<SsiErrorCode> {
    if (this.state !== SsiComponentState.RUNNING) {
      return SsiErrorCode.BUSY;
    }
    try {
      await this.onSuspend();
      this.state = SsiComponentState.SUSPENDED;
      return SsiErrorCode.OK;
    } catch (e: any) {
      this.log(`Suspend failed: ${e}`, 'warn');
      return SsiErrorCode.GENERIC;
    }
  }

  async resume(): Promise<SsiErrorCode> {
    if (this.state !== SsiComponentState.SUSPENDED) {
      return SsiErrorCode.BUSY;
    }
    try {
      await this.onResume();
      this.state = SsiComponentState.RUNNING;
      return SsiErrorCode.OK;
    } catch (e: any) {
      this.log(`Resume failed: ${e}`, 'warn');
      return SsiErrorCode.GENERIC;
    }
  }

  async stop(): Promise<SsiErrorCode> {
    if (this.state !== SsiComponentState.RUNNING && this.state !== SsiComponentState.SUSPENDED) {
      return SsiErrorCode.BUSY;
    }
    try {
      await this.onStop();
      this.state = SsiComponentState.TERMINATED;
      this.log(`Stop complete (uptime: ${((Date.now() - this.startedAt) / 1000).toFixed(1)}s)`);
      return SsiErrorCode.OK;
    } catch (e: any) {
      this.log(`Stop failed: ${e}`, 'warn');
      return SsiErrorCode.GENERIC;
    }
  }

  destroy(): void {
    this.state = SsiComponentState.TERMINATED;
    this.messageQueue = [];
    this.messageResolvers = [];
    this.eventListeners.clear();
    this.onDestroy();
  }

  // =========================================================================
  // Queries
  // =========================================================================

  getState(): SsiComponentState {
    return this.state;
  }

  getId(): SsiComponentId {
    return { ...this.id };
  }

  getStatus(): SsiComponentStatus {
    return {
      state: this.state,
      uptimeMs: this.startedAt ? Date.now() - this.startedAt : 0,
      memoryUsed: this.memoryUsed,
      memoryMax: this.memoryMax,
      errorCount: this.errorCount,
      cpuUsage: 0,
    };
  }

  // =========================================================================
  // Communication
  // =========================================================================

  async sendMessage(msg: SsiMessage): Promise<SsiErrorCode> {
    // Default: queue locally (override in subclass for bus integration)
    this.messageQueue.push(msg);
    // Resolve any pending receive
    const resolver = this.messageResolvers.shift();
    if (resolver) {
      resolver(msg);
    }
    return SsiErrorCode.OK;
  }

  async receiveMessage(timeoutMs: number = 0): Promise<SsiMessage | null> {
    const msg = this.messageQueue.shift();
    if (msg) return msg;
    if (timeoutMs === 0) return null;

    return new Promise((resolve) => {
      const timer = timeoutMs > 0
        ? setTimeout(() => {
            const idx = this.messageResolvers.indexOf(resolve);
            if (idx >= 0) this.messageResolvers.splice(idx, 1);
            resolve(null);
          }, timeoutMs)
        : null;

      this.messageResolvers.push((m: SsiMessage) => {
        if (timer) clearTimeout(timer);
        resolve(m);
      });
    });
  }

  // =========================================================================
  // Events
  // =========================================================================

  onEvent(eventType: number, callback: SsiEventCallback): void {
    if (!this.eventListeners.has(eventType)) {
      this.eventListeners.set(eventType, new Set());
    }
    this.eventListeners.get(eventType)!.add(callback);
  }

  async emitEvent(event: SsiEvent): Promise<SsiErrorCode> {
    const listeners = this.eventListeners.get(event.type);
    if (listeners) {
      for (const cb of listeners) {
        try { cb(event); } catch { /* ignore */ }
      }
    }
    return SsiErrorCode.OK;
  }

  // =========================================================================
  // Hooks (override in subclass)
  // =========================================================================

  protected async onInit(_config: SsiComponentConfig): Promise<void> { /* noop */ }
  protected async onStart(): Promise<void> { /* noop */ }
  protected async onSuspend(): Promise<void> { /* noop */ }
  protected async onResume(): Promise<void> { /* noop */ }
  protected async onStop(): Promise<void> { /* noop */ }
  protected onDestroy(): void { /* noop */ }

  // =========================================================================
  // Utilities
  // =========================================================================

  protected log(msg: string, level: 'info' | 'warn' | 'error' = 'info'): void {
    const prefix = `[${this.id.type}:${this.id.name}]`;
    const fn = level === 'error' ? console.error : level === 'warn' ? console.warn : console.log;
    fn(prefix, msg);
  }

  /** Track memory allocation */
  protected trackMemory(bytes: number): void {
    this.memoryUsed += bytes;
  }

  /** Create a standard SSI message */
  protected createMessage(targetUuid: UUID, payload: ArrayBuffer, type: number = 0, priority: number = 50): SsiMessage {
    return {
      type,
      id: Math.floor(Math.random() * Number.MAX_SAFE_INTEGER),
      timestamp: timestampUs(),
      sourceUuid: this.id.uuid,
      targetUuid,
      payload,
      priority,
    };
  }
}
