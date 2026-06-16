#!/bin/bash
# =============================================================================
# Toolchain: GCC Native
# =============================================================================
# GNU GCC 本机工具链。适用于 Linux 本机、MSYS2 GCC 等环境。
# =============================================================================

echo "[toolchain] 配置 GCC 本机工具链"

# 编译器
export CROSS_CC="${CROSS_CC:-gcc}"
export CROSS_CXX="${CROSS_CXX:-g++}"
export CROSS_CPP="${CROSS_CPP:-cpp}"
export CROSS_LD="${CROSS_LD:-ld}"
export CROSS_AR="${CROSS_AR:-ar}"
export CROSS_AS="${CROSS_AS:-as}"
export CROSS_RANLIB="${CROSS_RANLIB:-ranlib}"
export CROSS_STRIP="${CROSS_STRIP:-strip}"
export CROSS_NM="${CROSS_NM:-nm}"

# 构建工具
export CROSS_MAKE="${CROSS_MAKE:-make}"
export CROSS_CMAKE="${CROSS_CMAKE:-cmake}"
export CROSS_NINJA="${CROSS_NINJA:-ninja}"

# 默认编译标志（如果平台未设置）
export CROSS_CFLAGS="${CROSS_CFLAGS:-${CROSS_DEFAULT_CFLAGS:-}}"
export CROSS_CXXFLAGS="${CROSS_CXXFLAGS:-${CROSS_DEFAULT_CFLAGS:-}}"
export CROSS_LDFLAGS="${CROSS_LDFLAGS:-${CROSS_DEFAULT_LDFLAGS:-}}"

# 兼容 Termux 变量
export CC="${CROSS_CC}"
export CXX="${CROSS_CXX}"
export LD="${CROSS_LD}"
export AR="${CROSS_AR}"

# 检测版本
if command -v "${CROSS_CC}" &>/dev/null; then
    echo "[toolchain] GCC: $("${CROSS_CC}" --version | head -1)"
else
    echo "[toolchain] 警告: 未找到 gcc，请确保已安装构建工具链" >&2
fi

# shellcheck disable=SC2034
CROSS_TOOLCHAIN_READY=true
