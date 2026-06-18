# SkyNet (天网) 🌐  
**从 glibc-packages 到 SkyNet — 完全重构的标准化底层系统**

> 这不是一次简单的改名。这是一次从"包构建工具"到"文明级基础设施"的跃迁。

---

## 📥 立即安装

| 平台 | 安装包 | 说明 |
|------|--------|------|
| 🪟 **Windows 10/11** | [⬇ 下载 SkyNet Desktop v1.0.0](https://github.com/ghshhf/glibc-packages/releases/latest/download/SkyNet-Desktop-Setup-1.0.0.exe) | Electron 桌面仪表盘 + SSI 运行时 |
| 📱 **Android 7+** | [⬇ 下载 SkyNet Runtime v1.0.0](https://github.com/ghshhf/glibc-packages/releases/latest/download/SkyNet-Runtime-v1.0.0.apk) | WebView 运行时仪表盘 |
| 🐍 **pip 包** | `pip install skynet-ssi` | Python SDK（开发中）|
| 📦 **npm 包** | `npm install @skynet/browser-runtime` | 浏览器运行时 |

> 安装包通过 [GitHub Releases](https://github.com/ghshhf/glibc-packages/releases) 发布，SHA256 校验值见对应 Release 页面。

---

## ⚡ 升级了什么？（新旧对比）

| 维度 | 旧版 glibc-packages | 新版 SkyNet |
|------|---------------------|-------------|
| **定位** | 跨平台 glibc 包构建工具 | 标准化底层系统架构 |
| **包格式** | `.tar.gz` / `.deb` / `.pkg.tar.zst` | **`.swbn`** — Standard WASM Binary Notation |
| **架构** | 无统一规范，各包自行其是 | **SSI v1.0** — 12 个标准接口，强制规范 |
| **浏览器** | 外部移植目标 (`--platform browser`) | **原生核心组件** — 浏览器即内核 |
| **组件模型** | 无 | **标准化组件** — manifest + wasm + signature |
| **通信** | 无 | **SSI 总线** — 二进制 IPC，优先级队列，跨组件路由 |
| **安全** | 无 | **5 层安全模型** — 沙箱 + ACL + 签名 + 审计 |
| **运行时** | 无 | **SSI-KRN** — TypeScript WASM 运行时内核 |
| **安装包** | 无 | **Windows .exe + Android .apk** — 一键安装仪表盘 |

### 为什么用 SkyNet 重建？

```
旧模式：每个平台写一套代码
  Android → Java/Kotlin
  iOS     → Swift
  Windows → C++/.NET
  Web     → JavaScript
  ↓ 重复造轮子，维护地狱

新模式：SkyNet 标准化架构
  你的应用 ──→ .swbn 标准组件
                ↓
  ┌─────────────┼─────────────┐
  Browser      Native         Embedded
  (V8/WASM)   (WAMR/WASM3)   (WASM3-micro)
                ↓
         同一套代码，全平台运行
```

**关键优势：**
- **写一次，跑所有平台** — 浏览器、桌面、移动端、IoT 同一套 `.swbn`
- **浏览器深度集成** — Chromium 是系统原生渲染层，不是外挂
- **强制标准化** — 不遵循 SSI 接口的组件无法接入系统
- **安全优先** — 每个组件沙箱隔离，权限显式声明，行为可审计

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────┐
│   L5: 应用层 — 你的 WASM 应用 (.swbn 格式)              │
├─────────────────────────────────────────────────────────┤
│   L4: 系统服务层 — 浏览器引擎 / 存储 / 计算 / 网络      │
│   接口: SSI-BR / SSI-DB / SSI-AI / SSI-NET              │
│   └─ 🔗 SSI-NET 由 cloudflared/sidecar 提供             │
├─────────────────────────────────────────────────────────┤
│   L3: 内核层 — WASM 运行时 / SSI 总线 / 安全模块        │
│   接口: SSI-KRN / SSI-SEC                               │
├─────────────────────────────────────────────────────────┤
│   L2: 硬件抽象层 — 设备信息 / GPU / 传感器              │
│   接口: SSI-HAL                                         │
├─────────────────────────────────────────────────────────┤
│   L1: 物理层 — 手机 / 电脑 / 浏览器 / 服务器 / IoT      │
└─────────────────────────────────────────────────────────┘
```

> 🌐 **免费网络层**：SSI-NET 通过 [cloudflared/sidecar](https://github.com/ghshhf/cloudflared) 提供 18 种传输协议后端，利用 Cloudflare 免费隧道、TCP 直连、P2P、DNS/ICMP 隧道等多种方式实现零成本网络穿透。详见 [docs/SSI-NET.md](docs/SSI-NET.md)。

---

## 📦 安装包下载

| 平台 | 下载 | 说明 |
|------|------|------|
| **Windows** | `SkyNet-SSI-Runtime-Setup.exe` | 双击安装，自动创建桌面快捷方式 |
| **Android** | `SkyNet-SSI-Runtime.apk` | 安装后全屏运行 SSI 仪表盘 |
| **浏览器** | PWA | 访问 `test/browser-test.html` → "添加到桌面" |

---

## 🚀 快速开始

### 运行 SSI 演示（验证全栈）

```bash
npx ts-node ssi/demo/demo.ts
```

输出示例：
```
══════════════════════════════════════════════════
  SkyNet SSI Integration Demo v1.0
══════════════════════════════════════════════════
[OK] Component Registry: 3 components
[OK] SSI Service Bus: Running
[OK] Browser Engine: full mode (WebGPU)
[OK] IPC Routing: 5 messages delivered
✅ Demo Complete — SSI Stack Verified
```

### 构建你的第一个 .swbn 组件

```bash
# 1. 初始化组件
npx ts-node ssi/packager/src/index.ts init --name my-app --type browser

# 2. 编写代码 (main.wasm)
# 3. 构建 .swbn 包
npx ts-node ssi/packager/src/index.ts build . --output my-app.swbn

# 4. 验证
npx ts-node ssi/packager/src/index.ts info my-app.swbn
```

### 构建 WASM 组件（需要 Emscripten）

```bash
./build-wasm.sh --output-format swbn gpkg/zlib-wasm
# → output/zlib-1.3.2-browser-wasm32.swbn
```

---

## 📚 核心文档

| 文档 | 内容 | 读完能做什么 |
|------|------|-------------|
| [SYSTEM-STANDARD.md](SYSTEM-STANDARD.md) | 系统架构总纲 — 5 层模型、设计哲学 | 理解 SkyNet 为什么这样设计 |
| [SPEC-INTERFACE.md](SPEC-INTERFACE.md) | SSI 接口规范 — 12 个接口完整 IDL | 开发符合标准的组件 |
| [docs/SSI-NET.md](docs/SSI-NET.md) | 网络层架构 — 18 种传输协议 + 智能路由 | 理解 SkyNet 的免费网络层 |
| [specs/component-model.md](specs/component-model.md) | .swbn 包格式详解 | 打包、签名、验证组件 |
| [specs/ipc.md](specs/ipc.md) | IPC 总线二进制协议 | 实现跨组件通信 |
| [specs/security.md](specs/security.md) | 安全模型与权限体系 | 设计安全的组件架构 |

---

## 🧩 SSI 接口一览

| 接口 | 职责 | 状态 | 位置 |
|------|------|------|------|
| **SSI-CORE** | 组件生命周期、注册表、消息通信 | ✅ 参考实现完成 | `ssi/core/` |
| **SSI-BR** | 浏览器渲染、脚本执行、GPU 计算 | ✅ 三模渲染实现 | `ssi/browser/` |
| **SSI-BUS** | 服务总线、二进制 IPC、优先级队列 | ✅ 实现完成 | `ssi/bus/` |
| **SSI-UI** | 窗口管理、合成器、输入路由 | ✅ 实现完成 | `ssi/ui/` |
| **SSI-DB** | KV 存储、P2P 存储、存储证明 | ✅ 实现完成 | `ssi/db/` |
| **SSI-AI** | AI 任务调度、模型分发、推理 | ✅ 实现完成 | `ssi/ai/` |
| **SSI-SEC** | 身份、加密、权限、审计 | ✅ 实现完成 | `ssi/sec/` |
| **SSI-HAL** | 硬件抽象、传感器、电池 | ✅ 实现完成 | `ssi/hal/` |
| **SSI-NET** 🔗 | 免费网络层 — 18 种协议后端 + 智能路由 | ✅ **已在 cloudflared/sidecar 实现** | [ghshhf/cloudflared](https://github.com/ghshhf/cloudflared) → `docs/SSI-NET.md` |
| **SSI-KRN** | WASM 运行时、进程、内存管理 | ✅ 参考实现完成 | `browser-runtime/` |
| **PACKAGER** | .swbn 组件打包 CLI | ✅ 实现完成 | `ssi/packager/` |
| **SSI-FS** | 虚拟文件系统、多后端 | ❌ **尚未实现** | _待开发_ |

---

## 📁 项目结构

```
SkyNet/
├── 📜 SYSTEM-STANDARD.md          ← 系统宪法
├── 📜 SPEC-INTERFACE.md           ← SSI 接口规范
├── 📜 AI-TP-OS-Architecture.md    ← 操作系统架构
│
├── 📂 specs/                      ← 子规范
│   ├── component-model.md         ← .swbn 格式
│   ├── ipc.md                     ← IPC 总线
│   ├── wasm-runtime.md            ← WASM 运行时
│   └── security.md                ← 安全模型
│
├── 📂 ssi/                        ← 参考实现
│   ├── core/                      ← SSI-CORE 组件基座
│   ├── bus/                       ← SSI 服务总线
│   ├── browser/                   ← SSI-BR 浏览器引擎
│   ├── ui/                        ← SSI-UI 窗口系统
│   ├── ai/                        ← SSI-AI 计算引擎
│   ├── db/                        ← SSI-DB 存储引擎
│   ├── hal/                       ← SSI-HAL 硬件抽象
│   ├── sec/                       ← SSI-SEC 安全模块
│   ├── fs/                        ← SSI-FS 文件系统（开发中）
│   ├── packager/                  ← .swbn 打包 CLI
│   ├── demo/                      ← 端到端演示
│   └── demo-component/            ← 组件示例
│
├── 📂 browser-runtime/            ← SSI-KRN 运行时内核
├── 📂 desktop/                    ← Windows 安装包源码
├── 📂 android/                    ← Android APK 源码
│
├── 📂 cross-platform/             ← 构建系统
│   ├── toolchains/                ← 编译工具链
│   ├── platforms/                 ← 平台配置
│   └── core/                      ← 构建步骤
│
├── 📂 gpkg/                       ← 标准组件包定义
│   ├── wasm-runtime/              ← WASM 运行时引导
│   └── zlib-wasm/                 ← zlib WASM 示例
│
├── 📂 test/                       ← 浏览器测试页面
├── 📂 dist/                       ← 安装包输出
└── 📂 public/                     ← PWA 资源
```

---

## 🛠️ 原生包构建（向后兼容）

SkyNet 仍然支持原有的跨平台包构建系统：

```bash
# 构建单个包
./build-cross.sh --clean --dep-order gpkg/zlib

# 全平台构建（自动检测 Emscripten 添加 WASM）
./build-all.sh

# 查询包元数据
curl -s https://raw.githubusercontent.com/ghshhf/glibc-packages/master/repo.json \
  | python3 -m json.tool
```

---

## 🤝 贡献

| 贡献类型 | 入口 | 规范 |
|---------|------|------|
| 包定义 | `gpkg/<name>/build.sh` | Termux 标准 + format.py 校验 |
| SSI 组件 | `ssi/<module>/` | SPEC-INTERFACE.md |
| 架构设计 | Issue / PR → SYSTEM-STANDARD.md | 需核心团队 review |

---

## 📜 协议

MIT — 属于 SkyNet (天网) / AI-TP 协议项目的一部分  
**维护者**: [@ghshhf](https://github.com/ghshhf)  
**仓库**: https://github.com/ghshhf/glibc-packages

---

## 📦 组件包目录

项目包含 **335 个** 标准系统组件包，覆盖基础库、编译工具、网络协议、图形、多媒体等 **14 大类**。

| 分类 | 数量 |
|------|:----:|
| 📚 基础 C 库 | 8 |
| 🛠️ 编译工具链 | 39 |
| 🗜️ 压缩与归档 | 20 |
| 🔐 加密与安全 | 19 |
| 🌍 网络协议与工具 | 30 |
| 🗄️ 数据库 | 10 |
| 🎨 图形与 GPU | 42 |
| 🎵 多媒体与音频 | 35 |
| ⌨️ 编辑器与终端 | 18 |
| ⚙️ 系统工具 | 62 |
| 📦 开发库 | 35 |
| 🖥️ 桌面与 GUI | 47 |
| 🌐 AI-TP 协议栈 | 13 |
| 🤖 AI 工具 | 4 |

📋 完整列表见 [PACKAGES.md](./PACKAGES.md)

---

> **"标准即系统。系统即标准。"**  
> 在 SkyNet 中，"标准化"不是目的——它是唯一的存在方式。
