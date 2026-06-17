/**
 * SSI-PACKAGER: SWBN Component Packager Tests
 */

import { describe, it, expect } from 'vitest';

describe('SwbnPackager', () => {
  it('should have sign/verify/info static methods', async () => {
    const mod = await import('../src/index');
    expect(typeof mod.SwbnPackager.sign).toBe('function');
    expect(typeof mod.SwbnPackager.verify).toBe('function');
    expect(typeof mod.SwbnPackager.info).toBe('function');
  });

  it('should have generateKey method', async () => {
    const mod = await import('../src/index');
    expect(typeof mod.SwbnPackager.generateKey).toBe('function');
  });
});
