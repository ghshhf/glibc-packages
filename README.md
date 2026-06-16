# glibc-packages
面向公共 API 的跨平台软件包构建系统。为 Termux、Linux、Android、Windows、macOS、BSD 提供统一的 glibc 软件包构建与发布能力。

> 本项目以 [public-apis](https://github.com/ghshhf/public-apis) 为规范参考，采用一致的文档格式、验证脚本与 CI 工作流。

<br >

## 📦 快速开始

在本地构建一个软件包：

```bash
./build-cross.sh --clean --dep-order gpkg/zlib
```

通过公共 API 查询包信息：

```bash
# 查询包元数据
curl -s https://raw.githubusercontent.com/ghshhf/glibc-packages/master/repo.json | python3 -m json.tool
```

<br >

## Index

* [核心能力](#核心能力)
* [公共 API 概览](#公共-api-概览)
* [支持的软件包](#支持的软件包)
* [支持的平台与架构](#支持的平台与架构)
* [构建系统](#构建系统)
* [项目结构](#项目结构)
* [贡献指南](#贡献指南)
* [路线图](#路线图)

<br >

## 核心能力

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

<br >

## 公共 API 概览

本项目对外暴露的可程序化调用接口：

| API | Description | Auth | HTTPS | CORS |
| --- | --- | --- | --- | --- |
| [repo.json](https://raw.githubusercontent.com/ghshhf/glibc-packages/master/repo.json) | 仓库元数据与包索引（JSON 格式） | No | Yes | Yes |
| [build-cross.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-cross.sh) | 跨平台构建主入口（CLI API） | No | Yes | — |
| [build-package.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-package.sh) | 单包构建脚本（CLI API） | No | Yes | — |
| [get-build-package.sh](https://github.com/ghshhf/glibc-packages/blob/master/get-build-package.sh) | 构建脚本拉取工具（CLI API） | No | Yes | — |
| [generate-build-scripts.sh](https://github.com/ghshhf/glibc-packages/blob/master/generate-build-scripts.sh) | 构建脚本生成器（CLI API） | No | Yes | — |
| [gpkg/\*/build.sh](https://github.com/ghshhf/glibc-packages/tree/master/gpkg) | 单包元数据 API（`TERMUX_PKG_*` 变量规范） | No | Yes | — |

### 各平台构建 API

| API | Description | Auth | HTTPS | CORS |
| --- | --- | --- | --- | --- |
| [build-linux.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-linux.sh) | Linux 平台构建脚本 | No | Yes | — |
| [build-android.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-android.sh) | Android/Termux 平台构建脚本 | No | Yes | — |
| [build-windows.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-windows.sh) | Windows/MinGW 平台构建脚本 | No | Yes | — |
| [build-darwin.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-darwin.sh) | macOS 平台构建脚本 | No | Yes | — |
| [build-bsd.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-bsd.sh) | FreeBSD/OpenBSD 平台构建脚本 | No | Yes | — |
| [build-all.sh](https://github.com/ghshhf/glibc-packages/blob/master/build-all.sh) | 一键多平台调度脚本 | No | Yes | — |

### 辅助脚本 API

| API | Description | Auth | HTTPS | CORS |
| --- | --- | --- | --- | --- |
| [install-deps.sh](https://github.com/ghshhf/glibc-packages/blob/master/install-deps.sh) | 自动安装构建依赖 | No | Yes | — |
| [clean.sh](https://github.com/ghshhf/glibc-packages/blob/master/clean.sh) | 清理构建产物 | No | Yes | — |
| [big-pkgs.list](https://github.com/ghshhf/glibc-packages/blob/master/big-pkgs.list) | 大型包清单（资源占用参考） | No | Yes | — |

<br >

## 支持的软件包

### 基础系统库

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |
| [zlib](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zlib) | 无损数据压缩库 | configure | All | ✅ |
| [ncurses](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ncurses) | 终端 UI 库（htop 等依赖） | configure | All | ✅ |
| [readline](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/readline) | GNU 命令行编辑库 | autotools | Linux / macOS / BSD | ✅ |
| [libevent](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libevent) | 异步事件通知库 | autotools | Linux / macOS / BSD | ✅ |
| [openssl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/openssl) | TLS/SSL 与通用加密库 | custom | All | ✅ |

### 系统工具

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |
| [neofetch](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/neofetch) | 系统信息展示工具 | Shell script | All | ✅ |
| [htop](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/htop) | 交互式进程监视器 | autotools + ncurses | Linux / macOS / BSD | ✅ |
| [time](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/time) | GNU time 计时器 | autotools | Linux / macOS / BSD | ✅ |
| [sd](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/sd) | 快速 sed 替代（CLI 文本替换） | Rust/Cargo | All | ✅ |
| [tmux](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tmux) | 终端多路复用器 | autotools | Linux / macOS / BSD | ✅ |

### 编程语言

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |
| [python](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/python) | Python 3 解释器与标准库 | autotools | All | ✅ |
| [ruby](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ruby) | Ruby 解释器 | autotools | Linux / macOS / BSD | ✅ |
| [nodejs](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/nodejs) | Node.js JavaScript 运行时 | custom | All | ✅ |
| [rust](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/rust) | Rust 工具链（rustc + cargo） | custom | All | ✅ |

### 网络层包（AI-TP OS 依赖）

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |
| [libp2p](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libp2p) | P2P 网络基础库（go-libp2p） | Go | Linux / macOS | ✅ |
| [nat-pmp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/nat-pmp) | NAT 端口映射协议实现 | autotools | Linux / macOS / BSD | ✅ |
| [libstun](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libstun) | STUN/TURN 客户端库 | autotools | Linux / macOS / BSD | ✅ |
| [libquic](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libquic) | QUIC 协议实现（基于 ngtcp2） | autotools | All | ✅ |
| [ai-tp-nat](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-nat) | AI-TP 增强型 NAT 穿透工具（自研） | Rust | Linux / macOS | ✅ |

### 存储层包（AI-TP OS 依赖）

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |
| [libai-storage](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libai-storage) | AI-TP 统一存储 API 抽象层（自研） | CMake | All | ✅ |
| [syncthing](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/syncthing) | P2P 文件同步工具 | Go | Linux / macOS / BSD | ✅ |
| [ai-tp-sync](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-sync) | AI-TP 内容寻址文件同步（自研） | Rust | All | ✅ |
| [leveldb](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/leveldb) | 键值存储引擎（本地元数据存储） | CMake | All | ✅ |
| [sqlite3](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/sqlite3) | 轻量级关系数据库 | autotools | All | ✅ |

### 计算层包（AI-TP OS 依赖）

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |
| [ai-tp-scheduler](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-scheduler) | AI-TP 分布式 AI 任务调度器（自研） | Rust | Linux / macOS | ✅ |
| [ollama](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ollama) | 本地 LLM 推理服务 | Go | Linux / macOS | ✅ |
| [localai](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/localai) | 本地 AI 推理引擎（fork） | CMake + Go | Linux / macOS | ✅ |
| [ggml](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ggml) | 轻量级张量计算库（llama.cpp 底层） | CMake | All | ✅ |

### 开发工具

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |
| [git](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/git) | 分布式版本控制系统 | autotools | All | ✅ |
| [cmake](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/cmake) | 跨平台构建系统 | CMake | All | ✅ |
| [curl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/curl) | 命令行 HTTP 客户端 | autotools | All | ✅ |
| [jq](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/jq) | 命令行 JSON 处理器 | autotools | All | ✅ |

<br >

## 支持的平台与架构

| Platform | Architectures | Toolchain | Status |
| --- | --- | --- | --- |
| **Linux** | x86_64, aarch64, i686 | GCC / Clang | ✅ |
| **Android (Termux)** | aarch64, arm | Clang (NDK) | ✅ |
| **Windows** | x86_64, i686 | MinGW-w64 (GCC) | ✅ |
| **macOS** | x86_64, arm64 | Clang (Xcode) | ✅ |
| **FreeBSD** | x86_64, aarch64 | Clang / GCC | ✅ |
| **OpenBSD** | x86_64 | Clang | 📋 |
| **NetBSD** | x86_64 | GCC | 📋 |

<br >

## 构建系统

### 标准 6 步流水线

```
prepare → configure → build → install → post_install → package
```

### 构建系统自动识别

| Method | Detection | Pipeline |
| --- | --- | --- |
| Autotools | `./configure` exists → `./configure && make && make install` | ✅ |
| CMake | `CMakeLists.txt` exists 或 `TERMUX_PKG_FORCE_CMAKE=true` → CMake build | ✅ |
| Meson | `meson.build` exists → Meson + ninja | ✅ |
| Rust/Cargo | `Cargo.toml` exists → `cargo build --release` | ✅ |
| Go | `go.mod` exists → `go build` | ✅ |
| Custom Makefile | `Makefile` exists + `TERMUX_PKG_BUILD_IN_SRC=true` → `make && make install` | ✅ |
| Shell script | 纯脚本项目 → 直接安装文件到目标目录 | ✅ |

<br >

## 项目结构

```
├── README.md                   # 项目说明与公共 API 索引
├── CONTRIBUTING.md             # 贡献指南与包定义格式
├── SECURITY.md                 # 安全策略
├── LICENSE.md                  # 许可证
├── repo.json                   # 仓库元数据（机器可读 API）
├── big-pkgs.list               # 大型包清单
├── AI-TP-OS-Architecture.md    # AI-TP OS 架构文档
│
├── build-cross.sh              # 跨平台构建主入口
├── build-package.sh            # 单包构建脚本
├── build-all.sh                # 一键多平台调度
├── get-build-package.sh        # 构建脚本拉取工具
├── generate-build-scripts.sh   # 构建脚本生成器
├── install-deps.sh             # 构建依赖自动安装
├── clean.sh                    # 构建产物清理
│
├── build-linux.sh              # Linux 平台构建
├── build-android.sh            # Android/Termux 平台构建
├── build-windows.sh            # Windows/MinGW 平台构建
├── build-darwin.sh             # macOS 平台构建
├── build-bsd.sh                # FreeBSD/OpenBSD 平台构建
│
├── cross-platform/             # 跨平台框架模块
│   ├── platform-detect.sh      # 平台检测与配置
│   ├── platforms/              # 各平台配置文件
│   ├── toolchains/             # 工具链配置
│   └── core/                   # 核心模块 (build-steps, pkg-adapter)
│
├── gpkg/                       # 包定义目录
│   ├── zlib/                   # 每个包一个目录
│   │   ├── build.sh            # 包元数据与构建变量
│   │   └── *.patch             # 可选补丁文件
│   ├── neofetch/
│   ├── htop/
│   └── ...
│
├── scripts/                    # 验证与测试脚本（与 public-apis 规范对齐）
│   ├── validate/               # Python 验证模块
│   │   └── format.py           # 包定义格式校验
│   ├── tests/                  # Python 单元测试
│   ├── requirements.txt        # Python 依赖
│   └── README.md               # 脚本使用说明
│
└── .github/
    ├── workflows/              # GitHub Actions 工作流
    │   ├── test_of_push_and_pull.yml   # push/pull 触发的格式校验
    │   └── validate_links.yml          # 定时链接校验
    ├── ISSUE_TEMPLATE.md       # Issue 模板
    └── PULL_REQUEST_TEMPLATE.md # PR 模板
```

<br >

## 贡献指南

欢迎提交新包、修复构建问题或改进文档！详见 [CONTRIBUTING.md](CONTRIBUTING.md)。

关键要点：
- 包定义统一使用 `TERMUX_PKG_*` 变量规范
- 每个 PR 在合并前会经过 format.py 格式校验
- 包名使用 `-glibc` 后缀（glibc 本身除外）
- 描述不超过 100 字符，不以标点结尾
- 表格对齐遵循与 [public-apis](https://github.com/ghshhf/public-apis) 一致的 Markdown 规范

<br >

## 路线图

| 阶段 | 时间 | 目标 |
| --- | --- | --- |
| **第一阶段：基础架构** | 1-2 月 | 实现统一 build-step 框架 + Termux 变量适配器 + 5 平台 CI 矩阵 |
| **第二阶段：包生态扩展** | 2-3 月 | 补齐网络层、存储层、计算层依赖包（AI-TP OS 开发基础） |
| **第三阶段：公共 API 发布** | 3-4 月 | 发布稳定的包元数据查询 API、在线包浏览器、Webhook 推送 |

<br >

## 许可证

MIT License — 详见 [LICENSE.md](LICENSE.md)

**维护者**: [@ghshhf](https://github.com/ghshhf)
