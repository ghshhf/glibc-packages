/**
 * SSI Service Bus — Message Protocol
 *
 * Implements the IPC message bus protocol defined in SPEC-INTERFACE.md §12.
 * Handles binary message encoding/decoding, routing, and delivery.
 *
 * Message frame format (72 bytes header + payload):
 *   Offset  Size  Field
 *   0       4     magic    0x53534900 ("SSI\0")
 *   4       2     version
 *   6       2     flags    0x01=response, 0x02=error, 0x04=stream
 *   8       4     msg_type 0=call, 1=return, 2=event, 3=stream_data
 *   12      8     message_id
 *   20      8     correlation_id (0 for requests)
 *   28      16    source_uuid
 *   44      16    target_uuid
 *   60      1     priority  0-255
 *   61      1     ttl
 *   62      2     reserved
 *   64      4     payload_length
 *   68      N     payload
 *   68+N    4     checksum CRC32 of header+payload
 */

import {
  SsiMessage,
  SsiErrorCode,
  UUID,
  timestampUs,
  encodeSsiBuffer,
} from '../../core/src/types';
import { ComponentRegistry } from '../../core/src/registry';
import { SsiBaseComponent } from '../../core/src/component';

// =========================================================================
// Message Frame Constants
// =========================================================================

const MAGIC = 0x53534900; // "SSI\0"
const HEADER_SIZE = 68;

export enum SsiMsgType {
  CALL = 0,
  RETURN = 1,
  EVENT = 2,
  STREAM_DATA = 3,
}

export enum SsiMsgFlags {
  NONE = 0,
  RESPONSE = 0x01,
  ERROR = 0x02,
  STREAM = 0x04,
}

export enum SsiPriority {
  BACKGROUND = 0,
  LOW = 25,
  NORMAL = 50,
  HIGH = 100,
  CRITICAL = 200,
}

// =========================================================================
// Message Codec
// =========================================================================

export function encodeMessage(msg: SsiMessage): ArrayBuffer {
  // Parse UUIDs from hex string to bytes
  const sourceBytes = hexToBytes(msg.sourceUuid);
  const targetBytes = hexToBytes(msg.targetUuid);
  const payloadArr = msg.payload instanceof ArrayBuffer ? msg.payload : msg.payload;

  const totalLen = HEADER_SIZE + payloadArr.byteLength + 4; // +4 for CRC
  const buf = new ArrayBuffer(totalLen);
  const view = new DataView(buf);

  // Header
  view.setUint32(0, MAGIC, true);           // magic
  view.setUint16(4, 1, true);               // version
  view.setUint16(6, 0, true);               // flags
  view.setUint32(8, msg.type, true);        // msg_type
  view.setBigUint64(12, BigInt(msg.id), true);         // message_id
  view.setBigUint64(20, BigInt(0), true);              // correlation_id

  // UUIDs
  new Uint8Array(buf, 28, 16).set(sourceBytes);
  new Uint8Array(buf, 44, 16).set(targetBytes);

  view.setUint8(60, msg.priority);           // priority
  view.setUint8(61, 16);                     // ttl
  view.setUint16(62, 0, true);              // reserved
  view.setUint32(64, payloadArr.byteLength, true); // payload_length

  // Payload
  new Uint8Array(buf, 68).set(new Uint8Array(payloadArr));

  // CRC32 (simplified: first 4 bytes of SHA256 as placeholder)
  const crc = simpleHash(new Uint8Array(buf, 0, HEADER_SIZE + payloadArr.byteLength));
  view.setUint32(68 + payloadArr.byteLength, crc, true);

  return buf;
}

export function decodeMessage(buffer: ArrayBuffer): SsiMessage | null {
  if (buffer.byteLength < HEADER_SIZE) return null;
  const view = new DataView(buffer);
  const magic = view.getUint32(0, true);
  if (magic !== MAGIC) return null;

  const payloadLength = view.getUint32(64, true);
  const expectedLen = HEADER_SIZE + payloadLength + 4;
  if (buffer.byteLength < expectedLen) return null;

  // Extract UUIDs
  const sourceUuid = bytesToHex(new Uint8Array(buffer, 28, 16));
  const targetUuid = bytesToHex(new Uint8Array(buffer, 44, 16));

  return {
    type: view.getUint32(8, true),
    id: Number(view.getBigUint64(12, true)),
    timestamp: timestampUs(),
    sourceUuid,
    targetUuid,
    payload: buffer.slice(68, 68 + payloadLength),
    priority: view.getUint8(60),
  };
}

// =========================================================================
// Priority Queue
// =========================================================================

export class PriorityMessageQueue {
  private buckets: SsiMessage[][] = Array.from({ length: 256 }, () => []);
  private _length = 0;

  push(msg: SsiMessage): void {
    const priority = Math.min(255, Math.max(0, msg.priority));
    this.buckets[priority].push(msg);
    this._length++;
  }

  shift(): SsiMessage | undefined {
    for (let p = 255; p >= 0; p--) {
      if (this.buckets[p].length > 0) {
        this._length--;
        return this.buckets[p].shift();
      }
    }
    return undefined;
  }

  get length(): number {
    return this._length;
  }

  clear(): void {
    this.buckets = Array.from({ length: 256 }, () => []);
    this._length = 0;
  }
}

// =========================================================================
// SSI Service Bus
// =========================================================================

export class SsiServiceBus extends SsiBaseComponent {
  private registry: ComponentRegistry;
  private queues = new Map<UUID, PriorityMessageQueue>();
  private stats = {
    messagesSent: 0,
    messagesDelivered: 0,
    messagesDropped: 0,
    routingErrors: 0,
  };

  constructor(registry: ComponentRegistry) {
    super({
      name: 'ssi-service-bus',
      type: 'system',
      version: '1.0.0',
      vendor: 'AI-TP Foundation',
    });
    this.registry = registry;
  }

  async send(msg: SsiMessage): Promise<SsiErrorCode> {
    this.stats.messagesSent++;

    // Validate message
    if (!msg.sourceUuid || !msg.targetUuid) {
      this.stats.routingErrors++;
      return SsiErrorCode.INVALID;
    }

    // Route to target
    const target = this.registry.findByUuid(msg.targetUuid);
    if (!target) {
      this.stats.routingErrors++;
      this.stats.messagesDropped++;
      this.log(`Target not found: ${msg.targetUuid}`, 'warn');
      return SsiErrorCode.NOT_FOUND;
    }

    const queue = this.getOrCreateQueue(msg.targetUuid);
    queue.push(msg);
    this.stats.messagesDelivered++;

    // Deliver to component
    target.sendMessage(msg);
    return SsiErrorCode.OK;
  }

  /** Broadcast a message to all components of a given type */
  async broadcast(msg: SsiMessage, componentType?: string): Promise<void> {
    const targets = componentType
      ? this.registry.findByType(componentType)
      : this.registry.listEntries().map(e => this.registry.findByUuid(e.uuid)!)
          .filter((c): c is NonNullable<typeof c> => c !== undefined);

    for (const target of targets) {
      const busMsg = { ...msg, targetUuid: target.getId().uuid };
      const queue = this.getOrCreateQueue(target.getId().uuid);
      queue.push(busMsg);
      target.sendMessage(busMsg);
      this.stats.messagesDelivered++;
    }
  }

  /** Get pending message count for a component */
  getQueueLength(uuid: UUID): number {
    return this.queues.get(uuid)?.length ?? 0;
  }

  /** Get bus statistics */
  getStats() {
    return { ...this.stats };
  }

  private getOrCreateQueue(uuid: UUID): PriorityMessageQueue {
    if (!this.queues.has(uuid)) {
      this.queues.set(uuid, new PriorityMessageQueue());
    }
    return this.queues.get(uuid)!;
  }

  protected async onStart(): Promise<void> {
    this.log(`SSI Service Bus started (${this.registry.size} components registered)`);
  }

  protected async onStop(): Promise<void> {
    this.queues.clear();
    this.log(`Bus stopped. Stats: ${JSON.stringify(this.stats)}`);
  }
}

// =========================================================================
// Utilities
// =========================================================================

function hexToBytes(hex: string): Uint8Array {
  const clean = hex.replace(/-/g, '');
  const bytes = new Uint8Array(16);
  for (let i = 0; i < 16; i++) {
    bytes[i] = parseInt(clean.substr(i * 2, 2), 16);
  }
  return bytes;
}

function bytesToHex(bytes: Uint8Array): string {
  const parts: string[] = [];
  for (let i = 0; i < 16; i++) {
    parts.push(bytes[i].toString(16).padStart(2, '0'));
  }
  return `${parts.slice(0, 4).join('')}-${parts.slice(4, 6).join('')}-${parts.slice(6, 8).join('')}-${parts.slice(8, 10).join('')}-${parts.slice(10).join('')}`;
}

function simpleHash(data: Uint8Array): number {
  let hash = 0x811c9dc5;
  for (let i = 0; i < data.length; i++) {
    hash ^= data[i];
    hash = Math.imul(hash, 0x01000193);
  }
  return hash >>> 0;
}
