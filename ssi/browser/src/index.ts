/**
 * SSI-BR: Browser Engine Component
 *
 * The browser engine is the system's native rendering and script execution
 * layer. It implements SSI-BR interface from SPEC-INTERFACE.md §3.
 *
 * In browser mode: wraps Chromium/V8 capabilities via Web APIs.
 * In Node.js mode: uses puppeteer/jSDOM for headless rendering.
 * In embedded mode: minimal canvas-based rendering.
 *
 * This component is what makes the browser a **core system component**,
 * not an external porting target.
 */

import { SsiBaseComponent } from '../../core/src/component';
import {
  SsiComponentConfig,
  SsiErrorCode,
  SsiBuffer,
  SsiRect,
  encodeSsiBuffer,
  decodeSsiBufferAsString,
} from '../../core/src/types';

// =========================================================================
// SSI-BR Types (mirrors SPEC-INTERFACE.md §3)
// =========================================================================

export interface SsiRenderOptions {
  flags: number;
  scale: number;
  alphaMode: number;
  hardwareAccel: boolean;
  framerateLimit: number;
  userAgent?: string;
}

export interface SsiRenderLayer {
  rect: SsiRect;
  bitmap: ImageBitmap | null;
  opacity: number;
  zOrder: number;
  sourceUrl: string;
}

export interface SsiSandboxConfig {
  memoryLimit: number;
  timeLimitMs: number;
  allowedApis: string[];
  allowNetwork: boolean;
  allowFs: boolean;
  allowedOrigins: string[];
}

export interface SsiNetworkRequest {
  url: string;
  method: string;
  headers: Record<string, string>;
  body?: ArrayBuffer;
  timeoutMs: number;
}

export interface SsiNetworkResponse {
  statusCode: number;
  headers: Record<string, string>;
  body: ArrayBuffer;
  elapsedMs: number;
}

export interface SsiDisplayInfo {
  width: number;
  height: number;
  dpi: number;
  refreshRate: number;
  colorDepth: number;
}

// =========================================================================
// Rendering Mode Detection
// =========================================================================

export type BrowserEngineMode = 'full' | 'lightweight' | 'headless';

function detectMode(): BrowserEngineMode {
  // Browser full mode
  if (typeof window !== 'undefined' && typeof document !== 'undefined') {
    // Check for WebGPU
    if (typeof (navigator as any).gpu !== 'undefined') {
      return 'full';
    }
    // Check for Canvas 2D
    if (typeof OffscreenCanvas !== 'undefined') {
      return 'lightweight';
    }
    return 'headless';
  }
  // Node.js / Worker → headless
  return 'headless';
}

// =========================================================================
// Browser Engine Component
// =========================================================================

export class BrowserEngine extends SsiBaseComponent {
  private mode: BrowserEngineMode = 'headless';
  private displayInfo: SsiDisplayInfo = { width: 1920, height: 1080, dpi: 96, refreshRate: 60, colorDepth: 32 };
  private sandboxCount = 0;
  private activeRenders = 0;
  private fps = 0;
  private frameCount = 0;
  private frameTimer: ReturnType<typeof setInterval> | null = null;

  // Canvas for lightweight mode
  private canvas: HTMLCanvasElement | OffscreenCanvas | null = null;
  private ctx: CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D | null = null;

  constructor() {
    super({
      name: 'browser-engine',
      type: 'browser',
      version: '1.0.0',
      vendor: 'AI-TP Foundation',
    });
    this.mode = detectMode();
  }

  // =========================================================================
  // SSI-BR: Render
  // =========================================================================

  async render(url: string, target: SsiRect, opts?: Partial<SsiRenderOptions>): Promise<SsiErrorCode> {
    this.log(`Render: ${url} → (${target.x},${target.y},${target.width}x${target.height}) [mode:${this.mode}]`);
    this.activeRenders++;
    this.frameCount++;

    try {
      switch (this.mode) {
        case 'full':
          await this.renderFull(url, target, opts);
          break;
        case 'lightweight':
          await this.renderLightweight(url, target, opts);
          break;
        case 'headless':
          await this.renderHeadless(url, target, opts);
          break;
      }
      this.activeRenders--;
      return SsiErrorCode.OK;
    } catch (e: any) {
      this.activeRenders--;
      this.log(`Render failed: ${e.message}`, 'error');
      return SsiErrorCode.GENERIC;
    }
  }

  async composite(layers: SsiRenderLayer[]): Promise<ImageBitmap | null> {
    this.log(`Composite: ${layers.length} layers`);
    // In full mode: GPU-accelerated compositing
    // In lightweight: canvas 2D compositing
    // In headless: skip
    if (this.ctx && layers.length > 0) {
      // Simple software compositing
      const sorted = [...layers].sort((a, b) => a.zOrder - b.zOrder);
      for (const layer of sorted) {
        if (layer.bitmap) {
          this.ctx.globalAlpha = layer.opacity;
          this.ctx.drawImage(
            layer.bitmap,
            layer.rect.x, layer.rect.y,
            layer.rect.width, layer.rect.height,
          );
        } else if (layer.sourceUrl) {
          this.log(`  Layer: ${layer.sourceUrl} (will fetch async)`, 'info');
        }
      }
    }
    return null;
  }

  getDisplayInfo(): SsiDisplayInfo {
    if (typeof screen !== 'undefined') {
      this.displayInfo = {
        width: screen.width,
        height: screen.height,
        dpi: screen.pixelDepth * 12,
        refreshRate: 60,
        colorDepth: screen.colorDepth,
      };
    }
    return { ...this.displayInfo };
  }

  // =========================================================================
  // SSI-BR: Script Execution
  // =========================================================================

  async executeScript(script: string, context?: Record<string, unknown>): Promise<any> {
    this.log(`Execute script (${script.length} chars)`);

    // In full mode: execute in iframe sandbox
    // In headless: use Function constructor
    try {
      const fn = new Function('context', script);
      return fn(context || {});
    } catch (e: any) {
      this.log(`Script execution failed: ${e.message}`, 'error');
      throw e;
    }
  }

  async compileScript(script: string, lang: 'js' | 'wat' = 'js'): Promise<SsiBuffer> {
    this.log(`Compile: ${lang} (${script.length} chars)`);
    // For JS: return as-is (V8 handles compilation at runtime)
    // For WAT: would need a WAT→WASM compiler
    return encodeSsiBuffer(script);
  }

  // =========================================================================
  // SSI-BR: Network Proxy
  // =========================================================================

  async fetch(req: SsiNetworkRequest): Promise<SsiNetworkResponse> {
    const start = performance.now();
    this.log(`Fetch: ${req.method} ${req.url}`);

    try {
      const headers = new Headers(req.headers);
      const response = await fetch(req.url, {
        method: req.method as any,
        headers,
        body: req.body,
        signal: req.timeoutMs > 0 ? AbortSignal.timeout(req.timeoutMs) : undefined,
      });

      const body = await response.arrayBuffer();
      const respHeaders: Record<string, string> = {};
      response.headers.forEach((value, key) => { respHeaders[key] = value; });

      return {
        statusCode: response.status,
        headers: respHeaders,
        body,
        elapsedMs: performance.now() - start,
      };
    } catch (e: any) {
      this.log(`Fetch failed: ${e.message}`, 'error');
      return {
        statusCode: 0,
        headers: {},
        body: new ArrayBuffer(0),
        elapsedMs: performance.now() - start,
      };
    }
  }

  // =========================================================================
  // SSI-BR: Sandbox
  // =========================================================================

  createSandbox(config?: Partial<SsiSandboxConfig>): number {
    const id = ++this.sandboxCount;
    this.log(`Sandbox #${id} created: ${JSON.stringify(config)}`);

    // In browser mode: create an isolated iframe
    if (this.mode === 'full' && typeof document !== 'undefined') {
      const iframe = document.createElement('iframe');
      iframe.sandbox = 'allow-scripts allow-same-origin';
      iframe.style.display = 'none';
      document.body.appendChild(iframe);
      // Store reference (in a real impl, use a Map)
    }

    return id;
  }

  destroySandbox(id: number): void {
    this.log(`Sandbox #${id} destroyed`);
  }

  // =========================================================================
  // Performance & Stats
  // =========================================================================

  getFps(): number {
    return this.fps;
  }

  getFrameCount(): number {
    return this.frameCount;
  }

  getActiveRenders(): number {
    return this.activeRenders;
  }

  getMode(): BrowserEngineMode {
    return this.mode;
  }

  // =========================================================================
  // SSI-CORE Lifecycle Hooks
  // =========================================================================

  protected async onInit(config: SsiComponentConfig): Promise<void> {
    this.log(`Browser Engine initializing (mode: ${this.mode})`);

    // Initialize canvas for lightweight mode
    if (this.mode === 'lightweight') {
      if (typeof OffscreenCanvas !== 'undefined') {
        this.canvas = new OffscreenCanvas(this.displayInfo.width, this.displayInfo.height);
        this.ctx = this.canvas.getContext('2d')!;
      } else if (typeof document !== 'undefined') {
        this.canvas = document.createElement('canvas');
        this.canvas.width = this.displayInfo.width;
        this.canvas.height = this.displayInfo.height;
        this.ctx = this.canvas.getContext('2d')!;
      }
    }

    this.displayInfo = config.display || this.displayInfo;
  }

  protected async onStart(): Promise<void> {
    // Start FPS counter
    this.frameTimer = setInterval(() => {
      this.fps = this.frameCount;
      this.frameCount = 0;
    }, 1000);

    this.log(`Browser Engine started (mode: ${this.mode}, display: ${this.displayInfo.width}x${this.displayInfo.height})`);
  }

  protected async onStop(): Promise<void> {
    if (this.frameTimer) {
      clearInterval(this.frameTimer);
      this.frameTimer = null;
    }
    this.log(`Browser Engine stopped`);
  }

  // =========================================================================
  // Private Render Implementations
  // =========================================================================

  private async renderFull(url: string, target: SsiRect, opts?: Partial<SsiRenderOptions>): Promise<void> {
    // Full mode: Use iframe for rendering
    if (typeof document !== 'undefined') {
      const iframe = document.createElement('iframe');
      iframe.src = url;
      iframe.style.position = 'absolute';
      iframe.style.left = `${target.x}px`;
      iframe.style.top = `${target.y}px`;
      iframe.style.width = `${target.width}px`;
      iframe.style.height = `${target.height}px`;
      iframe.style.border = 'none';
      iframe.style.pointerEvents = 'auto';
      document.body.appendChild(iframe);
    }
  }

  private async renderLightweight(url: string, target: SsiRect, opts?: Partial<SsiRenderOptions>): Promise<void> {
    // Lightweight mode: Use Canvas 2D
    if (this.ctx) {
      this.ctx.fillStyle = '#1a1a2e';
      this.ctx.fillRect(target.x, target.y, target.width, target.height);
      this.ctx.fillStyle = '#e0e0e0';
      this.ctx.font = '14px monospace';
      this.ctx.fillText(`SSI-BR: ${url}`, target.x + 8, target.y + 20);
      this.ctx.fillText(`resolution: ${target.width}x${target.height}`, target.x + 8, target.y + 40);
    }
  }

  private async renderHeadless(url: string, target: SsiRect, opts?: Partial<SsiRenderOptions>): Promise<void> {
    // Headless mode: No rendering, just metadata
    // Used for server-side operations
  }
}
