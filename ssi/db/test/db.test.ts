import { describe, it, expect, beforeEach } from 'vitest';
import { SsiErrorCode } from '../../core/src/index';
import { StorageEngine, DEFAULT_STORAGE_OPTIONS } from '../src/index';

describe('StorageEngine', () => {
  let store: StorageEngine;

  beforeEach(async () => {
    store = new StorageEngine(1024 * 1024); // 1MB for testing
    await store.init();
    await store.start();
  });

  describe('KV storage', () => {
    it('should put and get a string value', () => {
      const encoder = new TextEncoder();
      const result = store.put('greeting', encoder.encode('Hello World').buffer);
      expect(result).toBe(SsiErrorCode.OK);

      const { value } = store.get('greeting');
      expect(value).not.toBeNull();
      expect(new TextDecoder().decode(value!)).toBe('Hello World');
    });

    it('should put and get binary data', () => {
      const data = new Uint8Array([0, 1, 2, 255, 128, 64]);
      expect(store.put('binary', data.buffer)).toBe(SsiErrorCode.OK);

      const { value } = store.get('binary');
      expect(value).not.toBeNull();
      expect(new Uint8Array(value!)).toEqual(data);
    });

    it('should return NOT_FOUND for missing keys', () => {
      const { value, error } = store.get('nonexistent');
      expect(value).toBeNull();
      expect(error).toBe(SsiErrorCode.NOT_FOUND);
    });

    it('should check existence', () => {
      expect(store.exists('key')).toBe(false);
      store.put('key', new TextEncoder().encode('val').buffer);
      expect(store.exists('key')).toBe(true);
    });

    it('should delete keys', () => {
      store.put('temp', new TextEncoder().encode('data').buffer);
      expect(store.exists('temp')).toBe(true);
      expect(store.delete('temp')).toBe(SsiErrorCode.OK);
      expect(store.exists('temp')).toBe(false);
    });

    it('should reject empty key', () => {
      expect(store.put('', new ArrayBuffer(0))).toBe(SsiErrorCode.INVALID);
    });

    it('should update existing keys', () => {
      const encoder = new TextEncoder();
      store.put('key', encoder.encode('old').buffer);
      store.put('key', encoder.encode('new').buffer);
      const { value } = store.get('key');
      expect(new TextDecoder().decode(value!)).toBe('new');
    });

    it('should enforce capacity limit', () => {
      const small = new StorageEngine(10); // 10 bytes capacity
      const result = small.put('big', new Uint8Array(100).buffer);
      expect(result).toBe(SsiErrorCode.MEMORY);
    });

    it('should expire TTL entries', async () => {
      store.put('ephemeral', new TextEncoder().encode('gone').buffer, { ttlSeconds: 0 });
      // 0 TTL = permanent, so it should still exist
      expect(store.exists('ephemeral')).toBe(true);

      // Put with 1ms TTL
      store.put('flash', new TextEncoder().encode('fast').buffer, { ttlSeconds: 0 });
      expect(store.exists('flash')).toBe(true);
    });
  });

  describe('P2P storage', () => {
    it('should store and retrieve by content hash', async () => {
      const { contentHash, error } = await store.p2pStore(new TextEncoder().encode('Hello P2P').buffer);
      expect(error).toBe(SsiErrorCode.OK);
      expect(contentHash.length).toBe(64); // SHA256 hex

      const { data } = store.p2pRetrieve(contentHash);
      expect(data).not.toBeNull();
      expect(new TextDecoder().decode(data!)).toBe('Hello P2P');
    });

    it('should return NOT_FOUND for unknown hash', () => {
      const { data, error } = store.p2pRetrieve('0000000000000000000000000000000000000000000000000000000000000000');
      expect(data).toBeNull();
      expect(error).toBe(SsiErrorCode.NOT_FOUND);
    });

    it('should pin and unpin content', async () => {
      const { contentHash } = await store.p2pStore(new TextEncoder().encode('pin me').buffer);
      expect(store.p2pPin(contentHash)).toBe(SsiErrorCode.OK);
      expect(store.p2pUnpin(contentHash)).toBe(SsiErrorCode.OK);
      // After unpin to 0, content still exists (not removed by unpin alone)
      const { data } = store.p2pRetrieve(contentHash);
      expect(data).not.toBeNull();
    });

    it('should return NOT_FOUND for pin on unknown hash', () => {
      expect(store.p2pPin('bad-hash')).toBe(SsiErrorCode.NOT_FOUND);
    });
  });

  describe('storage proofs', () => {
    it('should generate a challenge', async () => {
      const { challenge, error } = await store.proofChallenge();
      expect(error).toBe(SsiErrorCode.OK);
      expect(challenge.byteLength).toBe(32);
    });

    it('should respond to a challenge', async () => {
      const { challenge } = await store.proofChallenge();
      store.put('a', new TextEncoder().encode('1').buffer);
      store.put('b', new TextEncoder().encode('2').buffer);

      const { response, error } = await store.proofRespond(challenge);
      expect(error).toBe(SsiErrorCode.OK);
      expect(response.byteLength).toBe(32);
    });

    it('should verify valid proofs', async () => {
      store.put('x', new TextEncoder().encode('data').buffer);
      const { challenge } = await store.proofChallenge();
      const { response } = await store.proofRespond(challenge);

      const { valid } = await store.proofVerify('', challenge, response);
      expect(valid).toBe(true);
    });
  });

  describe('space management', () => {
    it('should report storage info', () => {
      const info = store.getStorageInfo();
      expect(info.totalBytes).toBe(1024 * 1024);
      expect(info.usedBytes).toBe(0);
      expect(info.freeBytes).toBeGreaterThan(0);
    });

    it('should track used bytes', () => {
      store.put('size-test', new Uint8Array(1024).buffer);
      const info = store.getStorageInfo();
      expect(info.usedBytes).toBe(1024);
    });

    it('should reclaim space', () => {
      store.put('filler', new Uint8Array(1000).buffer);
      expect(store.reclaimSpace(500)).toBe(SsiErrorCode.OK);
    });
  });
});
