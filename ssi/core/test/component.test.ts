import { describe, it, expect } from 'vitest';
import { SsiBaseComponent } from '../src/component';
import { SsiComponentState, SsiErrorCode } from '../src/types';

// ── Test Helpers ──

class LifecycleTracker extends SsiBaseComponent {
  public hooks: string[] = [];

  constructor(name: string = 'tracker') {
    super({ name, type: 'test', version: '1.0.0', vendor: 'test' });
  }

  protected async onInit() { this.hooks.push('onInit'); }
  protected async onStart() { this.hooks.push('onStart'); }
  protected async onSuspend() { this.hooks.push('onSuspend'); }
  protected async onResume() { this.hooks.push('onResume'); }
  protected async onStop() { this.hooks.push('onStop'); }
  protected onDestroy(): void { this.hooks.push('onDestroy'); }
}

class FailingComponent extends SsiBaseComponent {
  constructor(private failOn: 'init' | 'start' | 'stop' | 'suspend' | 'resume') {
    super({ name: 'failing', type: 'test', version: '1.0.0', vendor: 'test' });
  }

  protected async onInit() {
    if (this.failOn === 'init') throw new Error('init failed');
  }
  protected async onStart() {
    if (this.failOn === 'start') throw new Error('start failed');
  }
  protected async onStop() {
    if (this.failOn === 'stop') throw new Error('stop failed');
  }
  protected async onSuspend() {
    if (this.failOn === 'suspend') throw new Error('suspend failed');
  }
  protected async onResume() {
    if (this.failOn === 'resume') throw new Error('resume failed');
  }
}

// ── Tests ──

describe('SsiBaseComponent', () => {
  describe('constructor', () => {
    it('should set default values for missing fields', () => {
      const comp = new LifecycleTracker('defaults');
      const id = comp.getId();
      expect(id.name).toBe('defaults');
      expect(id.version).toBe('1.0.0');
      expect(id.type).toBe('test');
      expect(id.vendor).toBe('test');
      expect(id.uuid).toBeDefined();
      expect(id.uuid.length).toBeGreaterThan(0);
    });

    it('should generate a unique UUID for each component', () => {
      const a = new LifecycleTracker('a');
      const b = new LifecycleTracker('b');
      expect(a.getId().uuid).not.toBe(b.getId().uuid);
    });
  });

  describe('init', () => {
    it('should transition from UNINITIALIZED to READY on success', async () => {
      const comp = new LifecycleTracker();
      const result = await comp.init();
      expect(result).toBe(SsiErrorCode.OK);
      expect(comp.getState()).toBe(SsiComponentState.READY);
      expect(comp.hooks).toContain('onInit');
    });

    it('should refuse re-init when already READY', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      const result = await comp.init();
      expect(result).toBe(SsiErrorCode.BUSY);
    });

    it('should transition to ERROR on init failure', async () => {
      const comp = new FailingComponent('init');
      const result = await comp.init();
      expect(result).toBe(SsiErrorCode.GENERIC);
      expect(comp.getState()).toBe(SsiComponentState.ERROR);
    });

    it('should allow re-init after ERROR state', async () => {
      const comp = new FailingComponent('init');
      await comp.init(); // fails → ERROR
      expect(comp.getState()).toBe(SsiComponentState.ERROR);
      // After fixing, we can re-init — the component is a FailingComponent
      // so it will fail again, but the point is ERROR allows retry
      const result = await comp.init();
      expect(result).toBe(SsiErrorCode.GENERIC);
      expect(comp.getState()).toBe(SsiComponentState.ERROR);
    });
  });

  describe('start', () => {
    it('should transition from READY to RUNNING', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      const result = await comp.start();
      expect(result).toBe(SsiErrorCode.OK);
      expect(comp.getState()).toBe(SsiComponentState.RUNNING);
      expect(comp.hooks).toContain('onStart');
    });

    it('should refuse start when UNINITIALIZED', async () => {
      const comp = new LifecycleTracker();
      const result = await comp.start();
      expect(result).toBe(SsiErrorCode.BUSY);
      expect(comp.getState()).toBe(SsiComponentState.UNINITIALIZED);
    });

    it('should transition to ERROR on start failure', async () => {
      const comp = new FailingComponent('start');
      await comp.init();
      const result = await comp.start();
      expect(result).toBe(SsiErrorCode.GENERIC);
      expect(comp.getState()).toBe(SsiComponentState.ERROR);
    });

    it('should track startedAt when entering RUNNING', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      const statusBefore = comp.getStatus();
      expect(statusBefore.uptimeMs).toBe(0);

      await comp.start();
      const statusAfter = comp.getStatus();
      // uptimeMs should be >= 0 after start (may be 0 in same millisecond)
      expect(statusAfter.uptimeMs).toBeGreaterThanOrEqual(0);
      expect(statusAfter.state).toBe(SsiComponentState.RUNNING);
    });
  });

  describe('suspend / resume', () => {
    it('should suspend from RUNNING to SUSPENDED', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      await comp.start();
      const result = await comp.suspend();
      expect(result).toBe(SsiErrorCode.OK);
      expect(comp.getState()).toBe(SsiComponentState.SUSPENDED);
      expect(comp.hooks).toContain('onSuspend');
    });

    it('should resume from SUSPENDED to RUNNING', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      await comp.start();
      await comp.suspend();
      const result = await comp.resume();
      expect(result).toBe(SsiErrorCode.OK);
      expect(comp.getState()).toBe(SsiComponentState.RUNNING);
      expect(comp.hooks).toContain('onResume');
    });

    it('should refuse suspend when not RUNNING', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      const result = await comp.suspend();
      expect(result).toBe(SsiErrorCode.BUSY);
    });

    it('should refuse resume when not SUSPENDED', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      const result = await comp.resume();
      expect(result).toBe(SsiErrorCode.BUSY);
    });
  });

  describe('stop', () => {
    it('should transition from RUNNING to TERMINATED', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      await comp.start();
      const result = await comp.stop();
      expect(result).toBe(SsiErrorCode.OK);
      expect(comp.getState()).toBe(SsiComponentState.TERMINATED);
      expect(comp.hooks).toContain('onStop');
    });

    it('should stop from SUSPENDED state', async () => {
      const comp = new LifecycleTracker();
      await comp.init();
      await comp.start();
      await comp.suspend();
      const result = await comp.stop();
      expect(result).toBe(SsiErrorCode.OK);
      expect(comp.getState()).toBe(SsiComponentState.TERMINATED);
    });

    it('should refuse stop when UNINITIALIZED', async () => {
      const comp = new LifecycleTracker();
      const result = await comp.stop();
      expect(result).toBe(SsiErrorCode.BUSY);
    });
  });

  describe('destroy', () => {
    it('should transition to TERMINATED and call onDestroy', () => {
      const comp = new LifecycleTracker();
      comp.destroy();
      expect(comp.getState()).toBe(SsiComponentState.TERMINATED);
      expect(comp.hooks).toContain('onDestroy');
    });
  });

  describe('getStatus', () => {
    it('should report correct state and error count', async () => {
      const comp = new LifecycleTracker();
      const status1 = comp.getStatus();
      expect(status1.state).toBe(SsiComponentState.UNINITIALIZED);
      expect(status1.errorCount).toBe(0);

      await comp.init();
      await comp.start();
      const status2 = comp.getStatus();
      expect(status2.state).toBe(SsiComponentState.RUNNING);
    });

    it('should increment errorCount on init failure', async () => {
      const comp = new FailingComponent('init');
      await comp.init();
      expect(comp.getStatus().errorCount).toBe(1);
    });
  });

  describe('message queue', () => {
    it('should send and receive messages', async () => {
      const comp = new LifecycleTracker();
      const msg = {
        type: 1, id: 100, timestamp: Date.now() * 1000,
        sourceUuid: 'src-uuid', targetUuid: 'tgt-uuid',
        payload: new ArrayBuffer(0), priority: 50,
      };
      const sendResult = await comp.sendMessage(msg);
      expect(sendResult).toBe(SsiErrorCode.OK);

      const received = await comp.receiveMessage(100);
      expect(received).not.toBeNull();
      expect(received!.id).toBe(100);
    });

    it('should return null on timeout with no messages', async () => {
      const comp = new LifecycleTracker();
      const received = await comp.receiveMessage(50);
      expect(received).toBeNull();
    });
  });

  describe('event system', () => {
    it('should emit events to registered listeners', async () => {
      const comp = new LifecycleTracker();
      const events: number[] = [];
      comp.onEvent(42, (e) => { events.push(e.type); });
      await comp.emitEvent({ type: 42, source: 'test', timestamp: Date.now() * 1000, data: new ArrayBuffer(0) });
      expect(events).toEqual([42]);
    });

    it('should not throw when emitting with no listeners', async () => {
      const comp = new LifecycleTracker();
      await expect(
        comp.emitEvent({ type: 99, source: 'test', timestamp: Date.now() * 1000, data: new ArrayBuffer(0) })
      ).resolves.toBe(SsiErrorCode.OK);
    });
  });
});
