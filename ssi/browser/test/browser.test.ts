/**
 * SSI-BR: Browser Engine Tests
 *
 * Tests for the browser engine implementation running in headless mode (Node.js).
 */

import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { BrowserEngine, SsiErrorCode } from '../src/index';

describe('BrowserEngine (SSI-BR)', () => {
  let engine: BrowserEngine;

  beforeEach(async () => {
    engine = new BrowserEngine();
    await engine.init({ display: { width: 800, height: 600, refreshRate: 30 } });
    await engine.start();
  });

  afterEach(async () => {
    await engine.stop();
    engine.destroy();
  });

  // ── Mode Detection ──

  it('should detect headless mode in Node.js', () => {
    const mode = engine.getMode();
    expect(mode).toBe('headless');
  });

  // ── Lifecycle ──

  it('should have correct component ID', () => {
    expect(engine.id.name).toBe('browser-engine');
    expect(engine.id.type).toBe('browser');
    expect(engine.id.version).toBe('1.0.0');
  });

  it('should return OK status after init and start', () => {
    expect(engine.getState()).toBeGreaterThanOrEqual(1); // >= INITIALIZING
  });

  // ── Display Info ──

  it('should return display info', () => {
    const info = engine.getDisplayInfo();
    expect(info.width).toBe(800);
    expect(info.height).toBe(600);
    expect(info.refreshRate).toBe(30);
    expect(info.colorDepth).toBeGreaterThan(0);
  });

  // ── Render ──

  it('should render headless (returns OK)', async () => {
    const err = await engine.render('about:blank', { x: 0, y: 0, width: 100, height: 100 });
    expect(err).toBe(SsiErrorCode.OK);
  });

  it('should increment frame counter on render', async () => {
    await engine.render('test://frame1', { x: 0, y: 0, width: 100, height: 100 });
    await engine.render('test://frame2', { x: 0, y: 0, width: 100, height: 100 });
    const frames = engine.getFrameCount();
    expect(frames).toBeGreaterThanOrEqual(2);
  });

  it('should track active renders', async () => {
    const err = await engine.render('test://op', { x: 0, y: 0, width: 50, height: 50 });
    expect(err).toBe(SsiErrorCode.OK);
    expect(engine.getActiveRenders()).toBe(0); // already finished
  });

  // ── Script Execution ──

  it('should execute simple script and return result', async () => {
    const result = await engine.executeScript('return context.a + context.b', { a: 3, b: 4 });
    expect(result).toBe(7);
  });

  it('should execute script with no context', async () => {
    const result = await engine.executeScript('return 42');
    expect(result).toBe(42);
  });

  it('should execute string manipulation', async () => {
    const result = await engine.executeScript('return context.s.toUpperCase()', { s: 'hello' });
    expect(result).toBe('HELLO');
  });

  it('should throw on invalid script', async () => {
    await expect(engine.executeScript('invalid syntax {{{')).rejects.toThrow();
  });

  // ── Script Compilation ──

  it('should compile JS script', async () => {
    const result = await engine.compileScript('console.log("hello")', 'js');
    const text = new TextDecoder().decode(result.data);
    expect(text).toBe('console.log("hello")');
  });

  // ── Sandbox ──

  it('should create and destroy sandboxes', () => {
    const id1 = engine.createSandbox({ memoryLimit: 64 * 1024 * 1024, allowNetwork: false });
    const id2 = engine.createSandbox();
    expect(id1).toBe(1);
    expect(id2).toBe(2);

    engine.destroySandbox(id1);
    engine.destroySandbox(id2);
    // Should not throw
  });

  it('should create sandboxes with various configs', () => {
    const id = engine.createSandbox({
      memoryLimit: 128 * 1024 * 1024,
      timeLimitMs: 5000,
      allowedApis: ['fetch', 'console'],
      allowNetwork: true,
      allowFs: false,
      allowedOrigins: ['https://example.com'],
    });
    expect(id).toBeGreaterThan(0);
    engine.destroySandbox(id);
  });
});

describe('BrowserEngine Mode Detection', () => {
  it('should initialize without config', async () => {
    const e = new BrowserEngine();
    await e.init({});
    await e.start();
    expect(e.getMode()).toBe('headless');
    await e.stop();
    e.destroy();
  });

  it('should be usable immediately after construction', () => {
    const e = new BrowserEngine();
    expect(e.getMode()).toBe('headless');
    expect(e.getFps()).toBe(0);
    expect(e.getFrameCount()).toBe(0);
    expect(e.getActiveRenders()).toBe(0);
  });
});
