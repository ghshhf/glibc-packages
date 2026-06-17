const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('skynet', {
  // System info
  getSystemInfo: () => ipcRenderer.invoke('get-system-info'),

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
