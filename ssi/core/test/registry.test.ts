import { describe, it, expect, beforeEach } from 'vitest';
import { ComponentRegistry } from '../src/registry';
import { SsiBaseComponent } from '../src/component';
import { SsiComponentState } from '../src/types';

// A concrete component for testing
class TestComponent extends SsiBaseComponent {
  constructor(name: string, type: string = 'test') {
    super({ name, type, version: '1.0.0', vendor: 'test-vendor' });
  }
  protected async onInit() { /* noop */ }
}

describe('ComponentRegistry', () => {
  let registry: ComponentRegistry;

  beforeEach(() => {
    registry = new ComponentRegistry();
  });

  describe('register', () => {
    it('should register a component and track it by name', () => {
      const comp = new TestComponent('comp-a');
      registry.register(comp);
      expect(registry.size).toBe(1);
      expect(registry.has(comp.getId().uuid)).toBe(true);
      expect(registry.findByName('comp-a')).toBe(comp);
    });

    it('should register a component and track it by type', () => {
      const comp = new TestComponent('comp-b', 'storage');
      registry.register(comp);
      const results = registry.findByType('storage');
      expect(results).toHaveLength(1);
      expect(results[0]).toBe(comp);
    });

    it('should allow registering multiple components of the same type', () => {
      const c1 = new TestComponent('c1', 'network');
      const c2 = new TestComponent('c2', 'network');
      registry.register(c1);
      registry.register(c2);
      expect(registry.findByType('network')).toHaveLength(2);
    });

    it('should allow lookup by UUID after registration', () => {
      const comp = new TestComponent('lookup-me');
      registry.register(comp);
      expect(registry.findByUuid(comp.getId().uuid)).toBe(comp);
    });
  });

  describe('unregister', () => {
    it('should remove a component from all indices', () => {
      const comp = new TestComponent('removable', 'ephemeral');
      registry.register(comp);
      const uuid = comp.getId().uuid;
      expect(registry.size).toBe(1);

      registry.unregister(uuid);
      expect(registry.size).toBe(0);
      expect(registry.findByName('removable')).toBeUndefined();
      expect(registry.findByUuid(uuid)).toBeUndefined();
      expect(registry.findByType('ephemeral')).toHaveLength(0);
    });

    it('should be safe to unregister a non-existent component', () => {
      expect(() => registry.unregister('nonexistent-uuid')).not.toThrow();
    });
  });

  describe('findByType', () => {
    it('should return empty array for unregistered type', () => {
      expect(registry.findByType('nonexistent')).toEqual([]);
    });
  });

  describe('listEntries', () => {
    it('should return all registry entries', () => {
      const c1 = new TestComponent('alpha');
      const c2 = new TestComponent('beta');
      registry.register(c1);
      registry.register(c2);
      const entries = registry.listEntries();
      expect(entries).toHaveLength(2);
      expect(entries.map(e => e.name).sort()).toEqual(['alpha', 'beta']);
    });
  });

  describe('updateState', () => {
    it('should update the state of a registered component entry', () => {
      const comp = new TestComponent('stateful');
      comp.init(); // transitions to READY
      registry.register(comp);
      const uuid = comp.getId().uuid;

      registry.updateState(uuid, SsiComponentState.RUNNING);
      const entry = registry.listEntries().find(e => e.uuid === uuid);
      expect(entry?.state).toBe(SsiComponentState.RUNNING);
    });

    it('should be safe to update state for unregistered component', () => {
      expect(() => registry.updateState('missing', SsiComponentState.RUNNING)).not.toThrow();
    });
  });

  describe('clear', () => {
    it('should remove all entries', () => {
      registry.register(new TestComponent('a'));
      registry.register(new TestComponent('b'));
      expect(registry.size).toBe(2);
      registry.clear();
      expect(registry.size).toBe(0);
    });
  });
});
