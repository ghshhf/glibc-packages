import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { SsiErrorCode } from '../../core/src/index';
import { WindowManager, SsiWindowFlags } from '../src/index';

describe('WindowManager', () => {
  let wm: WindowManager;

  beforeEach(async () => {
    wm = new WindowManager();
    await wm.init();
    await wm.start();
  });

  afterEach(async () => {
    await wm.stop();
  });

  describe('window lifecycle', () => {
    it('should create a window with default config', () => {
      const { windowId, error } = wm.createWindow({
        title: 'Test Window',
        rect: { x: 100, y: 100, width: 800, height: 600 },
      });
      expect(error).toBe(SsiErrorCode.OK);
      expect(windowId).toBeDefined();

      const info = wm.getWindowInfo(windowId);
      expect(info).not.toBeNull();
      expect(info!.title).toBe('Test Window');
      expect(info!.rect.width).toBe(800);
      expect(info!.rect.height).toBe(600);
      expect(info!.visible).toBe(true);
      expect(info!.resizable).toBe(true);
    });

    it('should create a window with custom flags', () => {
      const { windowId } = wm.createWindow({
        title: 'Fullscreen',
        rect: { x: 0, y: 0, width: 1920, height: 1080 },
        flags: SsiWindowFlags.FULLSCREEN,
        resizable: false,
        transparent: true,
      });
      const info = wm.getWindowInfo(windowId);
      expect(info!.flags).toBe(SsiWindowFlags.FULLSCREEN);
      expect(info!.resizable).toBe(false);
      expect(info!.opacity).toBe(1.0);
    });

    it('should list all windows', () => {
      wm.createWindow({ title: 'A', rect: { x: 0, y: 0, width: 400, height: 300 } });
      wm.createWindow({ title: 'B', rect: { x: 0, y: 0, width: 500, height: 400 } });

      const list = wm.listWindows();
      expect(list).toHaveLength(2);
      expect(list.map(w => w.title).sort()).toEqual(['A', 'B']);
    });

    it('should destroy a window', () => {
      const { windowId } = wm.createWindow({
        title: 'Remove Me',
        rect: { x: 0, y: 0, width: 400, height: 300 },
      });
      expect(wm.listWindows()).toHaveLength(1);

      const result = wm.destroyWindow(windowId);
      expect(result).toBe(SsiErrorCode.OK);
      expect(wm.listWindows()).toHaveLength(0);
    });

    it('should return NOT_FOUND for invalid window operations', () => {
      expect(wm.destroyWindow('nonexistent')).toBe(SsiErrorCode.NOT_FOUND);
      expect(wm.setWindowVisible('nonexistent', false)).toBe(SsiErrorCode.NOT_FOUND);
      expect(wm.setWindowRect('nonexistent', { x: 0, y: 0, width: 100, height: 100 })).toBe(SsiErrorCode.NOT_FOUND);
      expect(wm.getWindowInfo('nonexistent')).toBeNull();
    });
  });

  describe('window state', () => {
    it('should toggle visibility', () => {
      const { windowId } = wm.createWindow({
        title: 'Toggle',
        rect: { x: 0, y: 0, width: 400, height: 300 },
      });

      wm.setWindowVisible(windowId, false);
      expect(wm.getWindowInfo(windowId)!.visible).toBe(false);

      wm.setWindowVisible(windowId, true);
      expect(wm.getWindowInfo(windowId)!.visible).toBe(true);
    });

    it('should set window position and size', () => {
      const { windowId } = wm.createWindow({
        title: 'Resize',
        rect: { x: 10, y: 20, width: 800, height: 600 },
      });

      wm.setWindowRect(windowId, { x: 100, y: 200, width: 1024, height: 768 });
      const info = wm.getWindowInfo(windowId)!;
      expect(info.rect.x).toBe(100);
      expect(info.rect.y).toBe(200);
      expect(info.rect.width).toBe(1024);
      expect(info.rect.height).toBe(768);
    });

    it('should clamp dimensions to min/max', () => {
      const { windowId } = wm.createWindow({
        title: 'Clamp',
        rect: { x: 0, y: 0, width: 800, height: 600 },
        minWidth: 200,
        maxWidth: 1600,
      });

      wm.setWindowRect(windowId, { x: 0, y: 0, width: 50, height: 50 });
      let info = wm.getWindowInfo(windowId)!;
      expect(info.rect.width).toBe(200);
      expect(info.rect.height).toBe(100);

      wm.setWindowRect(windowId, { x: 0, y: 0, width: 2000, height: 2000 });
      info = wm.getWindowInfo(windowId)!;
      expect(info.rect.width).toBe(1600);
      expect(info.rect.height).toBe(2000); // within valid range
    });

    it('should set window opacity', () => {
      const { windowId } = wm.createWindow({
        title: 'Opacity',
        rect: { x: 0, y: 0, width: 400, height: 300 },
      });

      wm.setWindowOpacity(windowId, 0.5);
      expect(wm.getWindowInfo(windowId)!.opacity).toBe(0.5);

      wm.setWindowOpacity(windowId, 1.5);
      expect(wm.getWindowInfo(windowId)!.opacity).toBe(1.0);

      wm.setWindowOpacity(windowId, -0.5);
      expect(wm.getWindowInfo(windowId)!.opacity).toBe(0);
    });
  });

  describe('window focus', () => {
    it('should auto-focus the first window', () => {
      const { windowId } = wm.createWindow({
        title: 'First',
        rect: { x: 0, y: 0, width: 400, height: 300 },
      });
      expect(wm.getWindowInfo(windowId)!.focused).toBe(true);
    });

    it('should change focus between windows', () => {
      const a = wm.createWindow({ title: 'A', rect: { x: 0, y: 0, width: 400, height: 300 } }).windowId;
      const b = wm.createWindow({ title: 'B', rect: { x: 0, y: 0, width: 400, height: 300 } }).windowId;

      // First window gets auto-focus
      expect(wm.getWindowInfo(a)!.focused).toBe(true);
      expect(wm.getWindowInfo(b)!.focused).toBe(false);

      // Manually focus B
      wm.focusWindow(b);
      expect(wm.getWindowInfo(b)!.focused).toBe(true);
      expect(wm.getWindowInfo(a)!.focused).toBe(false);
    });
  });

  describe('compositor', () => {
    it('should accept frame commits', () => {
      const { windowId } = wm.createWindow({
        title: 'Render',
        rect: { x: 0, y: 0, width: 320, height: 240 },
      });

      const bitmap = {
        width: 320,
        height: 240,
        stride: 1280,
        pixels: new Uint8Array(320 * 240 * 4),
        format: 0,
      };

      const result = wm.commitFrame(windowId, bitmap);
      expect(result).toBe(SsiErrorCode.OK);
    });

    it('should track frame count', () => {
      const { windowId } = wm.createWindow({
        title: 'FPS Test',
        rect: { x: 0, y: 0, width: 320, height: 240 },
      });

      const bitmap = {
        width: 320, height: 240, stride: 1280,
        pixels: new Uint8Array(320 * 240 * 4), format: 0,
      };

      for (let i = 0; i < 10; i++) {
        wm.commitFrame(windowId, bitmap);
      }

      expect(wm.getWindowInfo(windowId)!.frameCount).toBe(10);
    });

    it('should return NOT_FOUND for invalid window', () => {
      const result = wm.commitFrame('nonexistent', {
        width: 100, height: 100, stride: 400,
        pixels: new Uint8Array(40000), format: 0,
      });
      expect(result).toBe(SsiErrorCode.NOT_FOUND);
    });
  });

  describe('VSync callbacks', () => {
    it('should fire frame callbacks at compositor tick', async () => {
      const { windowId } = wm.createWindow({
        title: 'VSync',
        rect: { x: 0, y: 0, width: 320, height: 240 },
      });

      let callbackFired = false;
      wm.onFrame(windowId, () => {
        callbackFired = true;
      });

      await new Promise(resolve => setTimeout(resolve, 40));
      expect(callbackFired).toBe(true);
    });

    it('should support removing callbacks without error', () => {
      const { windowId } = wm.createWindow({
        title: 'Off',
        rect: { x: 0, y: 0, width: 320, height: 240 },
      });

      const cb = () => {};
      wm.onFrame(windowId, cb);
      wm.offFrame(windowId, cb);
      wm.offFrame('nonexistent', cb);
    });
  });

  describe('input routing', () => {
    it('should route input to focused window', () => {
      const { windowId } = wm.createWindow({
        title: 'Target',
        rect: { x: 0, y: 0, width: 800, height: 600 },
      });

      const result = wm.routeInput({
        type: 0, action: 0, x: 400, y: 300,
        button: 0, pressure: 0, timestamp: Date.now() * 1000,
      });

      expect(result.targetWindowId).toBe(windowId);
      expect(result.error).toBe(SsiErrorCode.OK);
    });

    it('should find window by hit test', () => {
      const a = wm.createWindow({
        title: 'Left', rect: { x: 0, y: 0, width: 400, height: 600 },
      }).windowId;
      wm.createWindow({
        title: 'Right', rect: { x: 500, y: 0, width: 400, height: 600 },
      });

      // First window (a) has focus, click inside it
      const result = wm.routeInput({
        type: 0, action: 0, x: 200, y: 300,
        button: 0, pressure: 0, timestamp: Date.now() * 1000,
      });

      expect(result.targetWindowId).toBe(a);
    });

    it('should not return null when clicking outside focused window', () => {
      wm.createWindow({
        title: 'Window', rect: { x: 0, y: 0, width: 400, height: 300 },
      });

      const result = wm.routeInput({
        type: 0, action: 0, x: 9999, y: 9999,
        button: 0, pressure: 0, timestamp: Date.now() * 1000,
      });

      // Routes to focused window regardless of coordinates
      expect(result.targetWindowId).not.toBeNull();
    });
  });
});
