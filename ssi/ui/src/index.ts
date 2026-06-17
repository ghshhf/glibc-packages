/**
 * SSI-UI: Window System Interface
 *
 * Implements the IWindowManager interface from SPEC-INTERFACE.md §4.
 * Provides window management, compositing (frame submission + VSync),
 * and input routing for SSI components.
 *
 * SSI Interfaces implemented:
 *   - SSI-UI: Window lifecycle, compositor, input routing
 */

import {
  SsiBaseComponent,
  SsiComponentConfig,
  SsiErrorCode,
  SsiRect,
  SsiInputEvent,
  uuidv4,
} from '../../core/src/index';

// =========================================================================
// SSI-UI Data Structures
// =========================================================================

export enum SsiWindowFlags {
  NORMAL = 0,
  FULLSCREEN = 1,
  BORDERLESS = 2,
  POPUP = 3,
  TRANSPARENT = 4,
  ALWAYS_ON_TOP = 5,
}

export interface SsiWindowConfig {
  title: string;
  rect: SsiRect;
  flags?: SsiWindowFlags;
  resizable?: boolean;
  transparent?: boolean;
  initialOpacity?: number;
  iconUrl?: string;
  minWidth?: number;
  minHeight?: number;
  maxWidth?: number;
  maxHeight?: number;
}

export interface SsiWindowInfo {
  id: string;
  rect: SsiRect;
  visible: boolean;
  focused: boolean;
  opacity: number;
  title: string;
  flags: SsiWindowFlags;
  frameCount: number;
  fps: number;
  zOrder: number;
  resizable: boolean;
}

export interface SsiBitmap {
  width: number;
  height: number;
  stride: number;
  pixels: Uint8Array;  // RGBA pixel data
  format: number;       // 0 = RGBA8
}

export interface SsiRenderLayer {
  rect: SsiRect;
  bitmap: SsiBitmap;
  opacity: number;      // 0-1
  zOrder: number;
  sourceUrl: string;
}

export interface SsiFrameCallback {
  (windowId: string, timestamp: number): void;
}

export enum SsiInputAction {
  DOWN = 0,
  UP = 1,
  MOVE = 2,
  SCROLL = 3,
}

export enum SsiInputType {
  MOUSE = 0,
  TOUCH = 1,
  KEYBOARD = 2,
  GAMEPAD = 3,
}

export { SsiInputEvent };

// =========================================================================
// Window Manager — SSI-UI Implementation
// =========================================================================

export class WindowManager extends SsiBaseComponent {
  private windows = new Map<string, WindowInfo>();
  private compositor = new Compositor();
  private zCounter = 0;
  private focusedWindowId: string | null = null;
  private vsyncCallbacks = new Map<string, Set<SsiFrameCallback>>();
  private compositorTimer: ReturnType<typeof setInterval> | null = null;

  constructor() {
    super({ name: 'window-manager', type: 'ui', version: '1.0.0', vendor: 'AI-TP' });
  }

  // =========================================================================
  // Window Lifecycle
  // =========================================================================

  /** Create a new window */
  createWindow(config: SsiWindowConfig): { windowId: string; error: SsiErrorCode } {
    const windowId = uuidv4();
    const defaultConfig: SsiWindowConfig = {
      title: config.title || 'Untitled',
      rect: config.rect || { x: 100, y: 100, width: 800, height: 600 },
      flags: config.flags ?? SsiWindowFlags.NORMAL,
      resizable: config.resizable ?? true,
      transparent: config.transparent ?? false,
      initialOpacity: config.initialOpacity ?? 1.0,
      iconUrl: config.iconUrl || '',
      minWidth: config.minWidth ?? 100,
      minHeight: config.minHeight ?? 100,
      maxWidth: config.maxWidth ?? 4096,
      maxHeight: config.maxHeight ?? 4096,
    };

    const info: WindowInfo = {
      id: windowId,
      title: defaultConfig.title,
      rect: { ...defaultConfig.rect },
      visible: true,
      focused: false,
      opacity: defaultConfig.initialOpacity!,
      flags: defaultConfig.flags!,
      frameCount: 0,
      fps: 0,
      zOrder: this.zCounter++,
      resizable: defaultConfig.resizable!,
      minWidth: defaultConfig.minWidth!,
      minHeight: defaultConfig.minHeight!,
      maxWidth: defaultConfig.maxWidth!,
      maxHeight: defaultConfig.maxHeight!,
      createdAt: Date.now(),
      lastFrameTime: 0,
      frameTimestamps: [],
    };

    this.windows.set(windowId, info);
    this.vsyncCallbacks.set(windowId, new Set());
    this.log(`Window created: "${info.title}" (${windowId.substring(0, 8)}...)`);

    // First window gets focus automatically
    if (this.windows.size === 1) {
      this.focusWindow(windowId);
    }

    return { windowId, error: SsiErrorCode.OK };
  }

  /** Close and destroy a window */
  destroyWindow(windowId: string): SsiErrorCode {
    const info = this.windows.get(windowId);
    if (!info) return SsiErrorCode.NOT_FOUND;

    this.windows.delete(windowId);
    this.compositor.removeWindow(windowId);
    this.vsyncCallbacks.delete(windowId);

    if (this.focusedWindowId === windowId) {
      // Focus next available window
      const remaining = Array.from(this.windows.keys());
      this.focusedWindowId = remaining.length > 0 ? remaining[0] : null;
      if (this.focusedWindowId) {
        this.windows.get(this.focusedWindowId)!.focused = true;
      }
    }

    this.log(`Window destroyed: "${info.title}"`);
    return SsiErrorCode.OK;
  }

  /** Set window visibility */
  setWindowVisible(windowId: string, visible: boolean): SsiErrorCode {
    const info = this.windows.get(windowId);
    if (!info) return SsiErrorCode.NOT_FOUND;
    info.visible = visible;
    if (!visible && this.focusedWindowId === windowId) {
      info.focused = false;
    }
    return SsiErrorCode.OK;
  }

  /** Set window position and size */
  setWindowRect(windowId: string, rect: SsiRect): SsiErrorCode {
    const info = this.windows.get(windowId);
    if (!info) return SsiErrorCode.NOT_FOUND;

    // Clamp to min/max dimensions
    info.rect = {
      x: rect.x ?? info.rect.x,
      y: rect.y ?? info.rect.y,
      width: Math.max(info.minWidth, Math.min(info.maxWidth, rect.width)),
      height: Math.max(info.minHeight, Math.min(info.maxHeight, rect.height)),
    };
    return SsiErrorCode.OK;
  }

  /** Get window information */
  getWindowInfo(windowId: string): SsiWindowInfo | null {
    const info = this.windows.get(windowId);
    if (!info) return null;

    return {
      id: info.id,
      rect: { ...info.rect },
      visible: info.visible,
      focused: info.focused,
      opacity: info.opacity,
      title: info.title,
      flags: info.flags,
      frameCount: info.frameCount,
      fps: info.fps,
      zOrder: info.zOrder,
      resizable: info.resizable,
    };
  }

  /** List all window IDs */
  listWindows(): SsiWindowInfo[] {
    return Array.from(this.windows.values()).map(info => ({
      id: info.id,
      rect: { ...info.rect },
      visible: info.visible,
      focused: info.focused,
      opacity: info.opacity,
      title: info.title,
      flags: info.flags,
      frameCount: info.frameCount,
      fps: info.fps,
      zOrder: info.zOrder,
      resizable: info.resizable,
    }));
  }

  /** Focus a specific window */
  focusWindow(windowId: string): SsiErrorCode {
    const info = this.windows.get(windowId);
    if (!info) return SsiErrorCode.NOT_FOUND;

    // Unfocus current
    if (this.focusedWindowId) {
      const current = this.windows.get(this.focusedWindowId);
      if (current) current.focused = false;
    }

    info.focused = true;
    info.zOrder = this.zCounter++;
    this.focusedWindowId = windowId;
    return SsiErrorCode.OK;
  }

  /** Set window opacity */
  setWindowOpacity(windowId: string, opacity: number): SsiErrorCode {
    const info = this.windows.get(windowId);
    if (!info) return SsiErrorCode.NOT_FOUND;
    info.opacity = Math.max(0, Math.min(1, opacity));
    return SsiErrorCode.OK;
  }

  // =========================================================================
  // Compositor
  // =========================================================================

  /** Submit a rendered frame for display */
  commitFrame(windowId: string, frame: SsiBitmap): SsiErrorCode {
    const info = this.windows.get(windowId);
    if (!info) return SsiErrorCode.NOT_FOUND;

    this.compositor.submitFrame(windowId, frame);
    info.frameCount++;

    // Calculate FPS
    const now = Date.now();
    info.frameTimestamps.push(now);
    // Keep last 60 frames
    while (info.frameTimestamps.length > 60) info.frameTimestamps.shift();
    if (info.frameTimestamps.length >= 2) {
      const elapsed = now - info.frameTimestamps[0];
      info.fps = elapsed > 0
        ? Math.round(((info.frameTimestamps.length - 1) / elapsed) * 1000)
        : 0;
    }

    info.lastFrameTime = now;
    return SsiErrorCode.OK;
  }

  /** Register a VSync frame callback */
  onFrame(windowId: string, callback: SsiFrameCallback): SsiErrorCode {
    const cbs = this.vsyncCallbacks.get(windowId);
    if (!cbs) return SsiErrorCode.NOT_FOUND;
    cbs.add(callback);
    return SsiErrorCode.OK;
  }

  /** Remove a frame callback */
  offFrame(windowId: string, callback: SsiFrameCallback): void {
    const cbs = this.vsyncCallbacks.get(windowId);
    if (cbs) {
      cbs.delete(callback);
    }
  }

  // =========================================================================
  // Input Routing
  // =========================================================================

  /** Route an input event to the target window */
  routeInput(event: SsiInputEvent): { targetWindowId: string | null; error: SsiErrorCode } {
    // Try focused window first
    if (this.focusedWindowId) {
      const focusedWindow = this.windows.get(this.focusedWindowId);
      if (focusedWindow && focusedWindow.visible) {
        this.log(`Input routed: type=${event.type} action=${event.action} → "${focusedWindow.title}"`);
        return { targetWindowId: this.focusedWindowId, error: SsiErrorCode.OK };
      }
    }

    // Fallback: find topmost visible window at coordinates
    const sortedWindows = Array.from(this.windows.values())
      .filter(w => w.visible && w.flags !== SsiWindowFlags.POPUP)
      .sort((a, b) => b.zOrder - a.zOrder);

    for (const win of sortedWindows) {
      if (this.pointInRect(event.x, event.y, win.rect)) {
        this.focusedWindowId = win.id;
        win.focused = true;
        return { targetWindowId: win.id, error: SsiErrorCode.OK };
      }
    }

    return { targetWindowId: null, error: SsiErrorCode.NOT_FOUND };
  }

  // =========================================================================
  // Lifecycle Hooks
  // =========================================================================

  protected async onInit(_config: SsiComponentConfig): Promise<void> {
    this.log('Window Manager initializing');
  }

  protected async onStart(): Promise<void> {
    // Start compositor tick (60 FPS target)
    this.compositorTimer = setInterval(() => {
      this.tickCompositor();
    }, 1000 / 60);
    this.log('Window Manager started (60 FPS compositor)');
  }

  protected async onStop(): Promise<void> {
    if (this.compositorTimer) {
      clearInterval(this.compositorTimer);
      this.compositorTimer = null;
    }
    this.windows.clear();
    this.vsyncCallbacks.clear();
    this.focusedWindowId = null;
    this.log('Window Manager stopped');
  }

  protected onDestroy(): void {
    this.windows.clear();
    this.vsyncCallbacks.clear();
    if (this.compositorTimer) {
      clearInterval(this.compositorTimer);
    }
  }

  // =========================================================================
  // Internal
  // =========================================================================

  private pointInRect(x: number, y: number, rect: SsiRect): boolean {
    return x >= rect.x && x < rect.x + rect.width &&
           y >= rect.y && y < rect.y + rect.height;
  }

  private tickCompositor(): void {
    const now = performance.now() * 1000; // microseconds
    // Fire VSync callbacks for all visible windows
    for (const [windowId, cbs] of this.vsyncCallbacks) {
      if (cbs.size === 0) continue;
      const info = this.windows.get(windowId);
      if (info && info.visible) {
        for (const cb of cbs) {
          try { cb(windowId, now); } catch { /* ignore */ }
        }
      }
    }
  }
}

// =========================================================================
// Compositor — Frame Buffer Management
// =========================================================================

class Compositor {
  private frameBuffers = new Map<string, SsiBitmap>();
  private compositeCache: ImageData | null = null;
  private cacheDirty = true;

  /** Submit a rendered frame for a window */
  submitFrame(windowId: string, frame: SsiBitmap): void {
    this.frameBuffers.set(windowId, frame);
    this.cacheDirty = true;
  }

  /** Remove a window's frame buffer */
  removeWindow(windowId: string): void {
    this.frameBuffers.delete(windowId);
    this.cacheDirty = true;
  }

  /** Get the most recent frame for a window */
  getFrame(windowId: string): SsiBitmap | undefined {
    return this.frameBuffers.get(windowId);
  }

  /** Get all active frame buffers */
  getAllFrames(): Map<string, SsiBitmap> {
    return new Map(this.frameBuffers);
  }

  /** Clear all buffers */
  clear(): void {
    this.frameBuffers.clear();
    this.compositeCache = null;
    this.cacheDirty = true;
  }
}

// =========================================================================
// Internal Types
// =========================================================================

interface WindowInfo {
  id: string;
  title: string;
  rect: SsiRect;
  visible: boolean;
  focused: boolean;
  opacity: number;
  flags: SsiWindowFlags;
  frameCount: number;
  fps: number;
  zOrder: number;
  resizable: boolean;
  minWidth: number;
  minHeight: number;
  maxWidth: number;
  maxHeight: number;
  createdAt: number;
  lastFrameTime: number;
  frameTimestamps: number[];
}
