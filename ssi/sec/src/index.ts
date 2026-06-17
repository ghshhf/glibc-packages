/**
 * SSI-SEC: Security Module Interface
 *
 * Implements the ISecurityModule interface from SPEC-INTERFACE.md §11.
 * Provides identity key management, encryption, permissions, and audit logging.
 *
 * SSI Interfaces implemented:
 *   - SSI-SEC: Identity & keys, encryption, permissions, audit
 */

import {
  SsiBaseComponent,
  SsiComponentConfig,
  SsiErrorCode,
} from '../../core/src/index';

// =========================================================================
// SSI-SEC Data Structures
// =========================================================================

export enum SsiCryptoAlgorithm {
  ED25519 = 0,
  SECP256K1 = 1,
  RSA4096 = 2,
}

export interface SsiKeyPair {
  publicKey: ArrayBuffer;
  privateKey: ArrayBuffer;
  algorithm: SsiCryptoAlgorithm;
}

export interface SsiPermission {
  componentUuid: string;
  interface_: string;
  operation: string;
  grantedAt: number;
  expiresAt: number | null;
}

export interface SsiSecurityEvent {
  timestamp: number;
  componentUuid: string;
  action: string;
  interface_: string;
  operation: string;
  allowed: boolean;
  reason: string;
}

// =========================================================================
// Security Module — SSI-SEC Implementation
// =========================================================================

export class SecurityModule extends SsiBaseComponent {
  private keyPair: SsiKeyPair | null = null;
  private permissions = new Map<string, SsiPermission[]>();
  private auditLog: SsiSecurityEvent[] = [];
  private readonly MAX_AUDIT_LOG = 10000;
  private auditEnabled = true;

  constructor() {
    super({ name: 'security-module', type: 'sec', version: '1.0.0', vendor: 'AI-TP' });
  }

  // =========================================================================
  // Identity & Keys
  // =========================================================================

  /** Generate a local identity key pair (using Web Crypto API) */
  async generateIdentity(): Promise<{ keys: SsiKeyPair | null; error: SsiErrorCode }> {
    try {
      const keyGen = await crypto.subtle.generateKey(
        { name: 'ECDSA', namedCurve: 'P-256' },
        true,
        ['sign', 'verify']
      );
      const publicKeyRaw = await crypto.subtle.exportKey('raw', keyGen.publicKey);
      const privateKeyRaw = await crypto.subtle.exportKey('pkcs8', keyGen.privateKey);

      this.keyPair = {
        publicKey: publicKeyRaw,
        privateKey: privateKeyRaw,
        algorithm: SsiCryptoAlgorithm.ED25519,
      };

      this.log('Identity key pair generated (ECDSA P-256)');
      return { keys: { ...this.keyPair, publicKey: publicKeyRaw.slice(0), privateKey: privateKeyRaw.slice(0) }, error: SsiErrorCode.OK };
    } catch (err: any) {
      this.log(`Key generation failed: ${err.message}`, 'error');
      return { keys: null, error: SsiErrorCode.GENERIC };
    }
  }

  /** Get the local public key */
  getPublicKey(): { key: ArrayBuffer | null; error: SsiErrorCode } {
    if (!this.keyPair) return { key: null, error: SsiErrorCode.NOT_FOUND };
    return { key: this.keyPair.publicKey.slice(0), error: SsiErrorCode.OK };
  }

  /** Sign data using the local private key */
  async sign(data: ArrayBuffer): Promise<{ signature: ArrayBuffer | null; error: SsiErrorCode }> {
    if (!this.keyPair) return { signature: null, error: SsiErrorCode.NOT_FOUND };

    try {
      const key = await crypto.subtle.importKey(
        'pkcs8', this.keyPair.privateKey,
        { name: 'ECDSA', namedCurve: 'P-256' },
        false, ['sign']
      );
      const signature = await crypto.subtle.sign(
        { name: 'ECDSA', hash: 'SHA-256' },
        key, data
      );
      return { signature, error: SsiErrorCode.OK };
    } catch (err: any) {
      this.log(`Sign failed: ${err.message}`, 'error');
      return { signature: null, error: SsiErrorCode.GENERIC };
    }
  }

  /** Verify a signature */
  async verify(data: ArrayBuffer, signature: ArrayBuffer, publicKey: ArrayBuffer): Promise<{ valid: boolean; error: SsiErrorCode }> {
    try {
      const key = await crypto.subtle.importKey(
        'raw', publicKey,
        { name: 'ECDSA', namedCurve: 'P-256' },
        false, ['verify']
      );
      const valid = await crypto.subtle.verify(
        { name: 'ECDSA', hash: 'SHA-256' },
        key, signature, data
      );
      return { valid, error: SsiErrorCode.OK };
    } catch {
      return { valid: false, error: SsiErrorCode.INVALID };
    }
  }

  // =========================================================================
  // Encryption
  // =========================================================================

  /** Encrypt data using AES-GCM */
  async encrypt(plaintext: ArrayBuffer, key: ArrayBuffer): Promise<{ ciphertext: ArrayBuffer | null; error: SsiErrorCode }> {
    try {
      const cryptoKey = await crypto.subtle.importKey(
        'raw', key,
        { name: 'AES-GCM' },
        false, ['encrypt']
      );
      const iv = crypto.getRandomValues(new Uint8Array(12));
      const encrypted = await crypto.subtle.encrypt(
        { name: 'AES-GCM', iv },
        cryptoKey, plaintext
      );
      // Prepend IV to ciphertext
      const combined = new Uint8Array(iv.byteLength + encrypted.byteLength);
      combined.set(iv, 0);
      combined.set(new Uint8Array(encrypted), iv.byteLength);
      return { ciphertext: combined.buffer, error: SsiErrorCode.OK };
    } catch (err: any) {
      this.log(`Encrypt failed: ${err.message}`, 'error');
      return { ciphertext: null, error: SsiErrorCode.GENERIC };
    }
  }

  /** Decrypt data using AES-GCM */
  async decrypt(ciphertext: ArrayBuffer, key: ArrayBuffer): Promise<{ plaintext: ArrayBuffer | null; error: SsiErrorCode }> {
    try {
      const cryptoKey = await crypto.subtle.importKey(
        'raw', key,
        { name: 'AES-GCM' },
        false, ['decrypt']
      );
      // Extract IV from first 12 bytes
      const combined = new Uint8Array(ciphertext);
      const iv = combined.slice(0, 12);
      const encrypted = combined.slice(12);
      const plaintext = await crypto.subtle.decrypt(
        { name: 'AES-GCM', iv },
        cryptoKey, encrypted
      );
      return { plaintext, error: SsiErrorCode.OK };
    } catch (err: any) {
      this.log(`Decrypt failed: ${err.message}`, 'error');
      return { plaintext: null, error: SsiErrorCode.GENERIC };
    }
  }

  // =========================================================================
  // Permission Management
  // =========================================================================

  /** Check if a component has permission for an operation */
  checkPermission(componentUuid: string, interface_: string, operation: string): { allowed: boolean; error: SsiErrorCode } {
    const perms = this.permissions.get(componentUuid);
    if (!perms) {
      this.audit({ componentUuid, action: 'check', interface_, operation, allowed: false, reason: 'No permissions registered' });
      return { allowed: false, error: SsiErrorCode.PERMISSION };
    }

    const now = Date.now();
    for (const p of perms) {
      if (p.interface_ === interface_ && p.operation === operation) {
        if (p.expiresAt && now > p.expiresAt) {
          // Permission expired
          continue;
        }
        this.audit({ componentUuid, action: 'check', interface_, operation, allowed: true, reason: 'Permission granted' });
        return { allowed: true, error: SsiErrorCode.OK };
      }
    }

    this.audit({ componentUuid, action: 'check', interface_, operation, allowed: false, reason: 'Permission denied' });
    return { allowed: false, error: SsiErrorCode.PERMISSION };
  }

  /** Grant a permission to a component */
  grantPermission(componentUuid: string, permission: string, ttlSeconds: number = 0): SsiErrorCode {
    const parts = permission.split(':');
    if (parts.length < 2) return SsiErrorCode.INVALID;

    const interface_ = parts[0];
    const operation = parts[1];
    const expiresAt = ttlSeconds > 0 ? Date.now() + ttlSeconds * 1000 : null;

    if (!this.permissions.has(componentUuid)) {
      this.permissions.set(componentUuid, []);
    }
    this.permissions.get(componentUuid)!.push({
      componentUuid,
      interface_,
      operation,
      grantedAt: Date.now(),
      expiresAt,
    });

    this.audit({ componentUuid, action: 'grant', interface_, operation, allowed: true, reason: 'Permission granted' });
    return SsiErrorCode.OK;
  }

  /** Revoke a permission */
  revokePermission(componentUuid: string, permission: string): SsiErrorCode {
    const parts = permission.split(':');
    if (parts.length < 2) return SsiErrorCode.INVALID;
    const interface_ = parts[0];
    const operation = parts[1];

    const perms = this.permissions.get(componentUuid);
    if (!perms) return SsiErrorCode.NOT_FOUND;

    const idx = perms.findIndex(p => p.interface_ === interface_ && p.operation === operation);
    if (idx === -1) return SsiErrorCode.NOT_FOUND;

    perms.splice(idx, 1);
    if (perms.length === 0) this.permissions.delete(componentUuid);

    this.audit({ componentUuid, action: 'revoke', interface_, operation, allowed: false, reason: 'Permission revoked' });
    return SsiErrorCode.OK;
  }

  /** List permissions for a component */
  listPermissions(componentUuid: string): SsiPermission[] {
    return this.permissions.get(componentUuid)?.slice() ?? [];
  }

  // =========================================================================
  // Audit Logging
  // =========================================================================

  /** Log a security event */
  logEvent(event: SsiSecurityEvent): SsiErrorCode {
    return this.audit(event) ? SsiErrorCode.OK : SsiErrorCode.GENERIC;
  }

  /** Get audit log entries */
  getAuditLog(sinceTimestamp: number = 0, maxCount: number = 100): SsiSecurityEvent[] {
    const filtered = this.auditLog.filter(e => e.timestamp >= sinceTimestamp);
    return filtered.slice(-maxCount);
  }

  /** Clear audit log */
  clearAuditLog(): void {
    this.auditLog = [];
  }

  /** Enable or disable audit logging */
  setAuditEnabled(enabled: boolean): void {
    this.auditEnabled = enabled;
  }

  // =========================================================================
  // Lifecycle
  // =========================================================================

  protected async onInit(_config: SsiComponentConfig): Promise<void> {
    this.log('Security Module initializing');
  }

  protected async onStart(): Promise<void> {
    this.log('Security Module started');
  }

  protected async onStop(): Promise<void> {
    this.log('Security Module stopped');
    this.auditLog = [];
  }

  protected onDestroy(): void {
    this.keyPair = null;
    this.permissions.clear();
    this.auditLog = [];
  }

  // =========================================================================
  // Internal
  // =========================================================================

  private audit(event: Omit<SsiSecurityEvent, 'timestamp'>): boolean {
    if (!this.auditEnabled) return false;
    this.auditLog.push({
      ...event,
      timestamp: Date.now() * 1000, // microseconds
    });
    if (this.auditLog.length > this.MAX_AUDIT_LOG) {
      this.auditLog.splice(0, this.auditLog.length - this.MAX_AUDIT_LOG);
    }
    return true;
  }
}
