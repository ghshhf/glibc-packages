const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('skynet', {
  // System info
  getSystemInfo: () => ipcRenderer.invoke('get-system-info'),

  // SSI Runtime
  runtimeInit: () => ipcRenderer.invoke('runtime-init'),
  runtimeStatus: () => ipcRenderer.invoke('runtime-status'),
  runtimeComponents: () => ipcRenderer.invoke('runtime-components'),
  runtimeStop: () => ipcRenderer.invoke('runtime-stop'),
  runtimeLoadComponent: () => ipcRenderer.invoke('runtime-load-component'),
  runtimeLoadSwbn: (swbnPath) => ipcRenderer.invoke('runtime-load-swbn', swbnPath),

  // Runtime events from main process
  onRuntimeInitRequest: (callback) => {
    ipcRenderer.on('runtime-init-request', () => callback());
  },
  onRuntimeStopRequest: (callback) => {
    ipcRenderer.on('runtime-stop-request', () => callback());
  },

  // Logging
  log: (level, message, data) => ipcRenderer.invoke('log', level, message, data),
  exportLogs: () => ipcRenderer.invoke('export-logs'),
  getLogFiles: () => ipcRenderer.invoke('get-log-files'),
  readLogFile: (filePath) => ipcRenderer.invoke('read-log-file', filePath),

  // Events
  onAppReady: (callback) => {
    ipcRenderer.on('app-ready', (_event, data) => callback(data));
  },
});
