/**
 * SSI-AI: Compute Engine Interface
 *
 * Implements the IComputeEngine interface from SPEC-INTERFACE.md §7.
 * Provides task scheduling, model management, local inference, and resource reporting.
 *
 * SSI Interfaces implemented:
 *   - SSI-AI: Task queue, model registry, inference, compute resources
 */

import {
  SsiBaseComponent,
  SsiComponentConfig,
  SsiErrorCode,
  uuidv4,
} from '../../core/src/index';

// =========================================================================
// SSI-AI Data Structures
// =========================================================================

export enum SsiAiTaskStatus {
  PENDING = 0,
  RUNNING = 1,
  COMPLETED = 2,
  FAILED = 3,
  CANCELLED = 4,
}

export const SSI_AI_TASK_STATUS_LABEL: Record<SsiAiTaskStatus, string> = {
  [SsiAiTaskStatus.PENDING]: 'PENDING',
  [SsiAiTaskStatus.RUNNING]: 'RUNNING',
  [SsiAiTaskStatus.COMPLETED]: 'COMPLETED',
  [SsiAiTaskStatus.FAILED]: 'FAILED',
  [SsiAiTaskStatus.CANCELLED]: 'CANCELLED',
};

export interface SsiAiTask {
  id: string;
  type: 'training' | 'inference' | 'generic';
  modelName: string;
  input: ArrayBuffer;
  priority: number;
  maxCostTokens: number;
  requiredCapabilities: string[];
  status: SsiAiTaskStatus;
  createdAt: number;
  completedAt: number | null;
  result: ArrayBuffer | null;
  error: string | null;
}

export interface SsiComputeResources {
  cpuCores: number;
  gpuCount: number;
  ramBytes: number;
  vramBytes: number;
  computeUnits: number;
}

export interface SsiModelEntry {
  name: string;
  data: ArrayBuffer;
  registeredAt: number;
  size: number;
  capabilities: string[];
}

// =========================================================================
// Compute Engine — SSI-AI Implementation
// =========================================================================

export class ComputeEngine extends SsiBaseComponent {
  private tasks = new Map<string, SsiAiTask>();
  private models = new Map<string, SsiModelEntry>();
  private taskCounter = 0;
  private schedulerTimer: ReturnType<typeof setInterval> | null = null;
  private maxConcurrentTasks = 2;
  private runningTasks = 0;

  constructor() {
    super({ name: 'compute-engine', type: 'ai', version: '1.0.0', vendor: 'AI-TP' });
  }

  // =========================================================================
  // Task Management
  // =========================================================================

  /** Submit an AI compute task */
  submitTask(task: Omit<SsiAiTask, 'id' | 'status' | 'createdAt' | 'completedAt' | 'result' | 'error'>): string {
    const id = `task-${++this.taskCounter}-${Date.now()}`;
    const fullTask: SsiAiTask = {
      ...task,
      id,
      status: SsiAiTaskStatus.PENDING,
      createdAt: Date.now(),
      completedAt: null,
      result: null,
      error: null,
    };

    this.tasks.set(id, fullTask);
    this.log(`Task submitted: "${task.modelName}" (${id})`);

    // Schedule immediate execution
    this.scheduleExecution();

    return id;
  }

  /** Get task status */
  getTaskStatus(taskId: string): { status: SsiAiTaskStatus; task: SsiAiTask | null } {
    const task = this.tasks.get(taskId);
    if (!task) return { status: SsiAiTaskStatus.FAILED, task: null };
    return { status: task.status, task };
  }

  /** Cancel a task */
  cancelTask(taskId: string): SsiErrorCode {
    const task = this.tasks.get(taskId);
    if (!task) return SsiErrorCode.NOT_FOUND;
    if (task.status === SsiAiTaskStatus.COMPLETED || task.status === SsiAiTaskStatus.CANCELLED) {
      return SsiErrorCode.BUSY;
    }
    const wasRunning = task.status === SsiAiTaskStatus.RUNNING;
    task.status = SsiAiTaskStatus.CANCELLED;
    task.completedAt = Date.now();
    if (wasRunning) {
      this.runningTasks--;
    }
    this.log(`Task cancelled: ${taskId}`);
    return SsiErrorCode.OK;
  }

  /** Get task result (with optional timeout) */
  async getTaskResult(taskId: string, timeoutMs: number = 0): Promise<{ result: ArrayBuffer | null; error: SsiErrorCode }> {
    const task = this.tasks.get(taskId);
    if (!task) return { result: null, error: SsiErrorCode.NOT_FOUND };

    if (task.status === SsiAiTaskStatus.COMPLETED && task.result) {
      return { result: task.result.slice(0), error: SsiErrorCode.OK };
    }

    if (task.status === SsiAiTaskStatus.FAILED) {
      return { result: null, error: SsiErrorCode.GENERIC };
    }

    if (task.status === SsiAiTaskStatus.CANCELLED) {
      return { result: null, error: SsiErrorCode.PERMISSION };
    }

    // If task is still running, wait for completion if timeout > 0
    if (timeoutMs > 0 && (task.status === SsiAiTaskStatus.PENDING || task.status === SsiAiTaskStatus.RUNNING)) {
      const deadline = Date.now() + timeoutMs;
      while (Date.now() < deadline) {
        await new Promise(resolve => setTimeout(resolve, 50));
        const updated = this.tasks.get(taskId);
        if (!updated) return { result: null, error: SsiErrorCode.NOT_FOUND };
        if (updated.status === SsiAiTaskStatus.COMPLETED && updated.result) {
          return { result: updated.result.slice(0), error: SsiErrorCode.OK };
        }
        if (updated.status === SsiAiTaskStatus.FAILED) {
          return { result: null, error: SsiErrorCode.GENERIC };
        }
        if (updated.status === SsiAiTaskStatus.CANCELLED) {
          return { result: null, error: SsiErrorCode.PERMISSION };
        }
      }
      return { result: null, error: SsiErrorCode.TIMEOUT };
    }

    return { result: null, error: SsiErrorCode.NOT_FOUND };
  }

  /** Get available compute resources */
  getAvailableResources(): SsiComputeResources {
    return {
      cpuCores: typeof navigator !== 'undefined' ? navigator.hardwareConcurrency || 4 : 4,
      gpuCount: this.detectGpuCount(),
      ramBytes: this.detectRam(),
      vramBytes: this.detectVram(),
      computeUnits: this.calculateComputeUnits(),
    };
  }

  /** List all tasks */
  listTasks(): SsiAiTask[] {
    return Array.from(this.tasks.values())
      .sort((a, b) => b.createdAt - a.createdAt);
  }

  // =========================================================================
  // Model Management
  // =========================================================================

  /** Register a model for inference */
  registerModel(modelName: string, modelData: ArrayBuffer): SsiErrorCode {
    if (!modelName) return SsiErrorCode.INVALID;

    this.models.set(modelName, {
      name: modelName,
      data: modelData.slice(0),
      registeredAt: Date.now(),
      size: modelData.byteLength,
      capabilities: ['generic'],
    });

    this.log(`Model registered: "${modelName}" (${modelData.byteLength} bytes)`);
    return SsiErrorCode.OK;
  }

  /** Execute local inference */
  async inference(modelName: string, input: ArrayBuffer): Promise<{ output: ArrayBuffer | null; error: SsiErrorCode }> {
    const model = this.models.get(modelName);
    if (!model) return { output: null, error: SsiErrorCode.NOT_FOUND };

    // Simulate inference: apply a simple transformation on the input
    // In production, this would call an actual ML runtime (ONNX, WebNN, etc.)
    try {
      const inputBytes = new Uint8Array(input);
      const outputBytes = new Uint8Array(inputBytes.length);

      // Simulated computation: model "identity" returns input unchanged
      // Other models apply simple transforms
      if (modelName.includes('reverse')) {
        for (let i = 0; i < inputBytes.length; i++) {
          outputBytes[i] = inputBytes[inputBytes.length - 1 - i];
        }
      } else if (modelName.includes('scale')) {
        for (let i = 0; i < inputBytes.length; i++) {
          outputBytes[i] = Math.min(255, inputBytes[i] * 2);
        }
      } else {
        // Identity model — return input
        outputBytes.set(inputBytes);
      }

      this.log(`Inference completed: "${modelName}" (${inputBytes.length} → ${outputBytes.length} bytes)`);
      return { output: outputBytes.buffer, error: SsiErrorCode.OK };
    } catch (err: any) {
      this.log(`Inference failed: ${err.message}`, 'error');
      return { output: null, error: SsiErrorCode.GENERIC };
    }
  }

  /** Unregister a model */
  unregisterModel(modelName: string): SsiErrorCode {
    if (!this.models.has(modelName)) return SsiErrorCode.NOT_FOUND;
    this.models.delete(modelName);
    return SsiErrorCode.OK;
  }

  /** List registered models */
  listModels(): string[] {
    return Array.from(this.models.keys());
  }

  // =========================================================================
  // Lifecycle
  // =========================================================================

  protected async onInit(_config: SsiComponentConfig): Promise<void> {
    this.log('Compute Engine initializing');
  }

  protected async onStart(): Promise<void> {
    // Periodic scheduler tick
    this.schedulerTimer = setInterval(() => this.scheduleExecution(), 1000);
    this.log('Compute Engine started');
  }

  protected async onStop(): Promise<void> {
    if (this.schedulerTimer) {
      clearInterval(this.schedulerTimer);
      this.schedulerTimer = null;
    }
    // Cancel all pending tasks
    for (const [id, task] of this.tasks) {
      if (task.status === SsiAiTaskStatus.PENDING || task.status === SsiAiTaskStatus.RUNNING) {
        task.status = SsiAiTaskStatus.CANCELLED;
      }
    }
    this.runningTasks = 0;
    this.log('Compute Engine stopped');
  }

  protected onDestroy(): void {
    this.tasks.clear();
    this.models.clear();
    this.runningTasks = 0;
    if (this.schedulerTimer) clearInterval(this.schedulerTimer);
  }

  // =========================================================================
  // Internal
  // =========================================================================

  private scheduleExecution(): void {
    if (this.runningTasks >= this.maxConcurrentTasks) return;

    // Find highest priority pending task
    const pending = Array.from(this.tasks.values())
      .filter(t => t.status === SsiAiTaskStatus.PENDING)
      .sort((a, b) => b.priority - a.priority || a.createdAt - b.createdAt);

    if (pending.length === 0) return;

    const slots = this.maxConcurrentTasks - this.runningTasks;
    for (let i = 0; i < Math.min(slots, pending.length); i++) {
      this.executeTask(pending[i]);
    }
  }

  private async executeTask(task: SsiAiTask): Promise<void> {
    task.status = SsiAiTaskStatus.RUNNING;
    this.runningTasks++;
    this.log(`Executing task: ${task.id} (${task.modelName})`);

    // Simulate compute time based on input size
    const computeTime = Math.min(2000, Math.max(10, (task.input?.byteLength || 100) / 100));
    await new Promise(resolve => setTimeout(resolve, computeTime));

    // Execute the task (run inference with model)
    const model = this.models.get(task.modelName);
    if (model && task.input) {
      const { output } = await this.inference(task.modelName, task.input);
      if (output) {
        task.result = output;
        task.status = SsiAiTaskStatus.COMPLETED;
      } else {
        task.status = SsiAiTaskStatus.FAILED;
        task.error = 'Inference returned no output';
      }
    } else if (!model) {
      // No model — just return input as-is
      task.result = task.input ? task.input.slice(0) : new ArrayBuffer(0);
      task.status = SsiAiTaskStatus.COMPLETED;
    }

    task.completedAt = Date.now();
    this.runningTasks--;
    this.log(`Task completed: ${task.id} → ${SSI_AI_TASK_STATUS_LABEL[task.status]}`);
  }

  private detectGpuCount(): number {
    // In Node.js/Electron, try to detect GPU via WebGL
    try {
      if (typeof document !== 'undefined') {
        const canvas = document.createElement('canvas');
        const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
        if (gl) return 1;
      }
    } catch {}
    return 0;
  }

  private detectRam(): number {
    try {
      if (typeof performance !== 'undefined' && 'memory' in performance) {
        return (performance as any).memory?.jsHeapSizeLimit || 2147483648;
      }
    } catch {}
    return 2147483648; // 2GB default
  }

  private detectVram(): number {
    // Cannot detect VRAM in software; return reasonable default
    return 536870912; // 512MB default
  }

  private calculateComputeUnits(): number {
    const cores = typeof navigator !== 'undefined' ? navigator.hardwareConcurrency || 4 : 4;
    const gpu = this.detectGpuCount();
    return cores * 1.0 + gpu * 8.0;
  }
}
