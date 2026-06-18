/**
 * SkyNet 反馈系统 — GitHub Issue 自动创建
 *
 * 按 FEEDBACK-PLAN.md 设计实现。
 * 集成方式: 在 desktop/src/main.js 中 require('./feedback') 即可。
 *
 * 功能:
 *   1. 收集系统信息 (OS / SkyNet 版本 / Electron/Node 版本)
 *   2. 收集最近 200 行日志
 *   3. 用户填写问题描述
 *   4. 创建 GitHub Issue (公开)
 *
 * 安全:
 *   - GitHub Token 由用户在设置页输入（仅存本地，不影响系统）
 *   - 创建 Issue 前可预览全部内容
 *   - 日志中的本地路径和用户名会被用户预览确认
 */

const { ipcMain, dialog, shell } = require('electron');
const path = require('path');
const fs = require('fs');
const os = require('os');
const https = require('https');
const { URL } = require('url');

// =========================================================================
// 配置
// =========================================================================

const GITHUB_REPO = 'ghshhf/glibc-packages';
const GITHUB_API = 'https://api.github.com';

const APP_NAME = 'SkyNet SSI Runtime';
const APP_VERSION = '1.0.0';

// =========================================================================
// Token 存储（开发期写入本地文件；生产期可用 electron-store / 密钥链）
// =========================================================================

function getTokenPath(app) {
  const userDataPath = app ? app.getPath('userData') : process.cwd();
  return path.join(userDataPath, '.github-token');
}

function loadToken(app) {
  try {
    return fs.readFileSync(getTokenPath(app), 'utf-8').trim();
  } catch {
    return '';
  }
}

function saveToken(app, token) {
  try {
    const tokenPath = getTokenPath(app);
    fs.writeFileSync(tokenPath, token.trim(), 'utf-8');
    fs.chmodSync(tokenPath, 0o600); // 仅当前用户可读写
    return true;
  } catch (e) {
    console.error('[Feedback] Failed to save token:', e.message);
    return false;
  }
}

// =========================================================================
// 系统信息收集
// =========================================================================

function collectSystemInfo() {
  const mem = process.memoryUsage();
  const cpuCores = os.cpus().length;
  const cpus = os.cpus();
  const cpuModel = cpus.length > 0 ? cpus[0].model : 'unknown';

  return {
    OS: `${os.type()} ${os.release()} (${os.platform()})`,
    'SkyNet 版本': APP_VERSION,
    Electron: process.versions.electron || 'N/A',
    Node: process.versions.node,
    Chrome: process.versions.chrome,
    V8: process.versions.v8,
    CPU: `${cpuModel} (${cpuCores} cores)`,
    '总内存': `${(os.totalmem() / 1024 / 1024 / 1024).toFixed(1)} GB`,
    '可用内存': `${(os.freemem() / 1024 / 1024 / 1024).toFixed(1)} GB`,
    '进程内存 (RSS)': `${(mem.rss / 1024 / 1024).toFixed(1)} MB`,
    '主机名': os.hostname(),
    '用户目录': os.homedir(),
    '运行时间': `${Math.floor(process.uptime() / 60)} 分钟`,
  };
}

// =========================================================================
// 日志收集
// =========================================================================

function collectRecentLogs(logDir, maxLines = 200) {
  try {
    if (!logDir || !fs.existsSync(logDir)) return '（无日志目录）';

    const files = fs.readdirSync(logDir)
      .filter(f => f.endsWith('.log'))
      .sort()
      .reverse();

    if (files.length === 0) return '（无日志文件）';

    const allLines = [];
    let remaining = maxLines;

    for (const file of files) {
      if (remaining <= 0) break;
      const filePath = path.join(logDir, file);
      try {
        const content = fs.readFileSync(filePath, 'utf-8');
        const lines = content.split('\n').filter(Boolean);

        if (lines.length > remaining) {
          allLines.push(`# ... (省略 ${lines.length - remaining} 行)`);
          allLines.push(...lines.slice(lines.length - remaining));
          remaining = 0;
        } else {
          allLines.push(`# === ${file} ===`);
          allLines.push(...lines);
          remaining -= lines.length;
        }
      } catch {
        // 跳过无法读取的文件
      }
    }

    return allLines.slice(-maxLines).join('\n');
  } catch {
    return '（日志收集失败）';
  }
}

// =========================================================================
// GitHub API 调用
// =========================================================================

function createGitHubIssue(token, title, body) {
  return new Promise((resolve, reject) => {
    const postData = JSON.stringify({
      title,
      body,
      labels: ['feedback', 'automated'],
    });

    const url = new URL(`${GITHUB_API}/repos/${GITHUB_REPO}/issues`);

    const options = {
      hostname: url.hostname,
      path: url.pathname,
      method: 'POST',
      headers: {
        'Authorization': `token ${token}`,
        'Content-Type': 'application/json',
        'User-Agent': 'SkyNet-Feedback/1.0',
        'Content-Length': Buffer.byteLength(postData),
        'Accept': 'application/vnd.github.v3+json',
      },
    };

    const req = https.request(options, (res) => {
      let data = '';
      res.on('data', (chunk) => { data += chunk; });

      res.on('end', () => {
        try {
          const parsed = JSON.parse(data);
          if (res.statusCode === 201 || res.statusCode === 200) {
            resolve({
              success: true,
              issueUrl: parsed.html_url,
              issueNumber: parsed.number,
            });
          } else {
            resolve({
              success: false,
              statusCode: res.statusCode,
              error: parsed.message || data,
            });
          }
        } catch {
          resolve({
            success: false,
            statusCode: res.statusCode,
            error: data,
          });
        }
      });
    });

    req.on('error', (err) => {
      resolve({ success: false, error: err.message });
    });

    req.write(postData);
    req.end();
  });
}

// =========================================================================
// Issue 模板
// =========================================================================

function buildIssueBody(systemInfo, logs, userDescription) {
  const lines = ['## 环境信息', ''];
  for (const [key, value] of Object.entries(systemInfo)) {
    lines.push(`- **${key}**: ${value}`);
  }

  lines.push('', '---', '', '## 日志', '');
  lines.push('```log');
  lines.push(logs);
  lines.push('```');

  lines.push('', '---', '', '## 用户描述', '');
  lines.push(userDescription || '（用户未填写）');

  return lines.join('\n');
}

// =========================================================================
// 注册 IPC 处理器
// =========================================================================

function registerFeedbackHandlers(app, logDir) {
  // 获取 Token
  ipcMain.handle('feedback-get-token', () => {
    return loadToken(app);
  });

  // 保存 Token
  ipcMain.handle('feedback-save-token', (_event, token) => {
    return saveToken(app, token);
  });

  // 收集 Issue 预览数据
  ipcMain.handle('feedback-preview', () => {
    const systemInfo = collectSystemInfo();
    const logs = collectRecentLogs(logDir);
    return { systemInfo, logs };
  });

  // 提交 Issue
  ipcMain.handle('feedback-submit', async (_event, { title, description, systemInfo, logs }) => {
    const token = loadToken(app);
    if (!token) {
      return { success: false, error: '未设置 GitHub Token。请先在设置页配置。' };
    }

    const body = buildIssueBody(systemInfo, logs, description);
    const result = await createGitHubIssue(token, title, body);

    if (result.success) {
      // 打开浏览器跳转到 Issue
      shell.openExternal(result.issueUrl).catch(() => {});
    }

    return result;
  });
}

// =========================================================================
// 导出
// =========================================================================

module.exports = {
  registerFeedbackHandlers,
  collectSystemInfo,
  collectRecentLogs,
  createGitHubIssue,
  buildIssueBody,
};