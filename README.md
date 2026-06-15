# glibc-packages

为 Termux 构建基于 glibc 的软件包仓库，现已扩展支持跨平台构建（Linux/Android/Windows/macOS/BSD）。

## 📦 最新更新（2026-06-15）

> commit: [`b9134502`](https://github.com/ghshhf/glibc-packages/commit/b9134502)

**跨平台构建系统 v2 — 统一架构重构**

| 能力 | 说明 |
|------|------|
| 统一 build-step 框架 | 6 步标准流水线：`prepare → configure → build → install → post_install → package` |
| Termux 变量适配器 | `pkg-adapter.sh` 自动将 `TERMUX_PKG_*` 翻译为 `CROSS_PKG_*` |
| BUILD_IN_SRC 支持 | autotools 老项目可在源码目录内构建（`CROSS_BUILDDIR = CROSS_SRCDIR`） |
| 依赖拓扑排序 | `--dep-order` 用 `tsort` 按 `TERMUX_PKG_DEPENDS` 自动排序构建顺序 |
| SHA256 校验 | 源码下载带校验，错误自动重新下载 |
| 产物命名统一 | `.tar.gz` 输出，不再产生 `.deb.tar.gz` 等歧义扩展名 |
| 多包隔离 | 每个包在独立子 shell 构建，环境变量互不污染 |
| GitHub Actions 矩阵 | Linux x86_64/aarch64 + macOS + Windows + FreeBSD + Android |

**文件变化**：
```
 build-cross.sh                     | +587 / -566   ← 主入口，改用 step 框架
 cross-platform/core/build-steps.sh | +393 / -0     ← 标准 6 步构建（新增）
 cross-platform/core/pkg-adapter.sh | +99 / -0      ← Termux 变量适配器（新增）
 build-all.sh                       | +196 / -0     ← 一键多平台调度（新增）
 .github/workflows/build-cross.yml  | +350 / -0     ← 5 平台 CI 矩阵（新增）
```

**快速开始**：
```bash
./build-cross.sh --clean --dep-order gpkg/zlib gpkg/neofetch gpkg/python
```

## 功能特性

- **跨平台支持**: 支持 Linux、Android、Windows、macOS、BSD 等多个平台
- **多架构支持**: x86_64、aarch64、i686、arm 等架构
- **自动工具链选择**: 根据平台自动选择 GCC、Clang、MinGW 等工具链
- **依赖名称映射**: 将 glibc-packages 的 `xxx-glibc` 命名转换为各平台原生包名
- **灵活的构建流程**: 支持 autotools、Cmake、Rust/Cargo 等多种构建系统

## 快速开始

### 构建单个包

```bash
# 构建 neofetch 包
./build-cross.sh gpkg/neofetch

# 清理并重新构建
./build-cross.sh --clean gpkg/neofetch
```

### 支持的包

| 包名 | 类型 | 状态 |
|------|------|------|
| neofetch | 纯脚本 | ✅ |
| zlib | configure | ✅ |
| time | autotools | ✅ |
| htop | autotools + ncurses | ✅ |
| sd | Rust/Cargo | ✅ |

## 项目结构

```
├── build-cross.sh          # 跨平台构建主入口
├── cross-platform/         # 跨平台框架模块
│   ├── platform-detect.sh  # 平台检测与配置
│   ├── platforms/          # 平台配置文件
│   ├── toolchains/         # 工具链配置
│   └── core/               # 核心模块
└── gpkg/                   # 包定义目录
    ├── neofetch/
    ├── zlib/
    ├── time/
    ├── htop/
    └── sd/
```

## 支持的平台

- Linux (x86_64, aarch64, i686)
- Android (aarch64, arm)
- Windows (x86_64, i686) - MinGW
- macOS (x86_64, arm64)
- BSD (FreeBSD, OpenBSD)

## 许可证

MIT License