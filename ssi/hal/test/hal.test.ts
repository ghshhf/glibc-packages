import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { SsiErrorCode } from '../../core/src/index';
import { HardwareAbstractionLayer } from '../src/index';

describe('HardwareAbstractionLayer', () => {
  let hal: HardwareAbstractionLayer;

  beforeEach(async () => {
    hal = new HardwareAbstractionLayer();
    await hal.init();
    await hal.start();
  });

  afterEach(async () => {
    await hal.stop();
  });

  describe('device info', () => {
    it('should return device information', () => {
      const info = hal.getDeviceInfo();
      expect(info.platform).toBeTruthy();
      expect(info.os).toBeTruthy();
      expect(info.arch).toBeTruthy();
      expect(info.cpuCores).toBeGreaterThan(0);
      expect(info.totalRam).toBeGreaterThan(0);
      expect(info.totalStorage).toBeGreaterThan(0);
    });
  });

  describe('sensors', () => {
    it('should return accelerometer data', () => {
      const { data, error } = hal.getSensorData('accelerometer');
      expect(error).toBe(SsiErrorCode.OK);
      expect(data).not.toBeNull();
      expect(data!.byteLength).toBe(12); // 3 x Float32
    });

    it('should return gyroscope data', () => {
      const { data, error } = hal.getSensorData('gyroscope');
      expect(error).toBe(SsiErrorCode.OK);
      expect(data!.byteLength).toBe(12);
    });

    it('should return ambient light data', () => {
      const { data, error } = hal.getSensorData('ambient-light');
      expect(error).toBe(SsiErrorCode.OK);
      expect(data!.byteLength).toBe(4); // 1 x Float32
    });

    it('should return temperature data', () => {
      const { data } = hal.getSensorData('temperature');
      const val = new Float32Array(data!)[0];
      expect(val).toBeGreaterThanOrEqual(15);
      expect(val).toBeLessThanOrEqual(25);
    });

    it('should return NOT_SUPPORTED for unknown sensor', () => {
      const { data, error } = hal.getSensorData('brain-wave');
      expect(data).toBeNull();
      expect(error).toBe(SsiErrorCode.NOT_SUPPORTED);
    });

    it('should list available sensors', () => {
      const sensors = hal.listSensors();
      expect(sensors).toContain('accelerometer');
      expect(sensors).toContain('gyroscope');
      expect(sensors).toContain('temperature');
      expect(sensors.length).toBe(9);
    });

    it('should cache sensor data within rate limit', () => {
      const { data: d1 } = hal.getSensorData('accelerometer');
      const { data: d2 } = hal.getSensorData('accelerometer');
      // Both returns should have valid data
      expect(d1!.byteLength).toBe(12);
      expect(d2!.byteLength).toBe(12);
    });
  });

  describe('hardware control', () => {
    it('should handle vibrate', () => {
      const duration = new Uint32Array([300]);
      expect(hal.setHardwareState('vibrate', duration.buffer)).toBe(SsiErrorCode.OK);
    });

    it('should handle screen brightness', () => {
      const brightness = new Float32Array([0.5]);
      expect(hal.setHardwareState('screen-brightness', brightness.buffer)).toBe(SsiErrorCode.OK);
    });

    it('should return NOT_SUPPORTED for unknown control', () => {
      expect(hal.setHardwareState('teleport', new ArrayBuffer(0))).toBe(SsiErrorCode.NOT_SUPPORTED);
    });
  });

  describe('battery', () => {
    it('should return battery info', async () => {
      const info = await hal.getBatteryInfo();
      expect(info.level).toBeGreaterThanOrEqual(0);
      expect(info.level).toBeLessThanOrEqual(100);
      expect(info.status).toBeGreaterThanOrEqual(0);
      expect(info.status).toBeLessThanOrEqual(3);
    });
  });

  describe('network', () => {
    it('should return network status', () => {
      const status = hal.getNetworkStatus();
      expect(typeof status.online).toBe('boolean');
      expect(status.connectionType).toBeTruthy();
      expect(status.signalStrength).toBeGreaterThanOrEqual(0);
    });
  });

  describe('display', () => {
    it('should return display info', () => {
      const info = hal.getDisplayInfo();
      expect(info.width).toBeGreaterThan(0);
      expect(info.height).toBeGreaterThan(0);
      expect(info.dpi).toBeGreaterThan(0);
      expect(info.colorDepth).toBeGreaterThan(0);
    });
  });
});
