/**
 * Browser Networking Abstraction
 *
 * Provides a unified networking layer that works across browser, Node.js,
 * and Web Worker environments. Abstracts away the differences between:
 * - HTTP/fetch in browsers vs. http/https in Node.js
 * - WebSocket in browsers vs. ws in Node.js
 * - WebRTC (browser-only for direct P2P)
 *
 * This module is designed to be compatible with Emscripten's virtual
 * networking layer (emscripten_websocket, emscripten_fetch, etc.).
 */

import { Logger, detectEnvironment, BrowserNetwork } from './types';

export type { BrowserNetwork };

/**
 * Cross-platform network abstraction for WASM modules
 */
export class BrowserNetworkImpl implements BrowserNetwork {
  private static instance: BrowserNetworkImpl;
  private log: Logger;
  private env = detectEnvironment();

  /** WebSocket connections pool for reuse */
  private wsPool = new Map<string, WebSocket>();

  /** Active fetch controllers (for abort) */
  private activeControllers = new Map<string, AbortController>();

  private constructor() {
    this.log = new Logger('info');
  }

  static getInstance(): BrowserNetworkImpl {
    if (!BrowserNetworkImpl.instance) {
      BrowserNetworkImpl.instance = new BrowserNetworkImpl();
    }
    return BrowserNetworkImpl.instance;
  }

  // =========================================================================
  // HTTP/HTTPS Fetch
  // =========================================================================

  async fetch(url: string, options?: RequestInit): Promise<Response> {
    this.log.debug(`fetch: ${options?.method || 'GET'} ${url}`);

    // Node.js: fall back to native http module if needed
    if (this.env === 'node' && url.startsWith('http')) {
      return this.nodeFetch(url, options);
    }

    // Browser: use native fetch with timeout
    const controller = new AbortController();
    const requestId = `${Date.now()}-${Math.random().toString(36).slice(2)}`;
    this.activeControllers.set(requestId, controller);

    const timeout = setTimeout(() => {
      controller.abort();
      this.log.warn(`Fetch timeout: ${url}`);
    }, 30000);

    try {
      const response = await fetch(url, {
        ...options,
        signal: controller.signal,
        // Emscripten compatibility: ensure CORS mode
        credentials: options?.credentials || 'same-origin',
      });
      return response;
    } finally {
      clearTimeout(timeout);
      this.activeControllers.delete(requestId);
    }
  }

  /** HTTP fetch wrapper for Emscripten's emscripten_fetch */
  async emscriptenFetch(url: string, attributes: Record<string, any>): Promise<ArrayBuffer> {
    this.log.debug(`emscripten_fetch: ${url}`);

    const response = await this.fetch(url, {
      method: attributes.requestMethod || 'GET',
      headers: attributes.requestHeaders || {},
    });

    if (!response.ok) {
      throw new Error(`fetch failed: ${response.status} ${response.statusText}`);
    }

    return response.arrayBuffer();
  }

  // =========================================================================
  // WebSocket
  // =========================================================================

  connectWebSocket(url: string, protocols?: string[]): WebSocket {
    this.log.debug(`WebSocket connect: ${url}`);

    // Check pool for existing connection
    const poolKey = `${url}::${protocols?.join(',') || ''}`;
    const existing = this.wsPool.get(poolKey);
    if (existing && existing.readyState === WebSocket.OPEN) {
      this.log.debug(`WebSocket pool hit: ${url}`);
      return existing;
    }

    // Create new connection
    let ws: WebSocket;
    if (this.env === 'node') {
      // Node.js: require 'ws' dynamically
      const WebSocketImpl = require('ws');
      ws = new WebSocketImpl(url, protocols) as unknown as WebSocket;
    } else {
      ws = new WebSocket(url, protocols);
    }

    ws.addEventListener('close', () => {
      this.wsPool.delete(poolKey);
    });

    this.wsPool.set(poolKey, ws);
    return ws;
  }

  // =========================================================================
  // WebRTC
  // =========================================================================

  createPeerConnection(config?: RTCConfiguration): RTCPeerConnection {
    this.log.debug('Creating RTCPeerConnection');
    return new RTCPeerConnection(config);
  }

  // =========================================================================
  // Network status
  // =========================================================================

  async isOnline(): Promise<boolean> {
    if (this.env === 'browser') {
      return navigator.onLine;
    }
    // For Node.js, try a simple DNS resolution
    try {
      const dns = await import('dns');
      await dns.promises.resolve('example.com');
      return true;
    } catch {
      return false;
    }
  }

  async getEstimatedSpeed(): Promise<number> {
    // Rough estimate by measuring download time
    const start = performance.now();
    try {
      const response = await this.fetch('https://www.google.com/generate_204', {
        method: 'HEAD',
      });
      const timing = performance.now() - start;
      // Rough: assume 1KB response, speed = 1KB / time
      return Math.max(1000, (1024 * 8) / (timing / 1000)); // bits per second
    } catch {
      return 1000000; // default 1 Mbps fallback
    }
  }

  // =========================================================================
  // Emscripten Compatibility Layer
  // =========================================================================

  /**
   * Create a socket-like interface for Emscripten's emscripten_websocket API.
   * This allows C/C++ code compiled with Emscripten to use WebSockets.
   */
  createSocketBridge(ws: WebSocket): EmscriptenSocketBridge {
    return new EmscriptenSocketBridge(ws);
  }

  /** Abort all active fetches */
  abortAll(): void {
    for (const [, controller] of this.activeControllers) {
      controller.abort();
    }
    this.activeControllers.clear();
  }

  /** Close all pooled WebSocket connections */
  closeAllWebSockets(): void {
    for (const [, ws] of this.wsPool) {
      ws.close();
    }
    this.wsPool.clear();
  }

  // =========================================================================
  // Node.js fetch fallback
  // =========================================================================

  private async nodeFetch(url: string, options?: RequestInit): Promise<Response> {
    const http = url.startsWith('https') ? await import('https') : await import('http');
    return new Promise((resolve, reject) => {
      const parsedUrl = new URL(url);
      const req = http.request(
        {
          hostname: parsedUrl.hostname,
          port: parsedUrl.port || (url.startsWith('https') ? 443 : 80),
          path: parsedUrl.pathname + parsedUrl.search,
          method: options?.method || 'GET',
          headers: (options?.headers as Record<string, string>) || {},
          rejectUnauthorized: false,
        },
        (res: any) => {
          const chunks: Buffer[] = [];
          res.on('data', (chunk: Buffer) => chunks.push(chunk));
          res.on('end', () => {
            const body = Buffer.concat(chunks);
            resolve(new Response(body, {
              status: res.statusCode,
              statusText: res.statusMessage,
              headers: new Headers(res.headers as Record<string, string>),
            }));
          });
        }
      );
      req.on('error', reject);
      req.end();
    });
  }
}

/**
 * Bridge between Emscripten's C WebSocket API and browser WebSocket.
 * Allows WASM-compiled C code that uses emscripten_websocket to work
 * seamlessly in the browser.
 */
class EmscriptenSocketBridge {
  private ws: WebSocket;
  private onMessage: ((data: ArrayBuffer) => void) | null = null;
  private onOpen: (() => void) | null = null;
  private onClose: (() => void) | null = null;
  private onError: ((err: Event) => void) | null = null;

  constructor(ws: WebSocket) {
    this.ws = ws;

    ws.addEventListener('message', (event: MessageEvent) => {
      if (event.data instanceof ArrayBuffer) {
        this.onMessage?.(event.data);
      }
    });

    ws.addEventListener('open', () => this.onOpen?.());
    ws.addEventListener('close', () => this.onClose?.());
    ws.addEventListener('error', (err) => this.onError?.(err));
  }

  send(data: ArrayBuffer | string): void {
    this.ws.send(data);
  }

  close(): void {
    this.ws.close();
  }

  get readyState(): number {
    return this.ws.readyState;
  }

  // Callback setters (called from Emscripten glue code)
  setOnMessage(cb: (data: ArrayBuffer) => void): void { this.onMessage = cb; }
  setOnOpen(cb: () => void): void { this.onOpen = cb; }
  setOnClose(cb: () => void): void { this.onClose = cb; }
  setOnError(cb: (err: Event) => void): void { this.onError = cb; }
}

/** Convenience export */
export const browserNetwork = BrowserNetworkImpl.getInstance();
export default BrowserNetworkImpl;
