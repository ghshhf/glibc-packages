#!/bin/bash
# =============================================================================
# Platform: Windows (MSYS2 / MinGW-w64)
# =============================================================================
# Windows 平台配置。通过 MSYS2/MinGW-w64 提供类 Unix 构建环境。
#
# 架构说明:
#   mingw64  - 64位 (x86_64-w64-mingw32)  ← 默认
#   mingw32  - 32位 (i686-w64-mingw32)
#   clang64 - 64位 Clang/LLVM 工具链
#   ucrt64  - 64位 UCRT 运行时
#
# 在 MSYS2 shell 中，$MSYSTEM 变量会自动设置为：
#   MINGW64 / MINGW32 / CLANG64 / UCRT64 / MSYS
# =============================================================================

# 平台名称
export CROSS_PLATFORM_NAME="Windows (MSYS2/MinGW-w64)"
export CROSS_PLATFORM_OS="windows"
export CROSS_PLATFORM_FAMILY="windows"

# 平台特性标志
export CROSS_HAS_ANDROID_NDK=false
export CROSS_USES_CGCT=false
export CROSS_HAS_BIONIC_COMPAT=false
export CROSS_HAS_SYSTEMD=false
export CROSS_HAS_SELINUX=false

# 包格式（MSYS2 使用 pacman）
export CROSS_PACKAGE_FORMAT="pacman"
export CROSS_PACKAGE_EXT=".pkg.tar.zst"

# 检测 MSYSTEM 环境变量确定子平台
_CROSS_MSYSTEM="${MSYSTEM:-MINGW64}"
case "${_CROSS_MSYSTEM}" in
    MINGW64|mingw64)
        _CROSS_MINGW_PREFIX="/mingw64"
        _CROSS_MINGW_TRIPLE="x86_64-w64-mingw32"
        export CROSS_ARCH="x86_64"
        ;;
    MINGW32|mingw32)
        _CROSS_MINGW_PREFIX="/mingw32"
        _CROSS_MINGW_TRIPLE="i686-w64-mingw32"
        export CROSS_ARCH="i686"
        ;;
    CLANG64|clang64)
        _CROSS_MINGW_PREFIX="/clang64"
        _CROSS_MINGW_TRIPLE="x86_64-w64-mingw32"
        export CROSS_ARCH="x86_64"
        export CROSS_USE_CLANG=true
        ;;
    UCRT64|ucrt64)
        _CROSS_MINGW_PREFIX="/ucrt64"
        _CROSS_MINGW_TRIPLE="x86_64-w64-mingw32"
        export CROSS_ARCH="x86_64"
        ;;
    MSYS|msys)
        _CROSS_MINGW_PREFIX="/usr"
        _CROSS_MINGW_TRIPLE="x86_64-pc-msys"
        export CROSS_ARCH="x86_64"
        ;;
    *)
        _CROSS_MINGW_PREFIX="/mingw64"
        _CROSS_MINGW_TRIPLE="x86_64-w64-mingw32"
        export CROSS_ARCH="x86_64"
        ;;
esac

# 默认前缀（在 MSYS2 shell 内使用 msys 路径）
export CROSS_PREFIX_DEFAULT="${_CROSS_MINGW_PREFIX}"

# 架构三元组
export CROSS_HOST_TRIPLE="${_CROSS_MINGW_TRIPLE}"
export CROSS_TARGET_TRIPLE="${_CROSS_MINGW_TRIPLE}"

# 动态库/可执行文件扩展
export CROSS_SHARED_LIB_EXT=".dll"
export CROSS_SHARED_LIB_PREFIX="lib"
export CROSS_STATIC_LIB_EXT=".a"
export CROSS_EXECUTABLE_EXT=".exe"
export CROSS_IMPORT_LIB_EXT=".dll.a"

# Windows 路径转换（MSYS2 自动处理，但显式声明）
export CROSS_WIN_PATH_STYLE="msys"   # msys / cygdrive / native
export CROSS_PATH_SEP="/"

# 包依赖命名约定（使用 MSYS2 标准命名）
export CROSS_DEP_SUFFIX=""
export CROSS_DEP_GLIBC_NAME="mingw-w64-x86_64-crt-git"

# CFLAGS / LDFLAGS
export CROSS_DEFAULT_CFLAGS="-O2 -fPIC -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00"
export CROSS_DEFAULT_LDFLAGS="-Wl,--enable-auto-import"

# Windows 特定标志
export CROSS_CROSS_COMPILING=false   # 在 MSYS2 shell 内部为本机编译
export CROSS_NEEDS_NDK_PATCHES=false
export CROSS_NEEDS_CGCT=false
export CROSS_HAS_WINAPI=true
export CROSS_HAS_DLL=true

# 清理
unset _CROSS_MSYSTEM _CROSS_MINGW_PREFIX _CROSS_MINGW_TRIPLE

# shellcheck disable=SC2034
CROSS_PLATFORM_READY=true
