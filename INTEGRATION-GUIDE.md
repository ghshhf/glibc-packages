# SkyNet SSI-FS 后端集成指南

## 已完成的新后端

在 `ssi/fs/src/` 下新增了 **2 个后端 + 更新了 index.ts**：

| 文件 | 说明 |
|------|------|
| `src/nodefs.ts` | NODEFS 后端 — 桥接 Node.js `fs/promises` 模块，支持真实文件系统 |
| `src/opfs.ts` | OPFS 后端 — 浏览器 Origin Private File System，支持持久化存储 |
| `src/index.ts` | 已更新，导出 `NodeFsBackend` 和 `OpfsBackend` |

### 集成步骤

1. 将本目录下的 `src/nodefs.ts`、`src/opfs.ts` 复制到你的 `ssi/fs/src/` 目录
2. 更新 `ssi/fs/src/index.ts`（已附最新版本）
3. 安装依赖（如需）：
   ```bash
   pnpm add -D @types/node
   ```
4. 运行测试验证：
   ```bash
   pnpm vitest run ssi/fs/test/
   ```

### 使用方式

```typescript
import { FileSystem, NodeFsBackend, OpfsBackend } from '../ssi/fs/src';

const fs = new FileSystem();

// 挂载主机文件系统到 /host
fs.mount('/host', new NodeFsBackend('/home/user'));

// 挂载浏览器持久化存储
const opfs = new OpfsBackend();
await opfs.init();
fs.mount('/persist', opfs);

// 读取文件
const data = await fs.readFile('/host/projects/README.md');
console.log(data);

// 卸载
fs.unmount('/host');
```

---

# SkyNet 反馈系统集成指南

## 已完成的功能

在 `desktop/` 下新增了反馈系统：

| 文件 | 说明 |
|------|------|
| `src/feedback.js` | Electron 主进程 IPC 处理器（收集系统信息、日志、调用 GitHub API） |
| `public/feedback-panel.html` | Dashboard 侧反馈面板 UI（Token 配置、问题描述、预览、提交） |

### 集成步骤

#### 1. main.js — 注册 IPC 处理器

在 `desktop/src/main.js` 中：

```javascript
const { registerFeedbackHandlers } = require('./feedback');

// 在 app.whenReady() 或创建窗口后注册
app.whenReady().then(() => {
  const logDir = path.join(app.getPath('userData'), 'logs');
  registerFeedbackHandlers(app, logDir);
  // ... 后续窗口创建逻辑
});
```

#### 2. index.html — 嵌入反馈面板

在 `desktop/public/index.html` 的 Tab 容器中添加：

```html
<!-- 在 Tab 导航栏添加新 Tab -->
<div class="nav-item" data-tab="feedback">
  <span class="nav-icon">📬</span>
  <span>反馈</span>
</div>

<!-- 在 Tab 内容区引入面板 -->
<!--#include virtual="feedback-panel.html" -->
<!-- 或直接复制 feedback-panel.html 的 <div id="tab-feedback"> 内容 -->
```

#### 3. preload.js — 暴露 IPC 通道（如需要）

确保 preload.js 中暴露了 `invoke` 方法：
```javascript
contextBridge.exposeInMainWorld('skynet', {
  invoke: (channel, ...args) => ipcRenderer.invoke(channel, ...args),
});
```

---

# 标准化 zlib 包

## 已完成

`gpkg/zlib/build.sh` 已按 CONTRIBUTING.md 标准规范化，可作为其他包的模板。

### 包含的字段

```
TERMUX_PKG_HOMEPAGE
TERMUX_PKG_DESCRIPTION
TERMUX_PKG_LICENSE
TERMUX_PKG_MAINTAINER    → @ghshhf
TERMUX_PKG_VERSION
TERMUX_PKG_SRCURL
TERMUX_PKG_SHA256
TERMUX_PKG_AUTO_UPDATE   → true
termux_step_pre_configure()  → 平台兼容处理
```

### 模板使用

将 `gpkg/zlib/build.sh` 作为其他包的标准模板，直接替换变量名、版本号和 SHA256 即可。