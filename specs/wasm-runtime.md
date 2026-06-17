# SSI WASM Runtime 规范

> **WebAssembly 运行时标准** — 系统所有应用的执行引擎规范。

---

## 1. 运行时层次

```
┌──────────────────────────────────────────┐
│            WASM 应用 (.swbn)               │
├──────────────────────────────────────────┤
│  标准导入接口 (SSI-IMPORT)                 │
│  - ssi_component_*                       │
│  - ssi_browser_*                         │
│  - ssi_storage_*                         │
│  - ssi_network_*                         │
├──────────────────────────────────────────┤
│  WASM 运行时引擎                           │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  │
│  │ WAMR    │  │ WASM3   │  │ V8 WASM │  │
│  │ (嵌入)   │  │ (轻量)  │  │ (浏览)  │  │
│  └─────────┘  └─────────┘  └─────────┘  │
├──────────────────────────────────────────┤
│  平台适配层                                │
│  Browser / Native Linux / Embedded        │
└──────────────────────────────────────────┘
```

## 2. WASM 导入规范

所有 WASM 模块可以导入以下标准接口：

```wat
;; SSI Core imports (always available)
(import "ssi" "component_init" (func ...))
(import "ssi" "component_start" (func ...))
(import "ssi" "component_stop" (func ...))
(import "ssi" "send_message" (func ...))
(import "ssi" "receive_message" (func ...))
(import "ssi" "log" (func ...))

;; SSI Browser imports (if permission granted)
(import "ssi_browser" "render" (func ...))
(import "ssi_browser" "execute_script" (func ...))
(import "ssi_browser" "fetch" (func ...))

;; SSI Storage imports (if permission granted)
(import "ssi_storage" "get" (func ...))
(import "ssi_storage" "put" (func ...))
(import "ssi_storage" "delete" (func ...))

;; WASI imports (standard POSIX)
(import "wasi_snapshot_preview1" "fd_write" (func ...))
(import "wasi_snapshot_preview1" "fd_read" (func ...))
(import "wasi_snapshot_preview1" "environ_get" (func ...))
```

## 3. 内存模型

| 参数 | 最小值 | 默认值 | 最大值 |
|------|--------|-------|--------|
| 初始内存 | 1MB | 64MB | — |
| 最大内存 | — | 512MB | 4GB |
| 栈大小 | 64KB | 256KB | 8MB |
| 表大小 | 1 | 1024 | 10M |

## 4. 运行时后端选择策略

```c
SsiRuntimeBackend select_backend(SsiDeviceInfo device) {
    if (device.platform == "browser") {
        return V8_WASM;       // 优先使用 V8 内置 WASM
    }
    if (device.total_ram < 64 * 1024 * 1024) {
        return WASM3;         // 低内存设备用轻量运行时
    }
    if (device.os == "linux" || device.os == "android") {
        return WAMR;          // Linux 用 Intel 的 WAMR
    }
    return WAMR;              // 默认
}
```
