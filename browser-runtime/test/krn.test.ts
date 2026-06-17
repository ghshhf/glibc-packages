/**
 * SSI-KRN: WASM Runtime Kernel Tests
 *
 * Tests for the SsiRuntime kernel (browser-runtime module).
 */

import { describe, it, expect, afterEach } from 'vitest';
import SsiRuntime, { VirtualFS } from '../src/index';
import { detectEnvironment, formatBytes } from '../src/types';

describe('SsiRuntime (SSI-KRN)', () => {
  // Clean up after each test
  afterEach(() => {
    try {
      const runtime = SsiRuntime.getInstance();
      runtime.dispose();
    } catch {
      // Not initialized, skip
    }
  });

  // ── Init & Singleton ──

  it('should initialize the runtime kernel', () => {
    const runtime = SsiRuntime.init({ logLevel: 'info' });
    expect(runtime).toBeDefined();
    expect(runtime.isInitialized).toBe(true);
  });

  it('should return the same instance on repeated init', () => {
    const r1 = SsiRuntime.init({ logLevel: 'info' });
    const r2 = SsiRuntime.init({ logLevel: 'debug' }); // singleton: config ignored after first call
    expect(r1).toBe(r2);
  });

  it('should accept default config', () => {
    const runtime = SsiRuntime.init({});
    expect(runtime.config.logLevel).toBe('info');
    expect(runtime.config.defaultTimeout).toBe(30000);
  });

  // ── Subsystems ──

  it('should have all subsystems initialized', () => {
    const runtime = SsiRuntime.init({ logLevel: 'info' });
    expect(runtime.wasm).toBeDefined();
    expect(runtime.fs).toBeDefined();
    expect(runtime.net).toBeDefined();
    expect(runtime.threads).toBeDefined();
  });

  it('should detect current environment', () => {
    const runtime = SsiRuntime.init({ logLevel: 'info' });
    expect(['node', 'worker', 'browser']).toContain(runtime.environment);
  });

  // ── Feature Detection ──

  it('should report WebAssembly support', () => {
    SsiRuntime.init({ logLevel: 'info' });
    expect(typeof WebAssembly).not.toBe('undefined');
  });

  // ── getSummary ──

  it('should return a formatted summary string', () => {
    const runtime = SsiRuntime.init({ logLevel: 'info' });
    const summary = runtime.getSummary();
    expect(summary).toContain('SkyNet SSI-KRN Runtime');
    expect(summary).toContain('Environment');
    expect(summary).toContain('WASM Modules');
    expect(summary).toContain('Memory');
  });

  // ── Cleanup ──

  it('should dispose cleanly', () => {
    const runtime = SsiRuntime.init({ logLevel: 'info' });
    runtime.dispose();
    expect(runtime.isInitialized).toBe(false);
  });
});

describe('VirtualFS (SSI-FS frontend)', () => {
  afterEach(() => {
    // Clean up the singleton reference
    try {
      const fs = VirtualFS.getInstance();
      (fs as any).FS = null;
    } catch {
      // ignore
    }
  });

  it('should be a singleton', () => {
    const fs1 = VirtualFS.getInstance();
    const fs2 = VirtualFS.getInstance();
    expect(fs1).toBe(fs2);
  });

  it('should start as standalone (not connected)', () => {
    const fs = VirtualFS.getInstance();
    expect(fs.isConnected).toBe(false);
  });

  it('should accept an Emscripten FS connection', () => {
    const fs = VirtualFS.getInstance();
    expect(fs.isConnected).toBe(false);

    // Simulate connecting to Emscripten FS
    const mockFS = {
      analyzePath: () => ({ exists: false }),
      writeFile: () => {},
      readFile: () => new Uint8Array(0),
      mkdir: () => {},
      mkdirTree: () => {},
      readdir: () => [],
      unlink: () => {},
      stat: () => ({}),
      isDir: () => false,
      isFile: () => true,
    };
    fs.setFS(mockFS);
    expect(fs.isConnected).toBe(true);
  });
});

describe('Utility functions', () => {
  it('detectEnvironment should return a string', () => {
    const env = detectEnvironment();
    expect(typeof env).toBe('string');
    expect(['node', 'browser', 'worker', 'unknown']).toContain(env);
  });

  it('formatBytes should format correctly', () => {
    expect(formatBytes(0)).toMatch(/0\s*B/);
    expect(formatBytes(1024)).toContain('KB');
    expect(formatBytes(1024 * 1024)).toContain('MB');
    expect(formatBytes(1024 * 1024 * 1024)).toContain('GB');
  });

  it('formatBytes should handle decimals', () => {
    const result = formatBytes(1500);
    expect(result).toContain('.');
  });
});
