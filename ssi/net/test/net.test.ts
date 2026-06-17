/**
 * SSI-NET: Network Interface Tests
 *
 * Tests for the cloudflared sidecar HTTP binding.
 * Most tests check structure, defaults, and error handling
 * (no sidecar running in CI).
 */

import { describe, it, expect } from 'vitest';
import {
  NetComponent,
  SidecarClient,
  SsiConnectOptions,
  SsiNetworkRequest,
  SsiNetworkResponse,
  SsiSidecarStatus,
  DEFAULT_SIDECAR_ADDR,
  SsiErrorCode,
} from '../src/index';

describe('NetComponent (SSI-NET)', () => {
  // ── Default state ──

  it('should create with default config', () => {
    const net = new NetComponent();
    expect(net.id.name).toBe('network-layer');
    expect(net.id.type).toBe('net');
    expect(net.id.version).toBe('1.0.0');
    expect(net.isConnected).toBe(false);
    expect(net.backendType).toBe('cloudflare');
  });

  it('should accept custom sidecar URL', () => {
    const net = new NetComponent('http://localhost:9999');
    expect(net.sidecar.endpoint).toBe('http://localhost:9999');
  });

  it('should have correct default endpoint', () => {
    const net = new NetComponent();
    expect(net.sidecar.endpoint).toBe(DEFAULT_SIDECAR_ADDR);
  });

  // ── SidecarClient structure ──

  it('should have SidecarClient with all API methods', () => {
    const client = new SidecarClient();
    expect(typeof client.ping).toBe('function');
    expect(typeof client.getStatus).toBe('function');
    expect(typeof client.getStats).toBe('function');
    expect(typeof client.getLogs).toBe('function');
    expect(typeof client.start).toBe('function');
    expect(typeof client.stop).toBe('function');
    expect(typeof client.getMetrics).toBe('function');
    expect(typeof client.httpRequest).toBe('function');
  });

  it('should strip trailing slash from endpoint', () => {
    const client = new SidecarClient('http://localhost:8081/');
    expect(client.endpoint).toBe('http://localhost:8081');
  });

  // ── Error handling (no sidecar) ──

  it('should return NOT_FOUND when sidecar is unreachable', async () => {
    const net = new NetComponent('http://localhost:19999');
    net.init({});
    net.start();

    const err = await net.connect('cloudflare');
    expect(err).toBe(SsiErrorCode.NOT_FOUND);

    await net.stop();
    net.destroy();
  });

  it('should handle getStatus error gracefully', async () => {
    const client = new SidecarClient('http://localhost:19999');
    const result = await client.getStatus();
    expect(result.status).toBeNull();
    expect(result.error).toBeTruthy();
  });

  it('should handle getStats error gracefully', async () => {
    const client = new SidecarClient('http://localhost:19999');
    const result = await client.getStats();
    expect(result.stats).toBeNull();
    expect(result.error).toBeTruthy();
  });

  it('should handle getLogs error gracefully', async () => {
    const client = new SidecarClient('http://localhost:19999');
    const result = await client.getLogs();
    expect(result.logs).toEqual([]);
    expect(result.error).toBeTruthy();
  });

  it('should handle httpRequest error when not connected', async () => {
    const net = new NetComponent();
    const result = await net.httpRequest({
      url: 'http://example.com',
      method: 'GET',
      headers: {},
      timeoutMs: 5000,
    });
    expect(result.response).toBeNull();
    expect(result.error).toContain('not connected');
  });

  it('should handle httpRequest from client gracefully', async () => {
    const client = new SidecarClient('http://localhost:19999');
    const result = await client.httpRequest({
      url: 'http://localhost:19999/nonexistent',
      method: 'GET',
      headers: {},
      timeoutMs: 1000,
    });
    // SidecarClient.httpRequest uses fetch() directly (not sidecar proxy),
    // so a bad URL returns error
    expect(result.response).toBeNull();
    expect(result.error).toBeTruthy();
  });
});

describe('SSI-NET data types', () => {
  it('should have correct DEFAULT_SIDECAR_ADDR', () => {
    expect(DEFAULT_SIDECAR_ADDR).toBe('http://localhost:8081');
  });

  it('should support all backend type strings', () => {
    const backends: string[] = [
      'cloudflare', 'tcp-relay', 'skynet-p2p', 'http-proxy', 'socks5',
      'failover', 'smart-router', 'gre', 'packet-tunnel', 'udp-tunnel',
      'webrtc', 'quic', 'dns-tunnel', 'icmp-tunnel', 'ssh-reverse',
      'dtls', 'wireguard', 'rtsp', 'rtmp', 'sftp', 'mqtt',
    ];
    expect(backends.length).toBe(21);
    expect(backends).toContain('cloudflare');
    expect(backends).toContain('dns-tunnel');
    expect(backends).toContain('wireguard');
  });
});

describe('SsiConnectOptions defaults', () => {
  it('should allow partial construction', () => {
    const opts: Partial<SsiConnectOptions> = {
      timeoutMs: 10000,
      useRelay: true,
    };
    expect(opts.timeoutMs).toBe(10000);
    expect(opts.useRelay).toBe(true);
    // undefined fields should be optional
    expect(opts.encryption).toBeUndefined();
    expect(opts.useQuic).toBeUndefined();
  });
});

describe('SsiNetworkRequest / Response', () => {
  it('should construct a request', () => {
    const req: SsiNetworkRequest = {
      url: 'https://api.example.com/data',
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      timeoutMs: 5000,
    };
    expect(req.url).toBe('https://api.example.com/data');
    expect(req.method).toBe('POST');
  });

  it('should construct a response', () => {
    const resp: SsiNetworkResponse = {
      statusCode: 200,
      headers: { 'Content-Type': 'application/json' },
      body: new ArrayBuffer(0),
      elapsedMs: 123,
    };
    expect(resp.statusCode).toBe(200);
    expect(resp.elapsedMs).toBe(123);
  });
});
