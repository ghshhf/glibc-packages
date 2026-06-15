#!/bin/bash
# =============================================================================
# Toolchain: Clang/LLVM Native
# =============================================================================
# Clang/LLVM 本机工具链。适用于 macOS、FreeBSD 等默认使用 Clang 的系统。
# =============================================================================

echo "[toolchain] 配置 Clang/LLVM 本机工具链"

# 编译器
export CROSS_CC="${CROSS_CC:-clang}"
export CROSS_CXX="${CROSS_CXX:-clang++}"
export CROSS_CPP="${CROSS_CPP:-clang -E}"
export CROSS_LD="${CROSS_LD:-ld.lld}"
export CROSS_AR="${CROSS_AR:-llvm-ar}"
export CROSS_AS="${CROSS_AS:-llvm-as}"
export CROSS_RANLIB="${CROSS_RANLIB:-llvm-ranlib}"
export CROSS_STRIP="${CROSS_STRIP:-llvm-strip}"
export CROSS_NM="${CROSS_NM:-llvm-nm}"

# 构建工具
export CROSS_MAKE="${CROSS_MAKE:-make}"
export CROSS_CMAKE="${CROSS_CMAKE:-cmake}"
export CROSS_NINJA="${CROSS_NINJA:-ninja}"

# 默认编译标志
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
    echo "[toolchain] Clang: $("${CROSS_CC}" --version | head -1)"
else
    echo "[toolchain] 警告: 未找到 clang，尝试回退到 gcc" >&2
    source "$(dirname "${BASH_SOURCE[0]}")/gcc-native.sh"
fi

# shellcheck disable=SC2034
CROSS_TOOLCHAIN_READY=true
