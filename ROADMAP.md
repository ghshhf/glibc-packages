# SkyNet (天网) 开发路线图

> **12 个月计划 — 从标准化框架到完整生态平台**  
> 维护者: [@ghshhf](https://github.com/ghshhf)

---

## 总览

```
Phase 1 (Month 1-2)  ─── 清理遗留 + 补全缺口
Phase 2 (Month 3-4)  ─── 平台工具链 + 格式迁移
Phase 3 (Month 5-8)  ─── 开发者生态
Phase 4 (Month 9-12) ─── 质量保障 + 低门槛贡献
```

---

## Phase 1: 清理遗留 + 补全缺口（Month 1-2）

| # | 项目 | 说明 | 工时 | 优先级 |
|---|------|------|------|--------|
| 1 | **核心组件 status 转正** | 9 个 SSI 组件从 `design` → `production` | 半天 | 🔴 P0 |
| 2 | **Phase 2 补坑: AI 任务调度器 + 去中心化寻址** | `ai-tp-scheduler` 实现 + ai-tp-address 去中心化地址解析 | 2-3 周 | 🔴 P0 |
| 3 | **反馈系统上线** | 桌面端一键 Report Issue (OAuth Device Flow + GitHub API) | 1 周 | 🟠 P1 |

### 1. 核心组件 status 转正

**现状**: `repo.json` 中 9 个核心组件均标记为 `"status": "design"`，实际参考实现已完成。

**操作**: 逐个验证代码完整性 → 更新 `repo.json` → 更新 README 状态标记。

**验证标准**:
- 每个组件有完整 TypeScript 实现
- 通过 vitest 单元测试
- 注册在 SSI 总线上可通过 demo 端到端验证

### 2. Phase 2 补坑

#### AI 任务调度器 (ai-tp-scheduler)

**目标**: 将 AI 计算任务分配到空闲节点，支持本地优先 + P2P 卸载。

**接口**:
```typescript
interface AITaskScheduler {
  submit(task: AITask): Promise<AITaskHandle>;
  cancel(handle: AITaskHandle): Promise<void>;
  getStatus(handle: AITaskHandle): Promise<AITaskStatus>;
  getAvailableResources(): Promise<ComputeResources>;
}
```

**调度策略**:
1. 本地队列（优先级 + 资源匹配）
2. P2P 卸载（无空闲本地资源时，通过 SSI-NET 分发到网络节点）
3. 结果收集与缓存（SSI-DB）

#### 去中心化寻址 (ai-tp-address)

**目标**: 替代 DNS，实现 `ai-tp:<base58-public-key-hash>` 格式的自主地址解析。

**方案**:
- DHT 分布式哈希表（参考 libp2p Kademlia）
- 本地缓存 + AI 预测热点节点
- 地址 = 公钥哈希 → 内置身份认证

### 3. 反馈系统上线

详见 [FEEDBACK-PLAN.md](./FEEDBACK-PLAN.md)，核心是桌面端 Electron 内嵌 Issue 创建流程。

**关键实现点**:
- OAuth Device Flow（用户无需手动输入 Token）
- 日志收集（最近 200 行 + 系统信息）
- 预览编辑器（提交前可编辑内容）
- 隐私提醒（GitHub Issue 公开性提示）

---

## Phase 2: 平台工具链 + 格式迁移（Month 3-4）

| # | 项目 | 说明 | 工时 | 优先级 |
|---|------|------|------|--------|
| 4 | **Python SDK 发布** | `pip install skynet-ssi` 上 PyPI | 3 天 | 🟠 P1 |
| 5 | **P2P 同步集成到 SSI 总线** | ai-tp-sync 挂接 SSI-BUS，取消独立运行 | 1 周 | 🟠 P1 |
| 6 | **335 包批量转 .swbn** | 脚本化批量转换 + 签名 | 2 周 | 🟠 P1 |

### 4. Python SDK

**API 设计**:
```python
from skynet import SkyNetRuntime

runtime = SkyNetRuntime()
runtime.load("path/to/component.swbn")
runtime.start()

# 调用 SSI 接口
result = runtime.call("storage", "get", key="my-key")
print(result)
```

**包结构**:
```
skynet-ssi/
├── skynet/
│   ├── __init__.py         # 公开 API
│   ├── runtime.py          # WASM 运行时桥接
│   ├── component.py        # 组件生命周期管理
│   ├── ssi_bus.py          # IPC 客户端
│   └── cli.py              # 命令行入口
├── setup.py
└── README.md
```

优先支持 Python 3.10+，通过 `subprocess` 或 `ctypes` 桥接 Node.js 运行时（后续可切到纯 Rust/WAMR 绑定）。

### 5. P2P 同步集成到 SSI 总线

**现状**: `ai-tp-sync` 是独立工具，通过 `gpkg/` 构建，不经过 SSI-BUS。

**改造**: 封装为 SSI 标准组件：
- manifest.json 声明 SSI-CORE + SSI-NET 接口依赖
- 通过 SSI-BUS 接收同步命令（`sync:start`、`sync:pause`、`sync:status`）
- 通过 SSI-FS 读写本地文件
- 通过 SSI-SEC 签名验证文件完整性

### 6. 335 包批量转 .swbn

**转换格式**:
```
传统包: gpkg/<name>/build.sh
  ↓
.swbn 包:
gpkg/<name>/
├── build.sh              # 保留（向后兼容）
├── manifest.json         # 新增：SSI 组件清单
├── main.wasm             # 新增：WASM 二进制（Emscripten 编译）
└── signature.sig         # 新增：Ed25519 签名
```

**脚本方案**: `scripts/convert-to-swbn.sh`
- 解析 `build.sh` 获取版本/SHA256/依赖
- 生成 `manifest.json`
- 调用 Emscripten 编译 WASM
- 调用 `ssi/packager` 打包签名

---

## Phase 3: 开发者生态（Month 5-8）

| # | 项目 | 说明 | 工时 | 优先级 |
|---|------|------|------|--------|
| 7 | **统一 `skynet` CLI** | Go/Rust 二进制工具链，零依赖 | 2 周 | 🟡 P2 |
| 8 | **SSI 组件注册中心 (swbn Registry)** | `skynet publish` / `skynet install` | 3 周 | 🟡 P2 |
| 9 | **开发者门户 / 交互式文档** | VitePress + Playground | 2 周 | 🟡 P2 |
| 10 | **iOS 运行时支持** | Swift/SwiftUI 原生运行时 | 3-4 周 | 🟡 P2 |

### 7. `skynet` CLI

**语言选择**: Rust（性能 + 跨平台 + WAMR 绑定便利）或 Go（SSI-NET 复用）。

**命令矩阵**:

| 命令 | 功能 | 替代当前 |
|------|------|----------|
| `skynet init` | 初始化 .swbn 组件模板 | `npx ts-node ssi/packager/src/index.ts init` |
| `skynet build` | 构建 .swbn 包 | `build-cross.sh --output-format swbn` |
| `skynet run` | 在本地运行时启动组件 | `npx tsx ssi/demo/demo.ts` |
| `skynet test` | 运行 SSI 合规性测试 | `npx vitest run` |
| `skynet publish` | 发布到组件注册中心 | — |
| `skynet install` | 从注册中心安装组件 | `git clone` |
| `skynet inspect` | 查看组件元数据和接口签名 | 手动解压 |
| `skynet doctor` | 环境诊断（检查工具链依赖） | — |

### 8. swbn Registry

**后端选型**:

| 方案 | 成本 | 优势 | 劣势 |
|------|------|------|------|
| GitHub Packages | 免费 | 开箱即用，npm 兼容 | 去中心化不足 |
| Verdaccio | 免费 | 自托管，完全控制 | 需要服务器 |
| IPFS | 免费 | 去中心化 | 延迟高，GC 复杂 |

**推荐**: 先用 GitHub Packages（最快上线），后续迁移到 IPFS。

**skynet.json (组件包元数据)**:
```json
{
  "name": "@skynet/zlib",
  "version": "1.3.2",
  "ssi": {
    "interfaces": ["SSI-CORE"],
    "api_version": "1.0.0"
  },
  "wasm": {
    "runtime": "wamr",
    "memory_min": 65536,
    "memory_max": 1048576
  },
  "dependencies": {},
  "permissions": []
}
```

### 9. 开发者门户

**站点结构**:
```
skynet.dev/
├── /                     # 首页 + 项目概览
├── /docs/                # VitePress 文档
│   ├── getting-started   # 快速开始
│   ├── guides/           # 教程（组件开发、SSI 接口、打包）
│   └── references/       # API 参考（SPEC-INTERFACE 渲染）
├── /playground/          # 浏览器内 WASM Playground
│   ├── editor            # Monaco Editor
│   ├── compiler          # WASM 编译（Emscripten 或 Binaryen）
│   └── runner            # SSI-KRN 沙箱执行
└── /blog/                # 发布说明 + 技术文章
```

### 10. iOS 运行时

**技术栈**: Swift 5.9+ / SwiftUI / WAMR（C 绑定）

**核心模块**:
```
SkyNet iOS App/
├── Runtime/
│   ├── SSIKernel.swift        # SSI-KRN 桥接
│   ├── WASMRunner.swift       # WAMR 绑定层
│   └── ComponentManager.swift # 组件生命周期
├── Services/
│   ├── SSIBusService.swift    # IPC 总线 (Actor 模型)
│   ├── StorageService.swift   # SSI-DB (FileManager + CloudKit)
│   └── NetworkService.swift   # SSI-NET (NWConnection)
├── UI/
│   ├── DashboardView.swift    # 仪表盘主页
│   ├── ComponentListView.swift# 已安装组件列表
│   └── LogView.swift          # 运行时日志
└── Store/
    └── SettingsStore.swift    # 配置持久化
```

---

## Phase 4: 质量保障 + 低门槛贡献（Month 9-12）

| # | 项目 | 说明 | 工时 | 优先级 |
|---|------|------|------|--------|
| 11 | **SSI 合规性自动化测试套件** | 类似 Web Platform Tests 的 SSI CTS | 2 周 | 🟢 P3 |
| 12 | **Web IDE 浏览器内 .swbn 构建器** | PWA 开发环境，零安装 | 3 周 | 🟢 P3 |

### 11. SSI 合规性测试套件

**架构**:
```
ssi/conformance/
├── suites/
│   ├── ssi-core.yaml     # 生命周期: init/start/suspend/resume/stop/destroy
│   ├── ssi-br.yaml       # 渲染 + 脚本执行 + GPU
│   ├── ssi-bus.yaml      # 消息路由 + 优先级 + 多播
│   ├── ssi-sec.yaml      # 签名 + 加密 + 权限 + 审计
│   ├── ssi-fs.yaml       # 文件操作 + 多后端一致性
│   ├── ssi-db.yaml       # KV + P2P + 存储证明
│   ├── ssi-net.yaml      # 连接 + 发现 + NAT 穿透
│   ├── ssi-ai.yaml       # 任务提交 + 状态 + 推理
│   └── ssi-hal.yaml      # 设备信息 + 传感器 + 电池
├── harness.ts            # 测试运行器（TS + vitest）
├── mock/                 # 标准化 mock 组件
│   ├── mock-core.swbn
│   └── mock-br.swbn
└── reporter.ts           # HTML 兼容性报告生成
```

**产出**: HTML 兼容性报告，每个接口列出 ✅/❌ 及失败原因，类似 [wpt.fyi](https://wpt.fyi)。

### 12. Web IDE

**核心功能**:

| 功能 | 技术方案 |
|------|----------|
| 代码编辑器 | Monaco Editor（VS Code 内核） |
| WASM 编译 | Binaryen 的 wasm-opt (WASM→WASM) + Emscripten (C→WASM) |
| WAT 汇编 | wat2wasm (WebAssembly Binary Toolkit) |
| 沙箱执行 | SSI-KRN (browser-runtime) 自举运行 |
| 实时预览 | iframe + postMessage IPC |
| 模板系统 | 预置 5 种模板：blank/browser/storage/network/ai |
| 一键发布 | `skynet publish` → swbn Registry |

**PWA 模式**: 离线可用，Service Worker 缓存编译器 WASM 模块和模板。

---

## 进度追踪

| # | 项目 | 优先级 | 阶段 | 状态 |
|---|------|--------|------|------|
| 1 | 核心组件 status 转正 | 🔴 P0 | Phase 1 | ⏳ 待开始 |
| 2 | AI 任务调度器 + 去中心化寻址 | 🔴 P0 | Phase 1 | ⏳ 待开始 |
| 3 | 反馈系统上线 | 🟠 P1 | Phase 1 | ⏳ 待开始 |
| 4 | Python SDK 发布 | 🟠 P1 | Phase 2 | ⏳ 待开始 |
| 5 | P2P 同步集成到 SSI 总线 | 🟠 P1 | Phase 2 | ⏳ 待开始 |
| 6 | 335 包批量转 .swbn | 🟠 P1 | Phase 2 | ⏳ 待开始 |
| 7 | 统一 `skynet` CLI | 🟡 P2 | Phase 3 | ⏳ 待开始 |
| 8 | swbn Registry | 🟡 P2 | Phase 3 | ⏳ 待开始 |
| 9 | 开发者门户 | 🟡 P2 | Phase 3 | ⏳ 待开始 |
| 10 | iOS 运行时 | 🟡 P2 | Phase 3 | ⏳ 待开始 |
| 11 | SSI 合规性测试套件 | 🟢 P3 | Phase 4 | ⏳ 待开始 |
| 12 | Web IDE 构建器 | 🟢 P3 | Phase 4 | ⏳ 待开始 |

---

> **"标准即系统。系统即标准。"**  
> ROADMAP v1.0 — 2026-07-02
