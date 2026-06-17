import { describe, it, expect, beforeEach } from 'vitest';
import { SsiErrorCode } from '../../core/src/index';
import { SecurityModule, SsiCryptoAlgorithm } from '../src/index';

describe('SecurityModule', () => {
  let sec: SecurityModule;

  beforeEach(async () => {
    sec = new SecurityModule();
    await sec.init();
    await sec.start();
  });

  describe('identity & keys', () => {
    it('should generate an identity key pair', async () => {
      const { keys, error } = await sec.generateIdentity();
      expect(error).toBe(SsiErrorCode.OK);
      expect(keys).not.toBeNull();
      expect(keys!.publicKey.byteLength).toBeGreaterThan(0);
      expect(keys!.privateKey.byteLength).toBeGreaterThan(0);
      expect(keys!.algorithm).toBe(SsiCryptoAlgorithm.ECDSA_P256);
    });

    it('should get public key after generation', async () => {
      expect((await sec.generateIdentity()).error).toBe(SsiErrorCode.OK);
      const { key } = sec.getPublicKey();
      expect(key).not.toBeNull();
      expect(key!.byteLength).toBeGreaterThan(0);
    });

    it('should return NOT_FOUND before key generation', () => {
      const { key, error } = sec.getPublicKey();
      expect(key).toBeNull();
      expect(error).toBe(SsiErrorCode.NOT_FOUND);
    });

    it('should sign data', async () => {
      await sec.generateIdentity();
      const data = new TextEncoder().encode('test data').buffer;
      const { signature, error } = await sec.sign(data);
      expect(error).toBe(SsiErrorCode.OK);
      expect(signature).not.toBeNull();
      expect(signature!.byteLength).toBeGreaterThan(0);
    });

    it('should verify valid signatures', async () => {
      await sec.generateIdentity();
      const data = new TextEncoder().encode('verify me').buffer;
      const { signature } = await sec.sign(data)!;
      const { key } = sec.getPublicKey();

      const { valid } = await sec.verify(data, signature!, key!);
      expect(valid).toBe(true);
    });

    it('should reject tampered data', async () => {
      await sec.generateIdentity();
      const data = new TextEncoder().encode('original').buffer;
      const { signature } = await sec.sign(data)!;
      const { key } = sec.getPublicKey();

      const tampered = new TextEncoder().encode('tampered').buffer;
      const { valid } = await sec.verify(tampered, signature!, key!);
      expect(valid).toBe(false);
    });

    it('should return NOT_FOUND when signing without keys', async () => {
      const { signature, error } = await sec.sign(new ArrayBuffer(0));
      expect(signature).toBeNull();
      expect(error).toBe(SsiErrorCode.NOT_FOUND);
    });
  });

  describe('encryption', () => {
    it('should encrypt and decrypt data', async () => {
      const key = crypto.getRandomValues(new Uint8Array(32)); // AES-256
      const plaintext = new TextEncoder().encode('secret message').buffer;

      const { ciphertext, error: encError } = await sec.encrypt(plaintext, key.buffer);
      expect(encError).toBe(SsiErrorCode.OK);
      expect(ciphertext).not.toBeNull();

      const { plaintext: decrypted, error: decError } = await sec.decrypt(ciphertext!, key.buffer);
      expect(decError).toBe(SsiErrorCode.OK);
      expect(new TextDecoder().decode(decrypted!)).toBe('secret message');
    });

    it('should produce different ciphertexts for same plaintext', async () => {
      const key = crypto.getRandomValues(new Uint8Array(32));
      const data = new TextEncoder().encode('same data').buffer;

      const { ciphertext: c1 } = await sec.encrypt(data, key.buffer);
      const { ciphertext: c2 } = await sec.encrypt(data, key.buffer);

      // IV is random (first 12 bytes), so ciphertexts should differ
      const c1arr = new Uint8Array(c1!);
      const c2arr = new Uint8Array(c2!);
      expect(c1arr.length).toBe(c2arr.length);
      const same = c1arr.every((b, i) => b === c2arr[i]);
      expect(same).toBe(false);
    });
  });

  describe('permissions', () => {
    it('should grant and check permissions', () => {
      const uuid = 'comp-001';
      expect(sec.grantPermission(uuid, 'ssi-db:read')).toBe(SsiErrorCode.OK);

      const { allowed } = sec.checkPermission(uuid, 'ssi-db', 'read');
      expect(allowed).toBe(true);
    });

    it('should deny unregistered permissions', () => {
      const { allowed } = sec.checkPermission('unknown', 'ssi-db', 'write');
      expect(allowed).toBe(false);
    });

    it('should revoke permissions', () => {
      const uuid = 'comp-002';
      sec.grantPermission(uuid, 'ssi-db:write');
      expect(sec.checkPermission(uuid, 'ssi-db', 'write').allowed).toBe(true);

      sec.revokePermission(uuid, 'ssi-db:write');
      expect(sec.checkPermission(uuid, 'ssi-db', 'write').allowed).toBe(false);
    });

    it('should handle granular permissions', () => {
      const uuid = 'comp-003';
      sec.grantPermission(uuid, 'ssi-db:read');
      expect(sec.checkPermission(uuid, 'ssi-db', 'read').allowed).toBe(true);
      expect(sec.checkPermission(uuid, 'ssi-db', 'write').allowed).toBe(false);
    });

    it('should reject invalid permission format', () => {
      expect(sec.grantPermission('x', 'invalid')).toBe(SsiErrorCode.INVALID);
    });

    it('should list permissions', () => {
      const uuid = 'comp-004';
      sec.grantPermission(uuid, 'ssi-db:read');
      sec.grantPermission(uuid, 'ssi-net:connect');
      const list = sec.listPermissions(uuid);
      expect(list).toHaveLength(2);
    });
  });

  describe('audit log', () => {
    it('should record permission checks', () => {
      sec.checkPermission('audit-test', 'ssi-db', 'read');
      const log = sec.getAuditLog();
      expect(log.length).toBeGreaterThan(0);
      expect(log[log.length - 1].componentUuid).toBe('audit-test');
      expect(log[log.length - 1].allowed).toBe(false);
    });

    it('should filter by timestamp', () => {
      const before = Date.now() - 1;
      sec.checkPermission('test', 'ssi-db', 'read');
      const after = Date.now() + 1; // ensure it's after the event

      expect(sec.getAuditLog(after * 1000)).toHaveLength(0); // nothing after
      expect(sec.getAuditLog(before * 1000).length).toBeGreaterThan(0);
    });

    it('should cap at max entries', () => {
      for (let i = 0; i < 12000; i++) {
        sec.logEvent({
          timestamp: Date.now(), componentUuid: 'spam',
          action: 'test', interface_: 'x', operation: 'y',
          allowed: true, reason: 'test',
        });
      }
      expect(sec.getAuditLog().length).toBeLessThanOrEqual(10000);
    });
  });
});
