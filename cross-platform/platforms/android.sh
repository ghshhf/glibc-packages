#!/bin/bash
# =============================================================================
# Platform: Android / Termux
# =============================================================================
# Android 平台配置。复用原有的 CGCT 交叉编译工具链和 NDK 补丁。
# 这是项目的原始目标平台，保持最大的向后兼容性。
# =============================================================================

# 平台名称
export CROSS_PLATFORM_NAME="Android (Termux)"
export CROSS_PLATFORM_OS="android"
export CROSS_PLATFORM_FAMILY="linux"

# 平台特性标志
export CROSS_HAS_ANDROID_NDK=true
export CROSS_USES_CGCT=true
export CROSS_HAS_BIONIC_COMPAT=true   # 支持 bionic 头文件兼容
export CROSS_HAS_SELINUX=false
export CROSS_HAS_SYSTEMD=false

# 包格式
export CROSS_PACKAGE_FORMAT="pacman"
export CROSS_PACKAGE_EXT=".pkg.tar.xz"

# 默认前缀（Android Termux 标准路径）
export CROSS_PREFIX_DEFAULT="/data/data/com.termux/files/usr"

# 架构映射：统一架构名 → Android 工具链三元组
case "${CROSS_ARCH}" in
    x86_64)
        export CROSS_HOST_TRIPLE="x86_64-linux-android"
        export CROSS_TARGET_TRIPLE="x86_64-linux-android"
        export CROSS_ANDROID_API_LEVEL="24"
        ;;
    aarch64)
        export CROSS_HOST_TRIPLE="aarch64-linux-android"
        export CROSS_TARGET_TRIPLE="aarch64-linux-android"
        export CROSS_ANDROID_API_LEVEL="24"
        ;;
    i686)
        export CROSS_HOST_TRIPLE="i686-linux-android"
        export CROSS_TARGET_TRIPLE="i686-linux-android"
        export CROSS_ANDROID_API_LEVEL="24"
        ;;
    arm|armv7a)
        export CROSS_HOST_TRIPLE="armv7a-linux-androideabi"
        export CROSS_TARGET_TRIPLE="arm-linux-androideabi"
        export CROSS_ANDROID_API_LEVEL="24"
        ;;
esac

# 动态库扩展
export CROSS_SHARED_LIB_EXT=".so"
export CROSS_STATIC_LIB_EXT=".a"
export CROSS_EXECUTABLE_EXT=""

# NDK 补丁路径（仅 Android 使用）
export CROSS_NDK_PATCHES_DIR="${CROSS_SCRIPTDIR}/ndk-patches"

# 交叉编译 glibc 工具链脚本（仅 Android 需要）
export CROSS_CGCT_DIR="${CROSS_SCRIPTDIR}/scripts-cgct"

# 包依赖命名约定
export CROSS_DEP_SUFFIX="-glibc"
export CROSS_DEP_GLIBC_NAME="glibc"

# CFLAGS / LDFLAGS 默认值
export CROSS_DEFAULT_CFLAGS="-O2 -fPIC"
export CROSS_DEFAULT_LDFLAGS="-Wl,-rpath,${CROSS_PREFIX}/lib"

# 构建标志
export CROSS_CROSS_COMPILING=true  # Android 始终交叉编译（或在 Termux 本机编译）
export CROSS_NEEDS_NDK_PATCHES=true
export CROSS_NEEDS_CGCT=true

# shellcheck disable=SC2034
CROSS_PLATFORM_READY=true
