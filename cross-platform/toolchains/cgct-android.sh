#!/bin/bash
# =============================================================================
# Toolchain: CGCT (Cross Gnu C Toolchain) for Android
# =============================================================================
# Android 交叉编译 glibc 工具链。这是 glibc-packages 项目的核心工具链。
#
# CGCT = 从 termux-packages 继承的交叉编译工具链，支持:
#   - binutils 构建
#   - gcc 构建 (启用 glibc 支持)
#   - Android API level 24+
#   - 支持架构: aarch64, arm, i686, x86_64
# =============================================================================

echo "[toolchain] 配置 CGCT Android 交叉工具链"

# CGCT 工具链安装路径（可通过环境变量覆盖）
: "${CROSS_CGCT_PREFIX:=/data/data/com.termux/files/glibc/opt/cgct}"
: "${CROSS_CGCT_TARGETDIR:=${CROSS_CGCT_PREFIX}/target}"

# 根据架构设置工具链三元组
_CROSS_CGCT_TRIPLE=""
case "${CROSS_ARCH}" in
    aarch64)
        _CROSS_CGCT_TRIPLE="aarch64-linux-android"
        ;;
    arm)
        _CROSS_CGCT_TRIPLE="arm-linux-androideabi"
        ;;
    i686)
        _CROSS_CGCT_TRIPLE="i686-linux-android"
        ;;
    x86_64)
        _CROSS_CGCT_TRIPLE="x86_64-linux-android"
        ;;
    *)
        echo "[toolchain] 警告: 未知架构 ${CROSS_ARCH}，默认使用 aarch64" >&2
        _CROSS_CGCT_TRIPLE="aarch64-linux-android"
        export CROSS_ARCH="aarch64"
        ;;
esac

# Android API level
: "${CROSS_ANDROID_API_LEVEL:=24}"

# 编译器路径（带三元组前缀）
export CROSS_CC="${CROSS_CC:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-gcc}"
export CROSS_CXX="${CROSS_CXX:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-g++}"
export CROSS_CPP="${CROSS_CPP:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-cpp}"
export CROSS_LD="${CROSS_LD:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-ld}"
export CROSS_AR="${CROSS_AR:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-ar}"
export CROSS_RANLIB="${CROSS_RANLIB:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-ranlib}"
export CROSS_STRIP="${CROSS_STRIP:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-strip}"
export CROSS_NM="${CROSS_NM:-${CROSS_CGCT_PREFIX}/bin/${_CROSS_CGCT_TRIPLE}-nm}"

# 构建工具
export CROSS_MAKE="${CROSS_MAKE:-make}"
export CROSS_CMAKE="${CROSS_CMAKE:-cmake}"
export CROSS_NINJA="${CROSS_NINJA:-ninja}"

# CGCT 特定编译标志
export CROSS_CFLAGS="${CROSS_CFLAGS:--O2 -fPIC -D__ANDROID_API__=${CROSS_ANDROID_API_LEVEL}"
export CROSS_CXXFLAGS="${CROSS_CXXFLAGS}"
export CROSS_LDFLAGS="${CROSS_LDFLAGS}"

# sysroot (Android 目标文件系统
export CROSS_SYSROOT="${CROSS_CGCT_TARGETDIR}/${_CROSS_CGCT_TRIPLE}"

# Termux 兼容变量
export CC="${CROSS_CC}"
export CXX="${CROSS_CXX}"
export LD="${CROSS_LD}"
export AR="${CROSS_AR}"
export TERMUX_PREFIX="${CROSS_PREFIX}"
export TERMUX_ARCH="${CROSS_ARCH}"
export TERMUX_HOST_PLATFORM="${_CROSS_CGCT_TRIPLE}"

# 交叉编译标志（用于 autotools）
export CROSS_HOST="${_CROSS_CGCT_TRIPLE}"
export CROSS_BUILD="$(gcc -dumpmachine 2>/dev/null || echo x86_64-linux-gnu)"
export CROSS_CONFIGURE_ARGS="--host=${_CROSS_CGCT_TRIPLE} --build=${CROSS_BUILD}"
export CROSS_CMAKE_ARGS="${CROSS_CMAKE_ARGS} -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_NDK=${CROSS_CGCT_PREFIX}"

# 检测工具链是否存在
if [[ -x "${CROSS_CC}" ]]; then
    echo "[toolchain] CGCT GCC: $("${CROSS_CC}" --version | head -1)"
elif command -v "${CROSS_CC}" &>/dev/null; then
    echo "[toolchain] CGCT GCC: $("${CROSS_CC}" --version | head -1)"
else
    echo "[toolchain] 警告: 未找到 CGCT 工具链 (${CROSS_CC})" >&2
    echo "[toolchain] 请先运行脚本构建 CGCT:" >&2
    echo "  cd ${CROSS_SCRIPTDIR}/scripts-cgct && bash init.sh && bash build.sh" >&2
fi

# shellcheck disable=SC2034
CROSS_TOOLCHAIN_READY=true
