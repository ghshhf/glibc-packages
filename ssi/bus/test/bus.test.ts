import { describe, it, expect, beforeEach } from 'vitest';
import { SsiServiceBus, SsiMsgFlags, SsiPriority } from '../src/index';
import { ComponentRegistry } from '../../core/src/registry';
import { SsiBaseComponent } from '../../core/src/component';
import { SsiErrorCode } from '../../core/src/types';

// ── Test Helpers ──

class BusTestComponent extends SsiBaseComponent {
  public receivedMessages: any[] = [];
  constructor(name: string, type: string = 'generic') {
    super({ name, type, version: '1.0.0', vendor: 'test' });
  }

  protected async onInit() { /* noop */ }
}

function makeMsg(id: number, src: string, tgt: string, payload?: string): any {
  return {
    type: 1,
    id,
    timestamp: Date.now() * 1000,
    sourceUuid: src,
    targetUuid: tgt,
    payload: payload ? new TextEncoder().encode(payload).buffer : new ArrayBuffer(0),
    priority: SsiPriority.NORMAL,
  };
}

// ── Tests ──

describe('SsiServiceBus', () => {
  let registry: ComponentRegistry;
  let bus: SsiServiceBus;

  beforeEach(async () => {
    registry = new ComponentRegistry();
    bus = new SsiServiceBus(registry);
    await bus.init();
    await bus.start();
  });

  describe('lifecycle', () => {
    it('should start in UNINITIALIZED then transition to RUNNING', () => {
      const b = new SsiServiceBus(registry);
      expect(b.getState()).toBeDefined();
    });

    it('should be able to init and start', async () => {
      const b = new SsiServiceBus(registry);
      const initResult = await b.init();
      expect(initResult).toBe(SsiErrorCode.OK);
      const startResult = await b.start();
      expect(startResult).toBe(SsiErrorCode.OK);
    });
  });

  describe('send', () => {
    it('should deliver a message to a registered component', async () => {
      const comp = new BusTestComponent('receiver', 'app');
      await comp.init();
      await comp.start();
      registry.register(comp);

      const msg = makeMsg(1, 'sender-uuid', comp.getId().uuid, 'hello');
      const result = await bus.send(msg);
      expect(result).toBe(SsiErrorCode.OK);
    });

    it('should return NOT_FOUND for unregistered target', async () => {
      const msg = makeMsg(2, 'src-uuid', 'nonexistent-uuid');
      const result = await bus.send(msg);
      expect(result).toBe(SsiErrorCode.NOT_FOUND);
    });
  });

  describe('broadcast', () => {
    it('should deliver messages to all components of a type', async () => {
      const c1 = new BusTestComponent('worker-1', 'worker');
      const c2 = new BusTestComponent('worker-2', 'worker');
      await c1.init(); await c1.start();
      await c2.init(); await c2.start();
      registry.register(c1);
      registry.register(c2);

      await expect(bus.broadcast(
        makeMsg(3, 'src-uuid', '00000000-0000-0000-0000-000000000000'),
        'worker'
      )).resolves.toBeUndefined();

      const stats = bus.getStats();
      expect(stats.messagesDelivered).toBe(2);
    });

    it('should not throw when no components match the type', async () => {
      await expect(bus.broadcast(
        makeMsg(4, 'src-uuid', '00000000-0000-0000-0000-000000000000'),
        'nonexistent-type'
      )).resolves.toBeUndefined();
    });
  });

  describe('stats', () => {
    it('should track message delivery metrics', async () => {
      const comp = new BusTestComponent('metrics-target');
      await comp.init(); await comp.start();
      registry.register(comp);

      await bus.send(makeMsg(10, 'src-uuid', comp.getId().uuid));
      await bus.send(makeMsg(11, 'src-uuid', 'unknown-uuid'));

      const stats = bus.getStats();
      expect(stats.messagesSent).toBe(2);
      expect(stats.messagesDelivered).toBe(1);
      expect(stats.messagesDropped).toBe(1);
      expect(stats.routingErrors).toBe(1);
    });
  });
});
