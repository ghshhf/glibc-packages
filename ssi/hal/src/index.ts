/**
 * SSI-HAL: Hardware Abstraction Layer Interface
 *
 * Implements the IHardwareAbstraction interface from SPEC-INTERFACE.md §10.
 * Provides device info, sensors, battery, network status, and display info.
 *
 * SSI Interfaces implemented:
 *   - SSI-HAL: Device info, sensors, hardware control, battery, network, display
 */

import {
  SsiBaseComponent,
  SsiComponentConfig,
  SsiErrorCode,
} from '../../core/src/index';

// =========================================================================
// SSI-HAL Data Structures
// =========================================================================

export interface SsiDeviceInfo {
  platform: string;          // "browser", "native", "embedded"
  os: string;
  osVersion: string;
  arch: string;
  deviceModel: string;
  deviceVendor: string;
  cpuCores: number;
  totalRam: number;
  totalStorage: number;
  hasGpu: boolean;
  gpuVendor: string;
  batteryPowered: boolean;
  touchScreen: boolean;
}

export interface SsiBatteryInfo {
  level: number;             // 0-100
  status: number;            // 0=unknown, 1=charging, 2=discharging, 3=full
  temperatureCelsius: number;
}

export interface SsiNetworkStatus {
  online: boolean;
  connectionType: string;    // "wifi", "cellular", "ethernet", "unknown"
  signalStrength: number;    // 0-100
  ipAddress: string;
}

export interface SsiDisplayInfo {
  width: number;
  height: number;
  dpi: number;
  refreshRate: number;
  colorDepth: number;
}

// =========================================================================
// Hardware Abstraction Layer — SSI-HAL Implementation
// =========================================================================

export class HardwareAbstractionLayer extends SsiBaseComponent {
  private sensorDataCache = new Map<string, ArrayBuffer>();
  private lastSensorReadings = new Map<string, number>();

  constructor() {
    super({ name: 'hardware-abstraction', type: 'hal', version: '1.0.0', vendor: 'AI-TP' });
  }

  // =========================================================================
  // Device Info
  // =========================================================================

  /** Get comprehensive device information */
  getDeviceInfo(): SsiDeviceInfo {
    return {
      platform: this.detectPlatform(),
      os: this.detectOs(),
      osVersion: this.detectOsVersion(),
      arch: this.detectArch(),
      deviceModel: this.detectDeviceModel(),
      deviceVendor: this.detectDeviceVendor(),
      cpuCores: this.detectCpuCores(),
      totalRam: this.detectTotalRam(),
      totalStorage: this.detectTotalStorage(),
      hasGpu: this.detectHasGpu(),
      gpuVendor: this.detectGpuVendor(),
      batteryPowered: this.detectBatteryPowered(),
      touchScreen: this.detectTouchScreen(),
    };
  }

  // =========================================================================
  // Sensors
  // =========================================================================

  /** Get simulated sensor data */
  getSensorData(sensorType: string): { data: ArrayBuffer | null; error: SsiErrorCode } {
    const now = Date.now();
    const lastRead = this.lastSensorReadings.get(sensorType) || 0;

    // Rate-limit sensor reads to 50ms
    if (now - lastRead < 50) {
      const cached = this.sensorDataCache.get(sensorType);
      if (cached) return { data: cached.slice(0), error: SsiErrorCode.OK };
    }

    let data: ArrayBuffer;

    switch (sensorType) {
      case 'accelerometer': {
        const buf = new Float32Array(3);
        buf[0] = (Math.random() - 0.5) * 20; // X: -10 to 10 m/s²
        buf[1] = (Math.random() - 0.5) * 20; // Y
        buf[2] = (Math.random() - 0.5) * 20 + 9.8; // Z (gravity included)
        data = buf.buffer;
        break;
      }
      case 'gyroscope': {
        const buf = new Float32Array(3);
        buf[0] = (Math.random() - 0.5) * 200; // deg/s
        buf[1] = (Math.random() - 0.5) * 200;
        buf[2] = (Math.random() - 0.5) * 200;
        data = buf.buffer;
        break;
      }
      case 'magnetometer': {
        const buf = new Float32Array(3);
        buf[0] = (Math.random() - 0.5) * 100;
        buf[1] = (Math.random() - 0.5) * 100;
        buf[2] = (Math.random() - 0.5) * 100;
        data = buf.buffer;
        break;
      }
      case 'ambient-light': {
        const buf = new Float32Array(1);
        buf[0] = Math.random() * 1000; // lux
        data = buf.buffer;
        break;
      }
      case 'temperature': {
        const buf = new Float32Array(1);
        buf[0] = 20 + (Math.random() - 0.5) * 10; // 15-25°C
        data = buf.buffer;
        break;
      }
      case 'humidity': {
        const buf = new Float32Array(1);
        buf[0] = 40 + Math.random() * 40; // 40-80%
        data = buf.buffer;
        break;
      }
      case 'pressure': {
        const buf = new Float32Array(1);
        buf[0] = 1013 + (Math.random() - 0.5) * 20; // hPa
        data = buf.buffer;
        break;
      }
      case 'proximity': {
        const buf = new Float32Array(1);
        buf[0] = Math.random() > 0.5 ? 0 : Math.random() * 10; // cm (0 = near)
        data = buf.buffer;
        break;
      }
      case 'step-counter': {
        const buf = new Uint32Array(1);
        buf[0] = Math.floor(Math.random() * 10000);
        data = buf.buffer;
        break;
      }
      default:
        return { data: null, error: SsiErrorCode.NOT_SUPPORTED };
    }

    this.sensorDataCache.set(sensorType, data.slice(0));
    this.lastSensorReadings.set(sensorType, now);
    return { data: data.slice(0), error: SsiErrorCode.OK };
  }

  /** List available sensor types */
  listSensors(): string[] {
    return [
      'accelerometer', 'gyroscope', 'magnetometer', 'ambient-light',
      'temperature', 'humidity', 'pressure', 'proximity', 'step-counter',
    ];
  }

  // =========================================================================
  // Hardware Control
  // =========================================================================

  /** Set hardware state (vibration, LED, etc.) */
  setHardwareState(control: string, value: ArrayBuffer): SsiErrorCode {
    switch (control) {
      case 'vibrate': {
        const duration = new Uint32Array(value)[0] || 200;
        try {
          if (typeof navigator !== 'undefined' && navigator.vibrate) {
            navigator.vibrate(duration);
          }
        } catch {}
        this.log(`Hardware: vibrate(${duration}ms)`);
        return SsiErrorCode.OK;
      }

      case 'screen-brightness': {
        const brightness = new Float32Array(value)[0] || 1.0;
        this.log(`Hardware: screen brightness = ${(brightness * 100).toFixed(0)}%`);
        return SsiErrorCode.OK;
      }

      case 'lock-screen-orientation': {
        const orientation = new TextDecoder().decode(value);
        try {
          if (typeof screen !== 'undefined' && (screen as any).orientation?.lock) {
            (screen as any).orientation.lock(orientation);
          }
        } catch {}
        this.log(`Hardware: lock orientation = ${orientation}`);
        return SsiErrorCode.OK;
      }

      case 'keep-awake': {
        const awake = new Uint8Array(value)[0] !== 0;
        try {
          if (awake && typeof navigator !== 'undefined' && (navigator as any).wakeLock) {
            (navigator as any).wakeLock.request('screen');
          }
        } catch {}
        this.log(`Hardware: keep-awake = ${awake}`);
        return SsiErrorCode.OK;
      }

      default:
        return SsiErrorCode.NOT_SUPPORTED;
    }
  }

  // =========================================================================
  // Battery
  // =========================================================================

  /** Get battery information */
  async getBatteryInfo(): Promise<SsiBatteryInfo> {
    try {
      if (typeof navigator !== 'undefined' && 'getBattery' in navigator) {
        const battery = await (navigator as any).getBattery();
        return {
          level: Math.round(battery.level * 100),
          status: battery.charging ? 1 : battery.level >= 1 ? 3 : 2,
          temperatureCelsius: 25 + Math.round((Math.random() - 0.5) * 10),
        };
      }
    } catch {}

    // Fallback: simulated battery
    return {
      level: 85,
      status: 2, // discharging
      temperatureCelsius: 28,
    };
  }

  // =========================================================================
  // Network
  // =========================================================================

  /** Get network status */
  getNetworkStatus(): SsiNetworkStatus {
    let online = true;
    let connectionType = 'unknown';
    let signalStrength = 100;

    try {
      if (typeof navigator !== 'undefined') {
        online = navigator.onLine === true;
        const conn = (navigator as any).connection;
        if (conn) {
          connectionType = conn.type || 'unknown';  // "wifi", "cellular", "ethernet"
          signalStrength = conn.downlink ? Math.min(100, conn.downlink * 20) : 100;
        }
      }
    } catch {}

    return {
      online,
      connectionType,
      signalStrength,
      ipAddress: this.detectIpAddress(),
    };
  }

  // =========================================================================
  // Display
  // =========================================================================

  /** Get display information */
  getDisplayInfo(): SsiDisplayInfo {
    let width = 1920;
    let height = 1080;
    let dpi = 96;
    let colorDepth = 24;
    let refreshRate = 60;

    try {
      if (typeof screen !== 'undefined') {
        width = screen.width || width;
        height = screen.height || height;
        dpi = (screen as any).pixelDepth ? (screen as any).pixelDepth * 12 : dpi;
        colorDepth = screen.colorDepth || colorDepth;
      }
    } catch {}

    // Estimate refresh rate from frame timing if available
    try {
      if (typeof window !== 'undefined') {
        let last = performance.now();
        let frames = 0;
        const check = (now: number) => {
          if (now - last < 2000) { frames++; requestAnimationFrame(check); }
          else { refreshRate = Math.round(frames * 1000 / (now - last)); }
        };
        requestAnimationFrame(check);
      }
    } catch {}

    return { width, height, dpi, refreshRate, colorDepth };
  }

  // =========================================================================
  // Lifecycle
  // =========================================================================

  protected async onInit(_config: SsiComponentConfig): Promise<void> {
    this.log('HAL initializing');
  }

  protected async onStart(): Promise<void> {
    this.log('HAL started');
    // Log device info on start
    const info = this.getDeviceInfo();
    this.log(`Platform: ${info.platform}, OS: ${info.os} ${info.osVersion}, Arch: ${info.arch}`);
    this.log(`CPU: ${info.cpuCores} cores, RAM: ${(info.totalRam / 1024 / 1024 / 1024).toFixed(1)} GB`);
  }

  protected async onStop(): Promise<void> {
    this.sensorDataCache.clear();
    this.lastSensorReadings.clear();
    this.log('HAL stopped');
  }

  protected onDestroy(): void {
    this.sensorDataCache.clear();
    this.lastSensorReadings.clear();
  }

  // =========================================================================
  // Platform Detection Internals
  // =========================================================================

  private detectPlatform(): string {
    if (typeof window !== 'undefined' && typeof document !== 'undefined') return 'browser';
    if (typeof process !== 'undefined') return 'native';
    if (typeof (globalThis as any).WorkerGlobalScope !== 'undefined') return 'embedded';
    return 'unknown';
  }

  private detectOs(): string {
    if (typeof navigator === 'undefined') {
      try {
        const p = process.platform;
        if (p === 'win32') return 'Windows';
        if (p === 'darwin') return 'macOS';
        if (p === 'linux') return 'Linux';
        if (p === 'android') return 'Android';
        return p;
      } catch { return 'Unknown'; }
    }
    const ua = navigator.userAgent || '';
    if (ua.includes('Windows')) return 'Windows';
    if (ua.includes('Mac OS')) return 'macOS';
    if (ua.includes('Linux') || ua.includes('Android')) return ua.includes('Android') ? 'Android' : 'Linux';
    if (ua.includes('iPhone') || ua.includes('iPad')) return 'iOS';
    return 'Unknown';
  }

  private detectOsVersion(): string {
    try {
      if (typeof process !== 'undefined') return process.version || '';
    } catch {}
    const ua = navigator.userAgent || '';
    const m = ua.match(/(?:Windows NT |Mac OS X |Android )([\d_.]+)/);
    return m ? m[1].replace(/_/g, '.') : '';
  }

  private detectArch(): string {
    try {
      if (typeof process !== 'undefined') return process.arch;
    } catch {}
    const ua = navigator.userAgent || '';
    if (ua.includes('x64') || ua.includes('WOW64')) return 'x64';
    if (ua.includes('arm64') || ua.includes('aarch64')) return 'arm64';
    if (ua.includes('arm')) return 'arm';
    return 'unknown';
  }

  private detectDeviceModel(): string {
    try {
      if (typeof navigator !== 'undefined') {
        return (navigator as any).deviceMemory ? `Device (${(navigator as any).deviceMemory}GB)` : 'Unknown Device';
      }
    } catch {}
    return 'Unknown Device';
  }

  private detectDeviceVendor(): string {
    const ua = navigator.userAgent || '';
    if (ua.includes('Apple')) return 'Apple';
    if (ua.includes('Samsung')) return 'Samsung';
    if (ua.includes('Google')) return 'Google';
    if (ua.includes('Microsoft')) return 'Microsoft';
    if (ua.includes('Huawei')) return 'Huawei';
    if (ua.includes('Xiaomi')) return 'Xiaomi';
    return 'Unknown';
  }

  private detectCpuCores(): number {
    try {
      if (typeof navigator !== 'undefined') return navigator.hardwareConcurrency || 4;
      if (typeof process !== 'undefined') return require('os').cpus().length;
    } catch {}
    return 4;
  }

  private detectTotalRam(): number {
    try {
      if (typeof navigator !== 'undefined' && 'deviceMemory' in navigator) {
        return (navigator as any).deviceMemory * 1024 * 1024 * 1024;
      }
    } catch {}
    // In Electron/Node, we can get more precise info
    try {
      if (typeof process !== 'undefined') return require('os').totalmem();
    } catch {}
    return 8 * 1024 * 1024 * 1024; // 8GB default
  }

  private detectTotalStorage(): number {
    // Cannot reliably detect total storage in browser
    // Return a reasonable default
    return 256 * 1024 * 1024 * 1024; // 256GB default
  }

  private detectHasGpu(): boolean {
    try {
      if (typeof document !== 'undefined') {
        const canvas = document.createElement('canvas');
        return !!(canvas.getContext('webgl2') || canvas.getContext('webgl'));
      }
    } catch {}
    return false;
  }

  private detectGpuVendor(): string {
    try {
      if (typeof document !== 'undefined') {
        const canvas = document.createElement('canvas');
        const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
        if (gl) {
          const ext = gl.getExtension('WEBGL_debug_renderer_info');
          if (ext) return gl.getParameter(ext.UNMASKED_RENDERER_WEBGL) || 'Unknown GPU';
        }
      }
    } catch {}
    return 'Unknown';
  }

  private detectBatteryPowered(): boolean {
    try {
      const ua = navigator.userAgent || '';
      return ua.includes('iPhone') || ua.includes('iPad') || ua.includes('Android') || ua.includes('Laptop');
    } catch {}
    return false;
  }

  private detectTouchScreen(): boolean {
    try {
      if (typeof navigator !== 'undefined') return 'maxTouchPoints' in navigator && navigator.maxTouchPoints > 0;
    } catch {}
    return false;
  }

  private detectIpAddress(): string {
    // Simple local IP detection is not possible in browser without external service
    try {
      if (typeof process !== 'undefined') {
        const os = require('os');
        const ifaces = os.networkInterfaces();
        for (const name of Object.keys(ifaces)) {
          for (const iface of ifaces[name] || []) {
            if (iface.family === 'IPv4' && !iface.internal) return iface.address;
          }
        }
      }
    } catch {}
    return '127.0.0.1';
  }
}
