const { app, BrowserWindow, ipcMain, dialog, Menu } = require('electron');
const path = require('path');
const fs = require('fs');

let mainWindow;

// ── App Config ──
const APP_NAME = 'SkyNet SSI Runtime';
const APP_VERSION = '1.0.0';
const LOG_DIR = path.join(app.getPath('userData'), 'logs');

// ── Ensure log directory ──
if (!fs.existsSync(LOG_DIR)) {
  fs.mkdirSync(LOG_DIR, { recursive: true });
}

// ── Create main window ──
function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 860,
    minWidth: 900,
    minHeight: 600,
    title: APP_NAME,
    icon: path.join(__dirname, '..', 'public', 'icon.png'),
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
    },
    backgroundColor: '#0d1117',
    show: false,
  });

  mainWindow.loadFile(path.join(__dirname, '..', 'public', 'index.html'));

  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
    mainWindow.webContents.send('app-ready', {
      version: APP_VERSION,
      platform: process.platform,
      arch: process.arch,
      electronVersion: process.versions.electron,
      nodeVersion: process.versions.node,
      chromeVersion: process.versions.chrome,
    });
  });

  // Build menu
  const menuTemplate = [
    {
      label: 'SkyNet',
      submenu: [
        { label: `About ${APP_NAME} v${APP_VERSION}`, enabled: false },
        { type: 'separator' },
        { role: 'quit' },
      ],
    },
    {
      label: 'View',
      submenu: [
        { role: 'reload' },
        { role: 'toggleDevTools' },
        { type: 'separator' },
        { role: 'resetZoom' },
        { role: 'zoomIn' },
        { role: 'zoomOut' },
      ],
    },
    {
      label: 'Logs',
      submenu: [
        {
          label: 'Export Logs…',
          accelerator: 'CmdOrCtrl+E',
          click: () => exportLogs(),
        },
        { type: 'separator' },
        {
          label: 'Open Log Directory',
          click: () => {
            shell.openPath(LOG_DIR).catch(() => {});
          },
        },
      ],
    },
  ];

  const menu = Menu.buildFromTemplate(menuTemplate);
  Menu.setApplicationMenu(menu);
}

// ── IPC Handlers ──

// System info
ipcMain.handle('get-system-info', () => {
  return {
    appName: APP_NAME,
    appVersion: APP_VERSION,
    platform: process.platform,
    arch: process.arch,
    electron: process.versions.electron,
    node: process.versions.node,
    chrome: process.versions.chrome,
    v8: process.versions.v8,
    cpuUsage: process.cpuUsage(),
    memoryUsage: process.memoryUsage(),
    uptime: process.uptime(),
  };
});

// Log message from renderer
ipcMain.handle('log', (_event, level, message, data) => {
  const timestamp = new Date().toISOString();
  const logLine = `[${timestamp}] [${level.toUpperCase()}] ${message}`;
  const logFile = path.join(LOG_DIR, `skynet-${new Date().toISOString().slice(0, 10)}.log`);

  fs.appendFileSync(logFile, logLine + '\n');
  console.log(logLine, data || '');

  return { written: true, file: logFile };
});

// Export logs
ipcMain.handle('export-logs', async () => {
  return exportLogs();
});

// Get log files list
ipcMain.handle('get-log-files', () => {
  try {
    const files = fs.readdirSync(LOG_DIR)
      .filter(f => f.endsWith('.log'))
      .map(f => ({
        name: f,
        path: path.join(LOG_DIR, f),
        size: fs.statSync(path.join(LOG_DIR, f)).size,
        modified: fs.statSync(path.join(LOG_DIR, f)).mtime,
      }))
      .sort((a, b) => b.modified - a.modified);
    return files;
  } catch {
    return [];
  }
});

// Read log file content
ipcMain.handle('read-log-file', (_event, filePath) => {
  try {
    const fullPath = path.join(LOG_DIR, path.basename(filePath));
    if (fs.existsSync(fullPath)) {
      return fs.readFileSync(fullPath, 'utf-8');
    }
    return '';
  } catch {
    return '';
  }
});

// ── Export Logs ──
async function exportLogs() {
  const { canceled, filePath } = await dialog.showSaveDialog(mainWindow, {
    title: 'Export Logs',
    defaultPath: path.join(app.getPath('desktop'), `skynet-logs-${Date.now()}.txt`),
    filters: [{ name: 'Log Files', extensions: ['txt', 'log'] }],
  });

  if (canceled) return { exported: false };

  try {
    const files = fs.readdirSync(LOG_DIR).filter(f => f.endsWith('.log'));
    const allLogs = files
      .map(f => fs.readFileSync(path.join(LOG_DIR, f), 'utf-8'))
      .join('\n');

    fs.writeFileSync(filePath, allLogs, 'utf-8');
    return { exported: true, path: filePath };
  } catch (e) {
    return { exported: false, error: e.message };
  }
}

// ── App Lifecycle ──
app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});
