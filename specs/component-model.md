# SSI 组件模型规范

> **Standard SWBN Component Model** — 系统中所有组件遵循的统一打包与运行规范。

---

## 1. 组件包格式 (.swbn)

### 1.1 包结构

每个组件是一个 `.swbn` 归档文件（tar.gz 格式），包含以下文件：

```
my-component-1.0.0.swbn
├── manifest.json          # 🅼 组件清单（必需）
├── main.wasm              # 🅼 主 WASM 模块（必需）
├── main.js                # 🅾 Emscripten JS 胶水（可选）
├── types/                 # 🅾 类型定义
│   ├── index.d.ts         # TypeScript 类型
│   └── schema.swbn        # swbn schema 定义
├── assets/                # 🅾 资源文件
│   ├── icon.png
│   ├── style.css
│   └── locales/
├── policies/              # 🅾 策略文件
│   └── security.policy
├── tests/                 # 🅾 测试用例
│   └── test.wasm
└── signature.sig          # 🅼 数字签名（必需）
```

### 1.2 manifest.json 规范

```json
{
  "$schema": "https://schemas.ai-tp.org/swbn/manifest-v1.json",

  "component": {
    "id": "9a8b7c6d-5e4f-3a2b-1c0d-9e8f7a6b5c4d",
    "name": "browser-engine",
    "type": "browser",
    "version": "1.0.0",
    "vendor": "AI-TP Foundation",
    "description": "Core browser engine - system native rendering & scripting"
  },

  "entry": {
    "wasm": "main.wasm",
    "js_glue": "main.js",
    "memory": {
      "initial": "64MB",
      "maximum": "512MB",
      "stack": "256KB"
    }
  },

  "interfaces": {
    "provides": ["SSI-CORE", "SSI-BR"],
    "requires": ["SSI-UI", "SSI-NET", "SSI-FS"]
  },

  "permissions": [
    {
      "interface": "SSI-NET",
      "operations": ["fetch", "websocket", "listen"],
      "restriction": "unrestricted"
    },
    {
      "interface": "SSI-FS",
      "operations": ["read", "write"],
      "paths": ["/system/*", "/user-data/browser/*"]
    }
  ],

  "dependencies": {
    "window-manager": ">=1.0.0",
    "network-stack": ">=1.0.0",
    "file-system": ">=1.0.0"
  },

  "capabilities": {
    "gpu": true,
    "threads": true,
    "simd": false,
    "memory64": false
  },

  "lifecycle": {
    "autostart": true,
    "keep_alive": true,
    "restart_on_crash": true,
    "max_restarts": 5
  },

  "metadata": {
    "author": "ghshhf",
    "license": "MIT",
    "homepage": "https://github.com/ghshhf/glibc-packages"
  }
}
```

### 1.3 签名机制

```c
// 签名流程：
// 1. 对归档中 manifest.json + main.wasm 做 SHA256
//    hash = SHA256(manifest.json || main.wasm)
// 2. 用组件私钥签名
//    signature = Ed25519_Sign(hash, private_key)
// 3. 写入 signature.sig

// 验证流程：
// 1. 读取 signature.sig
// 2. 计算 hash = SHA256(manifest.json || main.wasm)
// 3. 用组件的公钥验证
//    valid = Ed25519_Verify(hash, signature, public_key)
```

### 1.4 组件注册表

系统中所有组件在启动时向 SSI 总线注册：

```c
typedef struct {
    uint8_t     uuid[16];
    string      name;
    string      type;
    string      version;
    string[]    provides_interfaces;
    string[]    requires_interfaces;
    SsiEndpoint endpoint;
    uint64_t    registered_at;
} SsiRegistryEntry;
```

---

## 2. 组件生命周期

### 2.1 状态机

```
                  ┌──────────────┐
                  │ UNINITIALIZED │
                  └──────┬───────┘
                         │ load_component()
                         ▼
                  ┌──────────────┐
    ┌─────────────│ INITIALIZING  │
    │             └──────┬───────┘
    │ init() 失败         │ init() 成功
    ▼                    ▼
┌────────┐       ┌──────────────┐
│ ERROR  │       │    READY      │
└────────┘       └──────┬───────┘
                        │ start()
                        ▼
                 ┌──────────────┐
      ┌──────────│   RUNNING     │──────────┐
      │ suspend  └──────┬───────┘ stop()    │
      ▼                 │                   ▼
┌──────────┐    resume  │            ┌──────────┐
│SUSPENDED │◄───────────┘            │TERMINATED │
└──────────┘                         └──────────┘
```

### 2.2 生命周期钩子调用顺序

```
加载阶段:
  load_module() → init() → start()

运行阶段:
  [正常运行] → suspend() → [暂停] → resume() → [继续运行]

终止阶段:
  [运行中] → stop() → destroy() → unload_module()

错误恢复:
  [运行中/暂停] → 检测到错误 → ERROR → [自动恢复] → init() → start()
```

---

## 3. 组件间通信

### 3.1 调用模式

| 模式 | 描述 | 适用场景 |
|------|------|---------|
| **同步调用** | 请求方阻塞等待响应 | 简单查询、配置读写 |
| **异步调用** | 请求方立即返回，通过回调/事件获取结果 | 网络请求、存储操作 |
| **事件推送** | 组件主动推送事件到总线 | UI 事件、系统通知、状态变更 |
| **流式通信** | 持续的双向数据流 | 音视频传输、日志流 |

### 3.2 通信时序

```
同步调用:
  调用方                         被调用方
    │                               │
    │───── 同步请求 msg (unicast) ──→│
    │←──── 同步响应 msg (unicast) ───│
    │                               │

异步调用:
  调用方                         被调用方
    │                               │
    │───── 异步请求 msg (unicast) ──→│
    │←──── 立即返回 ACK ─────────────│
    │          ...                   │
    │←──── 异步回调 msg (unicast) ───│
    │                               │

事件推送:
  发布方                         所有订阅者
    │                               │
    │───── 事件 msg (multicast) ────→│
    │──→│──→│──→│ (按类型分发)       │
    │                               │
```

---

## 4. 组件版本管理

### 4.1 版本号规范

使用语义化版本 `MAJOR.MINOR.PATCH`：

| 变更类型 | MAJOR | MINOR | PATCH |
|---------|-------|-------|-------|
| 接口不兼容修改 | +1 | 0 | 0 |
| 向后兼容的功能添加 | — | +1 | 0 |
| 向后兼容的 bug 修复 | — | — | +1 |

### 4.2 依赖解析

```
依赖规则:
  "browser-engine": ">=1.0.0"     // 大版本兼容
  "storage-engine": "~1.2.0"      // 小版本兼容
  "network-stack": "1.0.0"        // 精确匹配
  "window-manager": "*"           // 任意版本

解析策略:
  1. 收集所有组件及其依赖
  2. 构建依赖图
  3. 检查版本冲突
  4. 选择满足所有约束的最高版本
  5. 验证结果
```

---

## 5. 组件打包工具

### 5.1 命令行工具

```bash
# 初始化组件模板
swbn init --name my-component --type compute

# 构建组件
swbn build ./src --output my-component-1.0.0.swbn

# 签名组件
swbn sign --key ./private.key my-component-1.0.0.swbn

# 验证组件
swbn verify my-component-1.0.0.swbn

# 安装组件
swbn install my-component-1.0.0.swbn

# 查看组件信息
swbn info my-component-1.0.0.swbn

# 运行组件
swbn run my-component-1.0.0.swbn
```

### 5.2 从 glibc-packages 构建 .swbn

```
gpkg/zlib → build-cross.sh --platform browser
    → output/zlib-1.3.2-browser-wasm32.swbn
    → output/zlib-1.3.2.tgz (npm 兼容)

gpkg/browser-engine → build-cross.sh --platform browser
    → output/browser-engine-1.0.0-browser-wasm32.swbn
    → output/browser-engine-1.0.0.tgz (npm 兼容)
```

---

> 版本: 1.0.0 | 最后更新: 2026-06-17
