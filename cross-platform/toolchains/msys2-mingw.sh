#!/bin/bash
# =============================================================================
# Toolchain: MSYS2 MinGW Native
# =============================================================================
# MSYS2 环境下的本机 MinGW-w64 工具链。
# 在 MSYS2 shell 中运行时，$MSYSTEM 决定使用哪个工具链：
#   MINGW64 - 64位 GCC, /mingw64/bin
#   MINGW32 - 32位 GCC, /mingw32/bin
#   CLANG64 - 64位 Clang/LLVM, /clang64/bin
#   UCRT64  - 64位 UCRT GCC, /ucrt64/bin
#   MSYS    - POSIX 兼容层 GCC, /usr/bin
# =============================================================================

echo "[toolchain] 配置 MSYS2 MinGW 本机工具链"

# 检测 MSYSTEM
_CROSS_MSYSTEM="${MSYSTEM:-MINGW64}"
echo "[toolchain] MSYSTEM=${_CROSS_MSYSTEM}"

case "${_CROSS_MSYSTEM}" in
    CLANG64|clang64)
        # Clang 工具链
        export CROSS_CC="${CROSS_CC:-clang}"
        export CROSS_CXX="${CROSS_CXX:-clang++}"
        export CROSS_LD="${CROSS_LD:-ld.lld}"
        export CROSS_AR="${CROSS_AR:-llvm-ar}"
        export CROSS_RANLIB="${CROSS_RANLIB:-llvm-ranlib}"
        export CROSS_STRIP="${CROSS_STRIP:-llvm-strip}"
        export CROSS_NM="${CROSS_NM:-llvm-nm}"
        ;;
    *)
        # GCC 工具链 (MINGW64/MINGW32/UCRT64)
        export CROSS_CC="${CROSS_CC:-gcc}"
        export CROSS_CXX="${CROSS_CXX:-g++}"
        export CROSS_CPP="${CROSS_CPP:-cpp}"
        export CROSS_LD="${CROSS_LD:-ld}"
        export CROSS_AR="${CROSS_AR:-ar}"
        export CROSS_RANLIB="${CROSS_RANLIB:-ranlib}"
        export CROSS_STRIP="${CROSS_STRIP:-strip}"
        export CROSS_NM="${CROSS_NM:-nm}"
        ;;
esac

# 构建工具（MSYS2 提供 pacman 包）
export CROSS_MAKE="${CROSS_MAKE:-make}"
export CROSS_CMAKE="${CROSS_CMAKE:-cmake}"
export CROSS_NINJA="${CROSS_NINJA:-ninja}"

# 编译标志
export CROSS_CFLAGS="${CROSS_CFLAGS:--O2 -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00}"
export CROSS_CXXFLAGS="${CROSS_CXXFLAGS:--O2 -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00}"
export CROSS_LDFLAGS="${CROSS_LDFLAGS:--Wl,--enable-auto-import}"

# 兼容 Termux 变量
export CC="${CROSS_CC}"
export CXX="${CROSS_CXX}"
export LD="${CROSS_LD}"
export AR="${CROSS_AR}"

# 检测
if command -v "${CROSS_CC}" &>/dev/null; then
    echo "[toolchain] 编译器: $("${CROSS_CC}" --version | head -1)"
else
    echo "[toolchain] 警告: 未找到 ${CROSS_CC}，请在 MSYS2 shell 中运行:" >&2
    echo "  pacman -S mingw-w64-x86_64-gcc" >&2
fi

# shellcheck disable=SC2034
CROSS_TOOLCHAIN_READY=true
