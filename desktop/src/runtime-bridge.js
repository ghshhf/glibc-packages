/**
 * SkyNet SSI Runtime Bridge
 *
 * Loads the SSI-KRN runtime kernel (browser-runtime) into the
 * Electron main process and exposes it via IPC to the renderer.
 *
 * This file is CJS-compatible but uses dynamic import() to load
 * the ESM-compiled browser-runtime module.
 */

const path = require('path');
const fs = require('fs');
const { app, dialog } = require('electron');

let runtimeInstance = null;
let isInitialized = false;
let runtimeModule = null;
let loadedModules = [];

// ── Path resolution ──

function getRuntimeDistPath() {
  return path.resolve(app.getAppPath(), '..', 'browser-runtime', 'dist', 'index.js');
}

// ── Runtime Operations ──

async function initRuntime(config = {}) {
  if (isInitialized) return { ok: true, message: 'Already initialized' };

  try {
    const modPath = getRuntimeDistPath();
    runtimeModule = await import(modPath);
    const SsiRuntime = runtimeModule.SsiRuntime || runtimeModule.default;

    runtimeInstance = SsiRuntime.init({
      logLevel: config.logLevel || 'info',
      ...config,
    });

    isInitialized = true;
    return { ok: true, instance: true };
  } catch (err) {
    isInitialized = false;
    return { ok: false, error: err.message };
  }
}

function getRuntimeStatus() {
  if (!runtimeInstance) {
    return { running: false, components: [], wasmModuleCount: 0, memory: 0, uptime: 0 };
  }

  return {
    running: true,
    environment: runtimeInstance.environment,
    uptime: process.uptime(),
    memory: process.memoryUsage().rss,
    config: runtimeInstance.config,
    loadedModules: loadedModules.length,
  };
}

function getComponents() {
  if (!runtimeInstance) return [];

  const core = [
    { name: 'wasm-loader', type: 'ssi-krn', state: 'RUNNING', version: '1.0.0' },
    { name: 'virtual-fs', type: 'ssi-fs', state: 'RUNNING', version: '1.0.0' },
    { name: 'networking', type: 'ssi-net', state: 'RUNNING', version: '1.0.0' },
    { name: 'threading', type: 'ssi-krn', state: 'RUNNING', version: '1.0.0' },
  ];

  // Add any loaded .swbn components
  const userComponents = loadedModules.map((mod, i) => ({
    name: mod.name || `component-${i}`,
    type: mod.type || 'swbn',
    state: 'LOADED',
    version: mod.version || '1.0.0',
    path: mod.path,
  }));

  return [...core, ...userComponents];
}

async function stopRuntime() {
  if (!runtimeInstance || !isInitialized) return { ok: true, message: 'Not running' };

  try {
    runtimeInstance.wasm.clearCache();
    loadedModules = [];
    isInitialized = false;
    runtimeInstance = null;
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
}

/**
 * Load a .swbn component file
 */
async function loadSwbnFile(swbnPath) {
  if (!fs.existsSync(swbnPath)) {
    return { ok: false, error: 'File not found' };
  }

  try {
    const data = fs.readFileSync(swbnPath);
    const zlib = require('zlib');
    const decompressed = zlib.gunzipSync(data);

    // Extract manifest
    let offset = 0;
    let manifest = null;
    while (offset + 264 <= decompressed.length) {
      const name = decompressed.subarray(offset, offset + 256).toString('utf-8').replace(/\0/g, '').trim();
      const size = Number(decompressed.readBigUint64BE(offset + 256));
      offset += 264;

      if (name === 'manifest.json') {
        manifest = JSON.parse(decompressed.subarray(offset, offset + size).toString('utf-8'));
        break;
      }
      offset += size;
    }

    if (!manifest) {
      return { ok: false, error: 'Invalid .swbn: no manifest found' };
    }

    loadedModules.push({
      name: manifest.component?.name || path.basename(swbnPath, '.swbn'),
      type: manifest.component?.type || 'generic',
      version: manifest.component?.version || '1.0.0',
      path: swbnPath,
      manifest,
    });

    return {
      ok: true,
      component: {
        name: manifest.component?.name,
        type: manifest.component?.type,
        version: manifest.component?.version,
        interfaces: manifest.interfaces,
      }
    };
  } catch (err) {
    return { ok: false, error: err.message };
  }
}

/**
 * Open a file dialog to pick .swbn components
 */
async function openSwbnDialog() {
  const { canceled, filePaths } = await dialog.showOpenDialog({
    title: 'Load SWBN Component',
    filters: [{ name: 'SWBN Component', extensions: ['swbn'] }],
    properties: ['openFile'],
  });

  if (canceled || filePaths.length === 0) {
    return { ok: false, canceled: true };
  }

  return loadSwbnFile(filePaths[0]);
}

module.exports = {
  initRuntime,
  getRuntimeStatus,
  getComponents,
  stopRuntime,
  loadSwbnFile,
  openSwbnDialog,
};
