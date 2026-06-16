#!/bin/bash
# =============================================================================
# Platform: BSD (FreeBSD / NetBSD / OpenBSD / DragonFly)
# =============================================================================
# BSD 家族平台配置。这些系统使用 clang 或 gcc，有自己的 ports/package 系统。
# =============================================================================

# 检测具体 BSD 变体
_CROSS_BSD_VARIANT="$(uname -s 2>/dev/null || echo FreeBSD)"

# 平台名称
case "${_CROSS_BSD_VARIANT}" in
    FreeBSD)
        export CROSS_PLATFORM_NAME="FreeBSD"
        export CROSS_PLATFORM_OS="freebsd"
        ;;
    NetBSD)
        export CROSS_PLATFORM_NAME="NetBSD"
        export CROSS_PLATFORM_OS="netbsd"
        ;;
    OpenBSD)
        export CROSS_PLATFORM_NAME="OpenBSD"
        export CROSS_PLATFORM_OS="openbsd"
        ;;
    DragonFly)
        export CROSS_PLATFORM_NAME="DragonFly BSD"
        export CROSS_PLATFORM_OS="dragonfly"
        ;;
    *)
        export CROSS_PLATFORM_NAME="BSD"
        export CROSS_PLATFORM_OS="bsd"
        ;;
esac

export CROSS_PLATFORM_FAMILY="bsd"

# 平台特性标志
export CROSS_HAS_ANDROID_NDK=false
export CROSS_USES_CGCT=false
export CROSS_HAS_BIONIC_COMPAT=false
export CROSS_HAS_SYSTEMD=false

# 包格式（各 BSD 有不同）
case "${_CROSS_BSD_VARIANT}" in
    FreeBSD)
        export CROSS_PACKAGE_FORMAT="pkg"
        export CROSS_PACKAGE_EXT=".txz"
        ;;
    OpenBSD)
        export CROSS_PACKAGE_FORMAT="ports"
        export CROSS_PACKAGE_EXT=".tgz"
        ;;
    NetBSD)
        export CROSS_PACKAGE_FORMAT="pkg_install"
        export CROSS_PACKAGE_EXT=".tgz"
        ;;
    *)
        export CROSS_PACKAGE_FORMAT="tarball"
        export CROSS_PACKAGE_EXT=".tar.gz"
        ;;
esac

# 默认前缀
export CROSS_PREFIX_DEFAULT="/usr/local"

# 架构三元组
case "${CROSS_ARCH}" in
    x86_64)
        export CROSS_HOST_TRIPLE="x86_64-unknown-${CROSS_PLATFORM_OS}"
        export CROSS_TARGET_TRIPLE="${CROSS_HOST_TRIPLE}"
        ;;
    aarch64|arm64)
        export CROSS_ARCH="aarch64"
        export CROSS_HOST_TRIPLE="aarch64-unknown-${CROSS_PLATFORM_OS}"
        export CROSS_TARGET_TRIPLE="${CROSS_HOST_TRIPLE}"
        ;;
    *)
        export CROSS_HOST_TRIPLE="${CROSS_ARCH}-unknown-${CROSS_PLATFORM_OS}"
        export CROSS_TARGET_TRIPLE="${CROSS_HOST_TRIPLE}"
        ;;
esac

# 动态库 / 可执行文件扩展
export CROSS_SHARED_LIB_EXT=".so"
export CROSS_STATIC_LIB_EXT=".a"
export CROSS_EXECUTABLE_EXT=""

# 包依赖命名约定
export CROSS_DEP_SUFFIX=""
export CROSS_DEP_GLIBC_NAME="libc"   # BSD 有自己的 libc (FreeBSD libc)

# CFLAGS / LDFLAGS
export CROSS_DEFAULT_CFLAGS="-O2 -fPIC"
export CROSS_DEFAULT_LDFLAGS=""

# 构建标志
export CROSS_CROSS_COMPILING=false
export CROSS_NEEDS_NDK_PATCHES=false
export CROSS_NEEDS_CGCT=false

# 清理
unset _CROSS_BSD_VARIANT

# shellcheck disable=SC2034
CROSS_PLATFORM_READY=true
