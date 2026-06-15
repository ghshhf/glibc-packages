# glibc-packages

为 Termux 构建基于 glibc 的软件包仓库，现已扩展支持跨平台构建（Linux/Android/Windows/macOS/BSD）。

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