/**
 * SSI-CORE: Component Core — Type Definitions
 *
 * All types that SSI-CORE exports. Mirrors the IDL in SPEC-INTERFACE.md
 * exactly, translated to TypeScript.
 */

// =========================================================================
// Core Identifiers
// =========================================================================

export type UUID = string; // Format: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"

export interface SsiComponentId {
  uuid: UUID;
  name: string;
  version: string;
  type: string;
  apiVersion: number;
  vendor: string;
}

// =========================================================================
// Component State Machine
// =========================================================================

export enum SsiComponentState {
  UNINITIALIZED = 0,
  INITIALIZING,
  READY,
  RUNNING,
  SUSPENDED,
  ERROR,
  TERMINATED,
}

// =========================================================================
// Component Status
// =========================================================================

export interface SsiComponentStatus {
  state: SsiComponentState;
  uptimeMs: number;
  memoryUsed: number;
  memoryMax: number;
  errorCount: number;
  cpuUsage: number; // 0-1
}

// =========================================================================
// Messaging
// =========================================================================

export interface SsiMessage {
  type: number;
  id: number;
  timestamp: number;
  sourceUuid: UUID;
  targetUuid: UUID;
  payload: ArrayBuffer;
  priority: number; // 0-255
}

export interface SsiEvent {
  type: number;
  timestamp: number;
  sourceUuid: UUID;
  data: ArrayBuffer;
}

export type SsiEventCallback = (event: SsiEvent) => void;

// =========================================================================
// Buffer & Rect
// =========================================================================

export interface SsiBuffer {
  size: number;
  data: ArrayBuffer;
}

export interface SsiRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

// =========================================================================
// Error Handling
// =========================================================================

export enum SsiErrorCode {
  OK = 0x00000000,
  GENERIC = 0x80000001,
  NOT_FOUND = 0x80000002,
  BUSY = 0x80000003,
  TIMEOUT = 0x80000004,
  PERMISSION = 0x80000005,
  INVALID = 0x80000006,
  NOT_SUPPORTED = 0x80000007,
  MEMORY = 0x80000008,
}

export class SsiError extends Error {
  constructor(
    public code: SsiErrorCode,
    message: string,
    public component?: string,
  ) {
    super(`[SSI:0x${code.toString(16)}] ${message}`);
    this.name = 'SsiError';
  }
}

// =========================================================================
// Component Lifecycle Interface
// =========================================================================

export interface SsiComponentConfig {
  [key: string]: unknown;
}

export interface ISsiComponent {
  // ── Lifecycle ──
  init(config?: SsiComponentConfig): Promise<SsiErrorCode>;
  start(): Promise<SsiErrorCode>;
  suspend(): Promise<SsiErrorCode>;
  resume(): Promise<SsiErrorCode>;
  stop(): Promise<SsiErrorCode>;
  destroy(): void;

  // ── Queries ──
  getState(): SsiComponentState;
  getId(): SsiComponentId;
  getStatus(): SsiComponentStatus;

  // ── Communication ──
  sendMessage(msg: SsiMessage): Promise<SsiErrorCode>;
  receiveMessage(timeoutMs?: number): Promise<SsiMessage | null>;

  // ── Events ──
  onEvent(eventType: number, callback: SsiEventCallback): void;
  emitEvent(event: SsiEvent): Promise<SsiErrorCode>;
}

// =========================================================================
// Component Handle (runtime representation)
// =========================================================================

export interface SsiComponentHandle {
  id: UUID;
  name: string;
  type: string;
  component: ISsiComponent;
  state: SsiComponentState;
  memorySize: number;
}

// =========================================================================
// Registry
// =========================================================================

export interface SsiRegistryEntry {
  uuid: UUID;
  name: string;
  type: string;
  version: string;
  providesInterfaces: string[];
  requiresInterfaces: string[];
  state: SsiComponentState;
  registeredAt: number;
}

// =========================================================================
// SSI-CORE Utility Functions
// =========================================================================

export interface SsiInputEvent {
  type: number;         // 0=mouse, 1=touch, 2=keyboard, 3=gamepad
  action: number;       // 0=down, 1=up, 2=move, 3=scroll
  x: number;
  y: number;
  button: number;
  pressure: number;
  timestamp: number;
}

export function uuidv4(): UUID {
  // Use crypto.randomUUID if available (modern browsers, Node.js 19+)
  if (typeof crypto !== 'undefined' && typeof crypto.randomUUID === 'function') {
    return crypto.randomUUID() as UUID;
  }
  // Fallback for older environments
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
    const r = (Math.random() * 16) | 0;
    const v = c === 'x' ? r : (r & 0x3) | 0x8;
    return v.toString(16);
  });
}

export function timestampUs(): number {
  return Date.now() * 1000;
}

export function stateLabel(state: SsiComponentState): string {
  return SsiComponentState[state] || 'UNKNOWN';
}

export function errorLabel(code: SsiErrorCode): string {
  switch (code) {
    case SsiErrorCode.OK: return 'OK';
    case SsiErrorCode.GENERIC: return 'Generic Error';
    case SsiErrorCode.NOT_FOUND: return 'Not Found';
    case SsiErrorCode.BUSY: return 'Busy';
    case SsiErrorCode.TIMEOUT: return 'Timeout';
    case SsiErrorCode.PERMISSION: return 'Permission Denied';
    case SsiErrorCode.INVALID: return 'Invalid Argument';
    case SsiErrorCode.NOT_SUPPORTED: return 'Not Supported';
    case SsiErrorCode.MEMORY: return 'Out of Memory';
    default: return `Unknown (0x${ (code as number).toString(16) })`;
  }
}

export function encodeSsiBuffer(data: string | ArrayBuffer | Uint8Array): SsiBuffer {
  if (typeof data === 'string') {
    const encoded = new TextEncoder().encode(data);
    return { size: encoded.byteLength, data: encoded.buffer };
  }
  if (data instanceof Uint8Array) {
    return { size: data.byteLength, data: data.buffer as ArrayBuffer };
  }
  return { size: data.byteLength, data };
}

export function decodeSsiBufferAsString(buf: SsiBuffer): string {
  return new TextDecoder().decode(buf.data);
}

export function decodeSsiBufferAsUint8(buf: SsiBuffer): Uint8Array {
  return new Uint8Array(buf.data);
}
