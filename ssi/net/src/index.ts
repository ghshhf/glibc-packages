/**
 * SSI-NET: Network Interface Binding
 *
 * TypeScript binding for the cloudflared/sidecar HTTP API.
 * This is a lightweight wrapper — NOT a reimplementation.
 * The sidecar does the actual networking; this module controls it.
 *
 * Future: replace with native node project when refactored.
 *
 * SSI Interfaces implemented:
 *   - SSI-NET: Tunnel management, proxy control, node discovery
 */

import {
  SsiBaseComponent,
  SsiComponentConfig,
  SsiErrorCode,
} from '../../core/src/index';

export { SsiErrorCode } from '../../core/src/index';

// =========================================================================
// SSI-NET Data Structures (mirrors SPEC-INTERFACE.md §5)
// =========================================================================

export interface SsiConnectOptions {
  timeoutMs: number;
  encryption: number;   // 0=none, 1=auto, 2=required
  useRelay: boolean;
  useQuic: boolean;
}

export interface SsiNodeInfo {
  nodeId: string;
  address: string;
  addresses: string[];
  natType: number;
  lastSeen: number;
  latencyMs: number;
  score: number;
}

export interface SsiNetworkRequest {
  url: string;
  method: string;
  headers: Record<string, string>;
  body?: ArrayBuffer;
  timeoutMs: number;
}

export interface SsiNetworkResponse {
  statusCode: number;
  headers: Record<string, string>;
  body: ArrayBuffer;
  elapsedMs: number;
}

export interface SsiSidecarStatus {
  state: string;
  backendType: string;
  backendName: string;
  uptime: number;
}

export interface SsiSidecarStats {
  connectionsActive: number;
  bytesSent: number;
  bytesRecv: number;
  errorsTotal: number;
  failoversTotal: number;
  backendAvailable: boolean;
  latencyP50: number;
  latencyP99: number;
}

export type BackendType =
  | 'cloudflare'
  | 'tcp-relay'
  | 'skynet-p2p'
  | 'http-proxy'
  | 'socks5'
  | 'failover'
  | 'smart-router'
  | 'gre'
  | 'packet-tunnel'
  | 'udp-tunnel'
  | 'webrtc'
  | 'quic'
  | 'dns-tunnel'
  | 'icmp-tunnel'
  | 'ssh-reverse'
  | 'dtls'
  | 'wireguard'
  | 'rtsp'
  | 'rtmp'
  | 'sftp'
  | 'mqtt';

// =========================================================================
// Default constants
// =========================================================================

export const DEFAULT_SIDECAR_ADDR = 'http://localhost:8081';
export const DEFAULT_CONNECT_TIMEOUT = 30000;

// =========================================================================
// Sidecar HTTP Client
// =========================================================================

export class SidecarClient {
  private baseUrl: string;

  constructor(baseUrl: string = DEFAULT_SIDECAR_ADDR) {
    this.baseUrl = baseUrl.replace(/\/+$/, '');
  }

  /** Get the base URL */
  get endpoint(): string {
    return this.baseUrl;
  }

  /** Check if the sidecar is reachable */
  async ping(): Promise<boolean> {
    try {
      const res = await fetch(`${this.baseUrl}/api/status`, {
        signal: AbortSignal.timeout(5000),
      });
      return res.ok;
    } catch {
      return false;
    }
  }

  /** Get sidecar status */
  async getStatus(): Promise<{ status: SsiSidecarStatus | null; error: string | null }> {
    try {
      const res = await fetch(`${this.baseUrl}/api/status`, {
        signal: AbortSignal.timeout(5000),
      });
      if (!res.ok) return { status: null, error: `HTTP ${res.status}` };
      const data = await res.json();
      return { status: data as SsiSidecarStatus, error: null };
    } catch (e: any) {
      return { status: null, error: e.message };
    }
  }

  /** Get sidecar stats (metrics snapshot) */
  async getStats(): Promise<{ stats: SsiSidecarStats | null; error: string | null }> {
    try {
      const res = await fetch(`${this.baseUrl}/api/stats`, {
        signal: AbortSignal.timeout(5000),
      });
      if (!res.ok) return { stats: null, error: `HTTP ${res.status}` };
      const data = await res.json();
      return { stats: data as SsiSidecarStats, error: null };
    } catch (e: any) {
      return { stats: null, error: e.message };
    }
  }

  /** Get recent logs */
  async getLogs(n: number = 50): Promise<{ logs: string[]; error: string | null }> {
    try {
      const res = await fetch(`${this.baseUrl}/api/logs?n=${n}`, {
        signal: AbortSignal.timeout(5000),
      });
      if (!res.ok) return { logs: [], error: `HTTP ${res.status}` };
      const data = await res.json();
      return { logs: Array.isArray(data) ? data : [], error: null };
    } catch (e: any) {
      return { logs: [], error: e.message };
    }
  }

  /** Start the sidecar tunnel */
  async start(): Promise<{ ok: boolean; error: string | null }> {
    try {
      const res = await fetch(`${this.baseUrl}/api/start`, {
        method: 'POST',
        signal: AbortSignal.timeout(10000),
      });
      return { ok: res.ok, error: res.ok ? null : `HTTP ${res.status}` };
    } catch (e: any) {
      return { ok: false, error: e.message };
    }
  }

  /** Stop the sidecar tunnel */
  async stop(): Promise<{ ok: boolean; error: string | null }> {
    try {
      const res = await fetch(`${this.baseUrl}/api/stop`, {
        method: 'POST',
        signal: AbortSignal.timeout(10000),
      });
      return { ok: res.ok, error: res.ok ? null : `HTTP ${res.status}` };
    } catch (e: any) {
      return { ok: false, error: e.message };
    }
  }

  /** Get Prometheus metrics text */
  async getMetrics(): Promise<{ metrics: string; error: string | null }> {
    try {
      const res = await fetch(`${this.baseUrl}/metrics`, {
        signal: AbortSignal.timeout(5000),
      });
      if (!res.ok) return { metrics: '', error: `HTTP ${res.status}` };
      const text = await res.text();
      return { metrics: text, error: null };
    } catch (e: any) {
      return { metrics: '', error: e.message };
    }
  }

  /** HTTP proxy request — send through the active tunnel */
  async httpRequest(req: SsiNetworkRequest): Promise<{ response: SsiNetworkResponse | null; error: string | null }> {
    const start = performance.now();
    try {
      const headers = new Headers(req.headers);
      const res = await fetch(req.url, {
        method: req.method,
        headers,
        body: req.body,
        signal: req.timeoutMs > 0 ? AbortSignal.timeout(req.timeoutMs) : undefined,
      });

      const body = await res.arrayBuffer();
      const respHeaders: Record<string, string> = {};
      res.headers.forEach((v, k) => { respHeaders[k] = v; });

      return {
        response: {
          statusCode: res.status,
          headers: respHeaders,
          body,
          elapsedMs: performance.now() - start,
        },
        error: null,
      };
    } catch (e: any) {
      return {
        response: null,
        error: e.message,
      };
    }
  }
}

// =========================================================================
// NetComponent — SSI Component Wrapper
// =========================================================================

export class NetComponent extends SsiBaseComponent {
  private client: SidecarClient;
  private connected = false;
  private _backendType: BackendType = 'cloudflare';

  constructor(sidecarUrl: string = DEFAULT_SIDECAR_ADDR) {
    super({ name: 'network-layer', type: 'net', version: '1.0.0', vendor: 'AI-TP' });
    this.client = new SidecarClient(sidecarUrl);
  }

  /** Get the sidecar client (for direct HTTP API access) */
  get sidecar(): SidecarClient {
    return this.client;
  }

  /** Get the configured backend type */
  get backendType(): BackendType {
    return this._backendType;
  }

  /** Check if connected to sidecar */
  get isConnected(): boolean {
    return this.connected;
  }

  // =========================================================================
  // SSI-NET: Tunnel Management
  // =========================================================================

  /** Connect to the sidecar and establish tunnel */
  async connect(backendType: BackendType = 'cloudflare', options?: Partial<SsiConnectOptions>): Promise<SsiErrorCode> {
    this.log(`connect: ${backendType}`);

    // Ping the sidecar
    const alive = await this.client.ping();
    if (!alive) {
      this.log('sidecar not reachable', 'error');
      return SsiErrorCode.NOT_FOUND;
    }

    // Start the tunnel
    const result = await this.client.start();
    if (!result.ok) {
      this.log(`start failed: ${result.error}`, 'error');
      return SsiErrorCode.GENERIC;
    }

    this._backendType = backendType;
    this.connected = true;
    return SsiErrorCode.OK;
  }

  /** Get sidecar status */
  async getTunnelStatus(): Promise<{ status: SsiSidecarStatus | null; error: string | null }> {
    return this.client.getStatus();
  }

  /** Get stats */
  async getStats(): Promise<{ stats: SsiSidecarStats | null; error: string | null }> {
    return this.client.getStats();
  }

  /** Get logs */
  async getLogs(n?: number): Promise<{ logs: string[]; error: string | null }> {
    return this.client.getLogs(n);
  }

  // =========================================================================
  // SSI-NET: HTTP Proxy (through sidecar tunnel)
  // =========================================================================

  /** Execute an HTTP request through the active tunnel */
  async httpRequest(req: SsiNetworkRequest): Promise<{ response: SsiNetworkResponse | null; error: string | null }> {
    if (!this.connected) {
      return { response: null, error: 'not connected to sidecar' };
    }
    return this.client.httpRequest(req);
  }

  // =========================================================================
  // SSI-CORE Lifecycle
  // =========================================================================

  protected async onInit(config: SsiComponentConfig): Promise<void> {
    const url = (config.sidecarUrl as string) || DEFAULT_SIDECAR_ADDR;
    this.client = new SidecarClient(url);
    this.log(`NetComponent init (sidecar: ${url})`);
  }

  protected async onStart(): Promise<void> {
    this.log('NetComponent started');
  }

  protected async onStop(): Promise<void> {
    if (this.connected) {
      await this.client.stop();
      this.connected = false;
    }
    this.log('NetComponent stopped');
  }

  protected onDestroy(): void {
    this.connected = false;
  }
}

// =========================================================================
// Convenience: default instance
// =========================================================================

let defaultNet: NetComponent | null = null;

export function getDefaultNet(): NetComponent {
  if (!defaultNet) {
    defaultNet = new NetComponent();
  }
  return defaultNet;
}
