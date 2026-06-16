#!/bin/bash
# =============================================================================
# Platform: Linux (Native)
# =============================================================================
# Linux 本机平台配置。使用系统的 gcc/clang 和 glibc，无需交叉编译。
# 适用于：Debian/Ubuntu、Fedora/RHEL、Arch Linux 等主流发行版。
# =============================================================================

# 平台名称
export CROSS_PLATFORM_NAME="Linux (Native)"
export CROSS_PLATFORM_OS="linux"
export CROSS_PLATFORM_FAMILY="linux"

# 平台特性标志
export CROSS_HAS_ANDROID_NDK=false
export CROSS_USES_CGCT=false
export CROSS_HAS_BIONIC_COMPAT=false
export CROSS_HAS_SYSTEMD=true    # 大部分现代发行版使用 systemd
export CROSS_HAS_SELINUX=false   # 默认 false，由用户配置覆盖

# 包格式（根据发行版自动选择）
if command -v dpkg &>/dev/null; then
    export CROSS_PACKAGE_FORMAT="debian"
    export CROSS_PACKAGE_EXT=".deb"
elif command -v pacman &>/dev/null; then
    export CROSS_PACKAGE_FORMAT="pacman"
    export CROSS_PACKAGE_EXT=".pkg.tar.zst"
elif command -v rpm &>/dev/null; then
    export CROSS_PACKAGE_FORMAT="rpm"
    export CROSS_PACKAGE_EXT=".rpm"
else
    export CROSS_PACKAGE_FORMAT="tarball"
    export CROSS_PACKAGE_EXT=".tar.gz"
fi

# 默认前缀（Linux 标准路径）
export CROSS_PREFIX_DEFAULT="/usr/local"

# 架构映射
case "${CROSS_ARCH}" in
    x86_64)
        export CROSS_HOST_TRIPLE="x86_64-linux-gnu"
        export CROSS_TARGET_TRIPLE="x86_64-linux-gnu"
        ;;
    aarch64)
        export CROSS_HOST_TRIPLE="aarch64-linux-gnu"
        export CROSS_TARGET_TRIPLE="aarch64-linux-gnu"
        ;;
    i686)
        export CROSS_HOST_TRIPLE="i686-linux-gnu"
        export CROSS_TARGET_TRIPLE="i686-linux-gnu"
        ;;
    arm|armv7a)
        export CROSS_HOST_TRIPLE="arm-linux-gnueabihf"
        export CROSS_TARGET_TRIPLE="arm-linux-gnueabihf"
        ;;
esac

# 动态库扩展
export CROSS_SHARED_LIB_EXT=".so"
export CROSS_STATIC_LIB_EXT=".a"
export CROSS_EXECUTABLE_EXT=""

# 包依赖命名约定（Linux 使用标准名称，无后缀）
export CROSS_DEP_SUFFIX=""
export CROSS_DEP_GLIBC_NAME="libc6"

# CFLAGS / LDFLAGS 默认值（本机构建，不需要特殊参数）
export CROSS_DEFAULT_CFLAGS="-O2 -fPIC"
export CROSS_DEFAULT_LDFLAGS=""

# 构建标志
export CROSS_CROSS_COMPILING=false   # Linux 本机构建不需要交叉编译
export CROSS_NEEDS_NDK_PATCHES=false # 系统已有完整头文件
export CROSS_NEEDS_CGCT=false        # 系统已有 gcc/glibc

# shellcheck disable=SC2034
CROSS_PLATFORM_READY=true
