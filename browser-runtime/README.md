# SkyNet SSI-KRN: WASM Runtime Kernel

这是 **SkyNet (天网)** 的 SSI-KRN 运行时内核。  
它实现 [SSI (System Standard Interface)](../SPEC-INTERFACE.md) v1.0 规范中的 **SSI-CORE** 和 **SSI-KRN** 接口。

**浏览器不是移植目标——它是系统的原生运行模式之一。**

## 功能

| SSI 接口 | 模块 | 功能 |
|----------|------|------|
| **SSI-KRN** | `wasm-loader.ts` | WASM 模块加载、缓存、生命周期管理 |
| **SSI-FS** | `virtual-fs.ts` | 虚拟文件系统（MEMFS/OPFS/IDBFS/NODEFS） |
| **SSI-NET** | `networking.ts` | 网络抽象（HTTP/WebSocket/WebRTC） |
| **SSI-KRN** | `threading.ts` | Web Worker 线程池 + SAB Atomix |
| **SSI-CORE** | `index.ts` | 组件生命周期 + .swbn 组件管理 |

## 标准化架构

本运行时是 SkyNet 系统标准架构的一部分：

```
SYSTEM-STANDARD.md   ← 系统标准架构（本文档）
SPEC-INTERFACE.md    ← SSI 接口规范
specs/
├── component-model.md  ← 标准组件模型 (.swbn)
├── ipc.md              ← IPC 总线协议
├── wasm-runtime.md     ← WASM 运行时规范
└── security.md         ← 安全模型
browser-runtime/        ← SSI-KRN 运行时实现（本包）
```

## 快速开始

```typescript
import { SsiRuntime, SSI_COMPONENT_STATE } from '@glibc-packages/browser-runtime';

// 初始化 SSI 运行时内核
const kernel = SsiRuntime.init({
  logLevel: 'info',
  cacheModules: true,
});

// 加载 .swbn 组件
const component = await kernel.loadComponent('/components/browser-engine.wasm', {
  /* SSI-CORE.init() config */
});

// 通过 SSI-CORE 生命周期启动
console.log(component.state); // SSI_COMPONENT_STATE.READY
await component.start();
console.log(component.state); // SSI_COMPONENT_STATE.RUNNING

// 直接加载 WASM 模块
const mod = await loadWasm({
  wasmUrl: '/lib/zlib.wasm',
  name: 'zlib',
});
```

## 运行模式

本运行时在所有 SkyNet 支持的平台上以统一方式运行：

| 模式 | 运行环境 | 技术栈 |
|------|---------|--------|
| **Browser Mode** | Web 浏览器 / PWA | Chromium V8 + Emscripten |
| **Native Mode** | Node.js / Electron / Deno | V8 + Node API |
| **Embedded Mode** | IoT / MCU | 微型 WASM 运行时 |

## License

MIT — 属于 SkyNet (天网) 项目的一部分
