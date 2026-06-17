import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { SsiErrorCode } from '../../core/src/index';
import { ComputeEngine, SsiAiTaskStatus } from '../src/index';

describe('ComputeEngine', () => {
  let engine: ComputeEngine;

  beforeEach(async () => {
    engine = new ComputeEngine();
    await engine.init();
    await engine.start();
  });

  afterEach(async () => {
    await engine.stop();
  });

  describe('task management', () => {
    it('should submit and track a task', () => {
      const id = engine.submitTask({
        type: 'inference',
        modelName: 'test-model',
        input: new TextEncoder().encode('hello').buffer,
        priority: 50,
        maxCostTokens: 1000,
        requiredCapabilities: ['generic'],
      });
      expect(id).toBeDefined();
      expect(id.startsWith('task-')).toBe(true);

      const { status, task } = engine.getTaskStatus(id);
      expect(task).not.toBeNull();
      expect([SsiAiTaskStatus.PENDING, SsiAiTaskStatus.RUNNING]).toContain(status);
    });

    it('should cancel a pending task', () => {
      const id = engine.submitTask({
        type: 'generic',
        modelName: 'slow-model',
        input: new ArrayBuffer(1000),
        priority: 10,
        maxCostTokens: 100,
        requiredCapabilities: [],
      });
      expect(engine.cancelTask(id)).toBe(SsiErrorCode.OK);
      expect(engine.getTaskStatus(id).status).toBe(SsiAiTaskStatus.CANCELLED);
    });

    it('should return NOT_FOUND for unknown task', () => {
      const { status, task } = engine.getTaskStatus('nonexistent');
      expect(task).toBeNull();
      expect(engine.cancelTask('nonexistent')).toBe(SsiErrorCode.NOT_FOUND);
    });

    it('should list all tasks', () => {
      engine.submitTask({
        type: 'training', modelName: 'm1', input: new ArrayBuffer(10),
        priority: 50, maxCostTokens: 100, requiredCapabilities: [],
      });
      engine.submitTask({
        type: 'inference', modelName: 'm2', input: new ArrayBuffer(10),
        priority: 80, maxCostTokens: 100, requiredCapabilities: [],
      });
      expect(engine.listTasks()).toHaveLength(2);
    });

    it('should complete tasks with result', async () => {
      const id = engine.submitTask({
        type: 'inference',
        modelName: 'test',
        input: new TextEncoder().encode('data').buffer,
        priority: 99,
        maxCostTokens: 100,
        requiredCapabilities: [],
      });

      // Wait for completion
      const { result, error } = await engine.getTaskResult(id, 3000);
      expect(error).toBe(SsiErrorCode.OK);
      expect(result).not.toBeNull();
      expect(result!.byteLength).toBe(4); // "data" = 4 bytes (identity model)
    });
  });

  describe('model management', () => {
    it('should register a model', () => {
      const data = new TextEncoder().encode('model-weights').buffer;
      expect(engine.registerModel('my-model', data)).toBe(SsiErrorCode.OK);
      expect(engine.listModels()).toContain('my-model');
    });

    it('should reject empty model name', () => {
      expect(engine.registerModel('', new ArrayBuffer(0))).toBe(SsiErrorCode.INVALID);
    });

    it('should run inference with registered model', async () => {
      const modelData = new TextEncoder().encode('weights').buffer;
      engine.registerModel('my-model', modelData);

      const input = new TextEncoder().encode('test-input').buffer;
      const { output, error } = await engine.inference('my-model', input);
      expect(error).toBe(SsiErrorCode.OK);
      expect(output).not.toBeNull();
      expect(new TextDecoder().decode(output!)).toBe('test-input'); // identity
    });

    it('should return NOT_FOUND for unknown model', async () => {
      const { output, error } = await engine.inference('unknown', new ArrayBuffer(0));
      expect(output).toBeNull();
      expect(error).toBe(SsiErrorCode.NOT_FOUND);
    });

    it('should unregister a model', () => {
      engine.registerModel('temp', new ArrayBuffer(10));
      expect(engine.unregisterModel('temp')).toBe(SsiErrorCode.OK);
      expect(engine.listModels()).not.toContain('temp');
    });

    it('should apply reverse model transform', async () => {
      const modelData = new TextEncoder().encode('reverse').buffer;
      engine.registerModel('reverse-model', modelData);

      const input = new TextEncoder().encode('ABC').buffer;
      const { output } = await engine.inference('reverse-model', input);
      expect(new TextDecoder().decode(output!)).toBe('CBA');
    });
  });

  describe('compute resources', () => {
    it('should report available resources', () => {
      const resources = engine.getAvailableResources();
      expect(resources.cpuCores).toBeGreaterThan(0);
      expect(resources.ramBytes).toBeGreaterThan(0);
      expect(resources.computeUnits).toBeGreaterThan(0);
    });
  });

  describe('lifecycle', () => {
    it('should cancel pending tasks on stop', async () => {
      const id = engine.submitTask({
        type: 'generic', modelName: 'm', input: new ArrayBuffer(10000),
        priority: 50, maxCostTokens: 100, requiredCapabilities: [],
      });
      await engine.stop();
      expect(engine.getTaskStatus(id).status).toBe(SsiAiTaskStatus.CANCELLED);
    });
  });
});
