#!/bin/bash
# =============================================================================
# Platform: Browser / WebAssembly
# =============================================================================
# 浏览器平台配置。使用 Emscripten 将 C/C++ 包编译为 WebAssembly，
# 目标运行环境为 web 浏览器、Web Workers 和 Node.js。
#
# 关键特性:
#   - 编译目标: .wasm + .js (Emscripten 胶水代码)
#   - 无动态链接: 所有库静态链接为单个 .wasm
#   - 文件系统: Emscripten MEMFS (运行时) / OPFS (浏览器持久化)
#   - 网络: fetch API / WebSocket / WebRTC (浏览器限制)
#   - 多线程: Web Workers + SharedArrayBuffer (需 COOP/COEP 头)
#   - 打包: npm package (.tgz) 或纯静态文件 (.wasm + .js)
#   - 前缀: 虚拟路径 /usr/local (Emscripten 虚拟文件系统)
# =============================================================================

# 平台名称
export CROSS_PLATFORM_NAME="Browser / WebAssembly"
export CROSS_PLATFORM_OS="wasm"
export CROSS_PLATFORM_FAMILY="wasm"

# WASM 架构（仅支持 wasm32，wasm64 实验性）
case "${CROSS_ARCH}" in
    wasm32|wasm64|wasm)
        export CROSS_ARCH="wasm32"
        ;;
    *)
        echo "[browser-platform] 警告: 未知 WASM 架构 ${CROSS_ARCH}，使用 wasm32" >&2
        export CROSS_ARCH="wasm32"
        ;;
esac

# 平台特性标志
export CROSS_HAS_ANDROID_NDK=false
export CROSS_USES_CGCT=false
export CROSS_HAS_BIONIC_COMPAT=false
export CROSS_HAS_SYSTEMD=false
export CROSS_HAS_SELINUX=false
export CROSS_HAS_DYNAMIC_LINKING=false  # WASM 不支持动态链接
export CROSS_HAS_THREADS=true           # 支持 pthreads (需 SharedArrayBuffer)
export CROSS_HAS_FILESYSTEM=true        # Emscripten 虚拟文件系统
export CROSS_HAS_SIMD=false             # WASM SIMD 可选支持
export CROSS_HAS_EXCEPTIONS=false       # C++ 异常通过 -s DISABLE_EXCEPTION_CATCHING=0

# 包格式（WASM 产物打包为 tarball）
export CROSS_PACKAGE_FORMAT="tarball"
export CROSS_PACKAGE_EXT=".wasm.tar.gz"

# 默认前缀（Emscripten 虚拟文件系统路径）
export CROSS_PREFIX_DEFAULT="/usr/local"

# 架构映射
CROSS_HOST_TRIPLE="wasm32-unknown-emscripten"
export CROSS_HOST_TRIPLE
export CROSS_TARGET_TRIPLE="${CROSS_HOST_TRIPLE}"

# 库文件扩展名（WASM 全部静态链接，无 .so）
export CROSS_SHARED_LIB_EXT=".wasm"
export CROSS_STATIC_LIB_EXT=".a"
export CROSS_EXECUTABLE_EXT=".wasm"

# 包依赖命名约定
export CROSS_DEP_SUFFIX="-wasm"
export CROSS_DEP_GLIBC_NAME="wasi-libc"

# ---------------------------------------------------------------------------
# 默认 CFLAGS / LDFLAGS
# Emscripten 工具链会自动添加 -s WASM=1 等标志，
# 这里仅提供 WASM 基本优化选项
# ---------------------------------------------------------------------------
export CROSS_DEFAULT_CFLAGS="-O2 -fPIC -D__EMSCRIPTEN__=1 -DWASM_BROWSER=1"
export CROSS_DEFAULT_LDFLAGS=""

# ---------------------------------------------------------------------------
# 运行环境检测
# Emscripten SDK 位置
# ---------------------------------------------------------------------------
cross_browser_detect_runtime() {
    # 检测 Node.js（用于 WASM 模块测试）
    if command -v node &>/dev/null; then
        echo "[browser-platform] Node.js: $(node --version 2>/dev/null || 'unknown')"
        export CROSS_HAS_NODEJS=true
    else
        echo "[browser-platform] 注意: 未检测到 Node.js，无法运行 WASM 测试" >&2
        export CROSS_HAS_NODEJS=false
    fi

    # 检测 npm / yarn
    if command -v npm &>/dev/null; then
        echo "[browser-platform] npm: $(npm --version 2>/dev/null || 'unknown')"
        export CROSS_HAS_NPM=true
    else
        export CROSS_HAS_NPM=false
    fi
}

cross_browser_detect_runtime

# 构建标志
export CROSS_CROSS_COMPILING=true
export CROSS_NEEDS_NDK_PATCHES=false
export CROSS_NEEDS_CGCT=false

# WASM 特定变量
export CROSS_WASM_MEMORY_INITIAL="64MB"
export CROSS_WASM_MEMORY_MAXIMUM="512MB"
export CROSS_WASM_TOTAL_STACK="256KB"
export CROSS_WASM_USE_THREADS=true
export CROSS_WASM_USE_FILESYSTEM=true

# npm 包构建配置
export CROSS_NPM_SCOPE="@glibc-packages"
export CROSS_NPM_REGISTRY="https://npm.pkg.github.com"

# shellcheck disable=SC2034
CROSS_PLATFORM_READY=true
