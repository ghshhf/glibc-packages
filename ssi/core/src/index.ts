/**
 * SSI-CORE: Entry Point
 *
 * Exports all SSI-CORE types and the reference implementation.
 * This is the base layer that all SSI components build on.
 *
 * SSI Interfaces implemented:
 *   - SSI-CORE: Component lifecycle, registry, messaging
 */

export { SsiBaseComponent } from './component';
export { ComponentRegistry } from './registry';
export * from './types';
