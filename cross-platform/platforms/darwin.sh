#!/bin/bash
# =============================================================================
# Platform: macOS / Darwin
# =============================================================================
# macOS 平台配置。使用 Apple clang 或 GNU gcc，基于 BSD 用户空间 + XNU 内核。
#
# 注意：macOS 使用 dyld 动态链接器，有自己的框架系统 (Frameworks)。
# 包管理通常使用 Homebrew，但本框架支持直接构建。
# =============================================================================

# 平台名称
export CROSS_PLATFORM_NAME="macOS (Darwin)"
export CROSS_PLATFORM_OS="darwin"
export CROSS_PLATFORM_FAMILY="bsd"

# 平台特性标志
export CROSS_HAS_ANDROID_NDK=false
export CROSS_USES_CGCT=false
export CROSS_HAS_BIONIC_COMPAT=false
export CROSS_HAS_SYSTEMD=false
export CROSS_HAS_SELINUX=false
export CROSS_HAS_LAUNCHD=true   # macOS 使用 launchd

# 包格式
export CROSS_PACKAGE_FORMAT="tarball"
export CROSS_PACKAGE_EXT=".tar.gz"

# 默认前缀（与 Homebrew 兼容）
case "${CROSS_ARCH}" in
    x86_64)
        export CROSS_PREFIX_DEFAULT="/usr/local"
        export CROSS_HOST_TRIPLE="x86_64-apple-darwin"
        export CROSS_TARGET_TRIPLE="x86_64-apple-darwin"
        ;;
    aarch64|arm64)
        export CROSS_ARCH="aarch64"
        export CROSS_PREFIX_DEFAULT="/opt/homebrew"
        export CROSS_HOST_TRIPLE="arm64-apple-darwin"
        export CROSS_TARGET_TRIPLE="arm64-apple-darwin"
        ;;
    *)
        export CROSS_PREFIX_DEFAULT="/usr/local"
        export CROSS_HOST_TRIPLE="x86_64-apple-darwin"
        export CROSS_TARGET_TRIPLE="x86_64-apple-darwin"
        ;;
esac

# 动态库 / 可执行文件 扩展
export CROSS_SHARED_LIB_EXT=".dylib"
export CROSS_STATIC_LIB_EXT=".a"
export CROSS_EXECUTABLE_EXT=""

# 包依赖命名约定
export CROSS_DEP_SUFFIX=""
export CROSS_DEP_GLIBC_NAME="System"  # macOS 使用 libSystem.B.dylib，非 glibc

# CFLAGS / LDFLAGS
export CROSS_DEFAULT_CFLAGS="-O2 -fPIC -mmacosx-version-min=11.0"
export CROSS_DEFAULT_LDFLAGS="-Wl,-rpath,${CROSS_PREFIX}/lib"

# 构建标志
export CROSS_CROSS_COMPILING=false
export CROSS_NEEDS_NDK_PATCHES=false
export CROSS_NEEDS_CGCT=false
export CROSS_USES_CLANG=true   # macOS 默认使用 clang

# shellcheck disable=SC2034
CROSS_PLATFORM_READY=true
