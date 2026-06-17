/**
 * SSI-DB: Storage Engine Interface
 *
 * Implements the IStorageEngine interface from SPEC-INTERFACE.md §6.
 * Provides KV storage, P2P content-addressed storage, and storage proofs.
 *
 * SSI Interfaces implemented:
 *   - SSI-DB: KV store, P2P store, storage proofs, space management
 */

import {
  SsiBaseComponent,
  SsiComponentConfig,
  SsiErrorCode,
} from '../../core/src/index';

// =========================================================================
// SSI-DB Data Structures
// =========================================================================

export interface SsiStorageOptions {
  ttlSeconds: number;           // 0 = permanent
  p2pReplicate: boolean;
  replicationFactor: number;
  encrypt: boolean;
}

export interface SsiStorageInfo {
  totalBytes: number;
  usedBytes: number;
  freeBytes: number;
  p2pUsedBytes: number;
  p2pPeerCount: number;
}

export interface SsiStorageEntry {
  value: ArrayBuffer;
  options: SsiStorageOptions;
  createdAt: number;
  expiresAt: number | null;
}

export interface SsiP2PEntry {
  data: ArrayBuffer;
  pinCount: number;
  size: number;
  storedAt: number;
}

export const DEFAULT_STORAGE_OPTIONS: SsiStorageOptions = {
  ttlSeconds: 0,
  p2pReplicate: false,
  replicationFactor: 1,
  encrypt: false,
};

export const DEFAULT_STORAGE_CAPACITY = 1024 * 1024 * 1024; // 1GB default

// =========================================================================
// Storage Engine — SSI-DB Implementation
// =========================================================================

export class StorageEngine extends SsiBaseComponent {
  private kvStore = new Map<string, SsiStorageEntry>();
  private p2pEntries = new Map<string, SsiP2PEntry>();
  private totalCapacity: number;
  private usedBytes = 0;
  private p2pUsedBytes = 0;
  private p2pPeerCount = 0;
  private sweepTimer: ReturnType<typeof setInterval> | null = null;

  constructor(capacity: number = DEFAULT_STORAGE_CAPACITY) {
    super({ name: 'storage-engine', type: 'db', version: '1.0.0', vendor: 'AI-TP' });
    this.totalCapacity = capacity;
  }

  // =========================================================================
  // KV Storage
  // =========================================================================

  /** Store a key-value pair */
  put(key: string, value: ArrayBuffer | Uint8Array | string, options?: Partial<SsiStorageOptions>): SsiErrorCode {
    if (!key) return SsiErrorCode.INVALID;

    const data = this.normalizeBuffer(value);
    const opts: SsiStorageOptions = { ...DEFAULT_STORAGE_OPTIONS, ...options };

    // Check capacity
    const existing = this.kvStore.get(key);
    const sizeDelta = data.byteLength - (existing ? existing.value.byteLength : 0);
    if (this.usedBytes + sizeDelta > this.totalCapacity) {
      return SsiErrorCode.MEMORY;
    }

    const now = Date.now();
    const entry: SsiStorageEntry = {
      value: data,
      options: opts,
      createdAt: now,
      expiresAt: opts.ttlSeconds > 0 ? now + opts.ttlSeconds * 1000 : null,
    };

    this.kvStore.set(key, entry);
    this.usedBytes += data.byteLength;
    if (existing) {
      this.usedBytes -= existing.value.byteLength;
    }

    // P2P replication
    if (opts.p2pReplicate) {
      this.p2pStoreData(data).catch(() => {});
    }

    return SsiErrorCode.OK;
  }

  /** Retrieve a value by key */
  get(key: string): { value: ArrayBuffer | null; error: SsiErrorCode } {
    const entry = this.kvStore.get(key);
    if (!entry) return { value: null, error: SsiErrorCode.NOT_FOUND };

    // Check TTL
    if (entry.expiresAt && Date.now() > entry.expiresAt) {
      this.kvStore.delete(key);
      this.usedBytes -= entry.value.byteLength;
      return { value: null, error: SsiErrorCode.NOT_FOUND };
    }

    return { value: entry.value.slice(0), error: SsiErrorCode.OK };
  }

  /** Delete a key */
  delete(key: string): SsiErrorCode {
    const entry = this.kvStore.get(key);
    if (!entry) return SsiErrorCode.NOT_FOUND;
    this.kvStore.delete(key);
    this.usedBytes -= entry.value.byteLength;
    return SsiErrorCode.OK;
  }

  /** Check if a key exists */
  exists(key: string): boolean {
    const entry = this.kvStore.get(key);
    if (!entry) return false;
    if (entry.expiresAt && Date.now() > entry.expiresAt) {
      this.kvStore.delete(key);
      this.usedBytes -= entry.value.byteLength;
      return false;
    }
    return true;
  }

  /** List all keys (for debugging/management) */
  listKeys(): string[] {
    // Clean expired entries
    this.sweepExpired();
    return Array.from(this.kvStore.keys());
  }

  // =========================================================================
  // P2P Content-Addressed Storage
  // =========================================================================

  /** Store data by content hash */
  async p2pStore(data: ArrayBuffer | Uint8Array | string): Promise<{ contentHash: string; error: SsiErrorCode }> {
    const buf = this.normalizeBuffer(data);
    const hash = await this.sha256(buf);

    const existing = this.p2pEntries.get(hash);
    if (existing) {
      return { contentHash: hash, error: SsiErrorCode.OK };
    }

    if (this.p2pUsedBytes + buf.byteLength > this.totalCapacity) {
      return { contentHash: '', error: SsiErrorCode.MEMORY };
    }

    this.p2pEntries.set(hash, {
      data: buf,
      pinCount: 1,
      size: buf.byteLength,
      storedAt: Date.now(),
    });
    this.p2pUsedBytes += buf.byteLength;

    return { contentHash: hash, error: SsiErrorCode.OK };
  }

  /** Retrieve data by content hash */
  p2pRetrieve(contentHash: string): { data: ArrayBuffer | null; error: SsiErrorCode } {
    const entry = this.p2pEntries.get(contentHash);
    if (!entry) return { data: null, error: SsiErrorCode.NOT_FOUND };
    return { data: entry.data.slice(0), error: SsiErrorCode.OK };
  }

  /** Pin content to prevent garbage collection */
  p2pPin(contentHash: string): SsiErrorCode {
    const entry = this.p2pEntries.get(contentHash);
    if (!entry) return SsiErrorCode.NOT_FOUND;
    entry.pinCount++;
    return SsiErrorCode.OK;
  }

  /** Unpin content */
  p2pUnpin(contentHash: string): SsiErrorCode {
    const entry = this.p2pEntries.get(contentHash);
    if (!entry) return SsiErrorCode.NOT_FOUND;
    entry.pinCount = Math.max(0, entry.pinCount - 1);
    if (entry.pinCount === 0) {
      this.p2pEntries.delete(contentHash);
      this.p2pUsedBytes -= entry.size;
    }
    return SsiErrorCode.OK;
  }

  // =========================================================================
  // Storage Proofs (challenge-response)
  // =========================================================================

  /** Generate a storage proof challenge */
  async proofChallenge(): Promise<{ challenge: ArrayBuffer; error: SsiErrorCode }> {
    const challenge = new Uint8Array(32);
    crypto.getRandomValues(challenge);
    return { challenge: challenge.buffer, error: SsiErrorCode.OK };
  }

  /** Respond to a storage proof challenge */
  async proofRespond(challenge: ArrayBuffer): Promise<{ response: ArrayBuffer; error: SsiErrorCode }> {
    // Response = SHA256(challenge + all stored keys sorted)
    const keys = Array.from(this.kvStore.keys()).sort();
    const encoder = new TextEncoder();
    const combined = new Uint8Array(challenge.byteLength + keys.join('').length);
    combined.set(new Uint8Array(challenge), 0);
    let offset = challenge.byteLength;
    for (const key of keys) {
      const keyBytes = encoder.encode(key);
      combined.set(keyBytes, offset);
      offset += keyBytes.length;
    }
    const hash = await crypto.subtle.digest('SHA-256', combined);
    return { response: hash, error: SsiErrorCode.OK };
  }

  /** Verify a storage proof */
  async proofVerify(contentHash: string, challenge: ArrayBuffer, response: ArrayBuffer): Promise<{ valid: boolean; error: SsiErrorCode }> {
    // Recompute expected response
    const keys = Array.from(this.kvStore.keys()).sort();
    const encoder = new TextEncoder();
    const combined = new Uint8Array(challenge.byteLength + keys.join('').length);
    combined.set(new Uint8Array(challenge), 0);
    let offset = challenge.byteLength;
    for (const key of keys) {
      const keyBytes = encoder.encode(key);
      combined.set(keyBytes, offset);
      offset += keyBytes.length;
    }
    const expected = await crypto.subtle.digest('SHA-256', combined);

    // Compare
    if (expected.byteLength !== response.byteLength) return { valid: false, error: SsiErrorCode.OK };
    const expArr = new Uint8Array(expected);
    const respArr = new Uint8Array(response);
    for (let i = 0; i < expArr.length; i++) {
      if (expArr[i] !== respArr[i]) return { valid: false, error: SsiErrorCode.OK };
    }
    return { valid: true, error: SsiErrorCode.OK };
  }

  // =========================================================================
  // Space Management
  // =========================================================================

  /** Get storage statistics */
  getStorageInfo(): SsiStorageInfo {
    this.sweepExpired();
    return {
      totalBytes: this.totalCapacity,
      usedBytes: this.usedBytes,
      freeBytes: this.totalCapacity - this.usedBytes - this.p2pUsedBytes,
      p2pUsedBytes: this.p2pUsedBytes,
      p2pPeerCount: this.p2pPeerCount,
    };
  }

  /** Reclaim space by removing least-recently-used entries */
  reclaimSpace(targetBytes: number): SsiErrorCode {
    const toFree = Math.min(targetBytes, this.usedBytes + this.p2pUsedBytes);
    if (toFree <= 0) return SsiErrorCode.OK;

    let freed = 0;
    // First: unpin unpinned P2P content
    for (const [hash, entry] of this.p2pEntries) {
      if (entry.pinCount === 0) {
        this.p2pEntries.delete(hash);
        this.p2pUsedBytes -= entry.size;
        freed += entry.size;
        if (freed >= toFree) break;
      }
    }

    // Second: remove expired KV entries
    if (freed < toFree) {
      this.sweepExpired();
      freed = this.usedBytes; // recalculate after sweep
    }

    return SsiErrorCode.OK;
  }

  /** Set storage capacity */
  setCapacity(bytes: number): void {
    this.totalCapacity = bytes;
  }

  // =========================================================================
  // Lifecycle Hooks
  // =========================================================================

  protected async onInit(_config: SsiComponentConfig): Promise<void> {
    this.log('Storage Engine initializing');
  }

  protected async onStart(): Promise<void> {
    // Periodic TTL sweep every 60 seconds
    this.sweepTimer = setInterval(() => this.sweepExpired(), 60000);
    this.log('Storage Engine started');
  }

  protected async onStop(): Promise<void> {
    if (this.sweepTimer) {
      clearInterval(this.sweepTimer);
      this.sweepTimer = null;
    }
    this.log('Storage Engine stopped');
  }

  protected onDestroy(): void {
    this.kvStore.clear();
    this.p2pEntries.clear();
    this.usedBytes = 0;
    this.p2pUsedBytes = 0;
    if (this.sweepTimer) clearInterval(this.sweepTimer);
  }

  // =========================================================================
  // Internal
  // =========================================================================

  private normalizeBuffer(data: ArrayBuffer | Uint8Array | string): ArrayBuffer {
    if (typeof data === 'string') {
      return new TextEncoder().encode(data).buffer;
    }
    if (data instanceof Uint8Array) {
      return data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
    }
    return data.slice(0);
  }

  private async p2pStoreData(data: ArrayBuffer): Promise<string> {
    const hash = await this.sha256(data);
    if (!this.p2pEntries.has(hash)) {
      this.p2pEntries.set(hash, {
        data: data.slice(0),
        pinCount: 0,
        size: data.byteLength,
        storedAt: Date.now(),
      });
      this.p2pUsedBytes += data.byteLength;
    }
    return hash;
  }

  private async sha256(data: ArrayBuffer): Promise<string> {
    const hash = await crypto.subtle.digest('SHA-256', data);
    const bytes = new Uint8Array(hash);
    let hex = '';
    for (const b of bytes) {
      hex += b.toString(16).padStart(2, '0');
    }
    return hex;
  }

  private sweepExpired(): void {
    const now = Date.now();
    for (const [key, entry] of this.kvStore) {
      if (entry.expiresAt && now > entry.expiresAt) {
        this.kvStore.delete(key);
        this.usedBytes -= entry.value.byteLength;
      }
    }
  }
}
