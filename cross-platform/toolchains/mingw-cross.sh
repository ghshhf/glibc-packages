#!/bin/bash
# =============================================================================
# Toolchain: MinGW-w64 Cross Compiler
# =============================================================================
# MinGW-w64 交叉编译工具链。用于在 Linux 上交叉编译 Windows 可执行文件。
# 需要安装: mingw-w64-x86-64-dev, mingw-w64-i686-dev (Debian/Ubuntu)
#          mingw-w64-gcc (Arch Linux)
# =============================================================================

echo "[toolchain] 配置 MinGW-w64 交叉工具链"

# 架构检测
_CROSS_MINGW_TRIPLE=""
case "${CROSS_ARCH}" in
    x86_64|amd64)
        _CROSS_MINGW_TRIPLE="x86_64-w64-mingw32"
        ;;
    i686|i386)
        _CROSS_MINGW_TRIPLE="i686-w64-mingw32"
        ;;
    *)
        echo "[toolchain] 警告: 不支持的 MinGW 架构 ${CROSS_ARCH}，使用 x86_64" >&2
        _CROSS_MINGW_TRIPLE="x86_64-w64-mingw32"
        export CROSS_ARCH="x86_64"
        ;;
esac

# 编译器（带三元组前缀）
export CROSS_CC="${CROSS_CC:-${_CROSS_MINGW_TRIPLE}-gcc}"
export CROSS_CXX="${CROSS_CXX:-${_CROSS_MINGW_TRIPLE}-g++}"
export CROSS_CPP="${CROSS_CPP:-${_CROSS_MINGW_TRIPLE}-cpp}"
export CROSS_LD="${CROSS_LD:-${_CROSS_MINGW_TRIPLE}-ld}"
export CROSS_AR="${CROSS_AR:-${_CROSS_MINGW_TRIPLE}-ar}"
export CROSS_RANLIB="${CROSS_RANLIB:-${_CROSS_MINGW_TRIPLE}-ranlib}"
export CROSS_STRIP="${CROSS_STRIP:-${_CROSS_MINGW_TRIPLE}-strip}"
export CROSS_NM="${CROSS_NM:-${_CROSS_MINGW_TRIPLE}-nm}"
export CROSS_DLLTOOL="${CROSS_DLLTOOL:-${_CROSS_MINGW_TRIPLE}-dlltool}"

# 构建工具
export CROSS_MAKE="${CROSS_MAKE:-make}"
export CROSS_CMAKE="${CROSS_CMAKE:-cmake}"
export CROSS_NINJA="${CROSS_NINJA:-ninja}"

# MinGW 特定编译标志
export CROSS_CFLAGS="${CROSS_CFLAGS:--O2 -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00}"
export CROSS_CXXFLAGS="${CROSS_CXXFLAGS:--O2 -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00}"
export CROSS_LDFLAGS="${CROSS_LDFLAGS:--Wl,--enable-auto-import}"

# 兼容 Termux 变量
export CC="${CROSS_CC}"
export CXX="${CROSS_CXX}"
export LD="${CROSS_LD}"
export AR="${CROSS_AR}"

# 交叉编译标志（用于 autotools）
export CROSS_HOST="${_CROSS_MINGW_TRIPLE}"
export CROSS_BUILD="$(gcc -dumpmachine 2>/dev/null || echo x86_64-linux-gnu)"

# configure 参数（autotools 包需要）
export CROSS_CONFIGURE_ARGS="--host=${_CROSS_MINGW_TRIPLE} --build=${CROSS_BUILD}"
export CROSS_CMAKE_ARGS="-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=${CROSS_CC} -DCMAKE_CXX_COMPILER=${CROSS_CXX}"

# 检测工具链
if command -v "${CROSS_CC}" &>/dev/null; then
    echo "[toolchain] MinGW GCC: $("${CROSS_CC}" --version | head -1)"
else
    echo "[toolchain] 警告: 未找到 ${_CROSS_MINGW_TRIPLE}-gcc" >&2
    echo "[toolchain] 请安装: mingw-w64 (Debian/Ubuntu) 或 mingw-w64-gcc (Arch Linux)" >&2
fi

# 清理
unset _CROSS_MINGW_TRIPLE

# shellcheck disable=SC2034
CROSS_TOOLCHAIN_READY=true
