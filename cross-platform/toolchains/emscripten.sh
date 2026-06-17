#!/bin/bash
# =============================================================================
# Toolchain: Emscripten (WebAssembly / Browser)
# =============================================================================
# Emscripten WebAssembly 编译器工具链。
# 将 C/C++ 代码编译为 .wasm 并在浏览器/Node.js/Worker 环境中运行。
#
# 依赖:
#   - Emscripten SDK (emsdk): https://emscripten.org/docs/getting_started/downloads.html
#   - 安装: git clone https://github.com/emscripten-core/emsdk.git && cd emsdk
#           ./emsdk install latest && ./emsdk activate latest
#           source ./emsdk_env.sh
#
# 用法:
#   ./build-cross.sh --platform browser --arch wasm32 --toolchain emscripten gpkg/zlib
#
# 输出:
#   - .wasm 文件 (WebAssembly 二进制)
#   - .js 文件 (Emscripten 胶水代码)
#   - .data 文件 (可选的文件系统数据)
# =============================================================================

echo "[toolchain] 配置 Emscripten WebAssembly 工具链"

# ---------------------------------------------------------------------------
# 1. Emscripten SDK 检测
# ---------------------------------------------------------------------------
_CROSS_EMSDK_CHECKED=false

cross_emscripten_detect() {
    # 如果已经检测过，跳过
    if [[ "${_CROSS_EMSDK_CHECKED}" == "true" ]]; then
        return 0
    fi

    # 尝试多种方式定位 emcc
    local _emcc_candidates=()

    # 1. 已加载到 PATH（最常见）
    if command -v emcc &>/dev/null; then
        _emcc_candidates+=("$(command -v emcc)")
    fi

    # 2. EMSDK 环境变量
    if [[ -n "${EMSDK:-}" ]] && [[ -f "${EMSDK}/upstream/emscripten/emcc" ]]; then
        _emcc_candidates+=("${EMSDK}/upstream/emscripten/emcc")
    fi

    # 3. 常见安装位置
    for _candidate in \
        "${HOME}/emsdk/upstream/emscripten/emcc" \
        "${HOME}/dev/emsdk/upstream/emscripten/emcc" \
        "/opt/emsdk/upstream/emscripten/emcc" \
        "/usr/lib/emscripten/emcc" \
        "/usr/local/opt/emscripten/bin/emcc"; do
        if [[ -f "${_candidate}" ]]; then
            _emcc_candidates+=("${_candidate}")
        fi
    done

    # 4. npm 全局安装
    if command -v npx &>/dev/null; then
        _npx_emcc=$(npx --prefix /tmp emcc --version 2>/dev/null || true)
        if [[ -n "${_npx_emcc}" ]]; then
            _emcc_candidates+=("emcc")
        fi
    fi

    # 选择第一个可用的
    if (( ${#_emcc_candidates[@]} > 0 )); then
        export EMSDK_EMCC="${_emcc_candidates[0]}"
        _CROSS_EMSDK_CHECKED=true
        echo "[toolchain] 找到 Emscripten: ${EMSDK_EMCC}"
        return 0
    fi

    echo "[toolchain] 警告: 未找到 emcc (Emscripten SDK)" >&2
    echo "[toolchain] 请安装: https://emscripten.org/docs/getting_started/downloads.html" >&2
    echo "[toolchain]   git clone https://github.com/emscripten-core/emsdk.git" >&2
    echo "[toolchain]   cd emsdk && ./emsdk install latest && ./emsdk activate latest" >&2
    echo "[toolchain]   source ./emsdk_env.sh" >&2
    return 1
}

# ---------------------------------------------------------------------------
# 2. 工具链变量
# ---------------------------------------------------------------------------

# 运行 Emscripten 命令（如果 SDK 环境已激活则直接运行，否则使用完整路径）
cross_emcc() {
    if command -v emcc &>/dev/null; then
        command emcc "$@"
    elif [[ -n "${EMSDK_EMCC:-}" ]]; then
        "${EMSDK_EMCC}" "$@"
    else
        echo "[toolchain] 错误: emcc 不可用" >&2
        return 1
    fi
}

cross_em++() {
    if command -v em++ &>/dev/null; then
        command em++ "$@"
    elif [[ -n "${EMSDK_EMCC:-}" ]]; then
        "${EMSDK_EMCC/emcc/em++}" "$@"
    else
        echo "[toolchain] 错误: em++ 不可用" >&2
        return 1
    fi
}

# 编译器
export CROSS_CC="${CROSS_CC:-emcc}"
export CROSS_CXX="${CROSS_CXX:-em++}"
export CROSS_CPP="${CROSS_CPP:-emcc -E}"
export CROSS_LD="${CROSS_LD:-emcc}"
export CROSS_AR="${CROSS_AR:-emar}"
export CROSS_AS="${CROSS_AS:-emas}"
export CROSS_RANLIB="${CROSS_RANLIB:-emranlib}"
export CROSS_STRIP="${CROSS_STRIP:-emstrip}"
export CROSS_NM="${CROSS_NM:-emnm}"

# 构建工具
export CROSS_MAKE="${CROSS_MAKE:-make}"
export CROSS_CMAKE="${CROSS_CMAKE:-emcmake cmake}"
export CROSS_NINJA="${CROSS_NINJA:-ninja}"
export CROSS_CONFIGURE="${CROSS_CONFIGURE:-emconfigure}"

# ---------------------------------------------------------------------------
# 3. 默认编译标志（WASM 优化）
# ---------------------------------------------------------------------------

# 基础优化标志
_WASM_BASE_CFLAGS="-O2 -fPIC"

# 浏览器目标标志（默认启用 WASM 线程和文件系统）
_WASM_BROWSER_FLAGS="\
    -s WASM=1 \
    -s SINGLE_FILE=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MAXIMUM_MEMORY=512MB \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s FILESYSTEM=1 \
    -s FORCE_FILESYSTEM=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME=\"createModule\" \
    -s EXPORT_ES6=0 \
    -s USE_ES6_IMPORT_META=0 \
    -s DEMANGLE_SUPPORT=1 \
    -s ASSERTIONS=0"

# 线程支持（需要 SharedArrayBuffer）
_WASM_THREAD_FLAGS="\
    -s USE_PTHREADS=1 \
    -s PTHREAD_POOL_SIZE=4 \
    -s PROXY_TO_PTHREAD=0"

# 文件系统支持
_WASM_FS_FLAGS="\
    -s FORCE_FILESYSTEM=1 \
    -s FILESYSTEM=1 \
    -s LZ4=1"

# 导出函数（方便从 JS 调用）
_WASM_EXPORT_FLAGS="\
    -s EXPORTED_RUNTIME_METHODS=['callMain','FS','PATH','ERRNO_CODES','TTY','DYNAMIC_CALLS','stackAlloc','stackSave','stackRestore','getValue','setValue'] \
    -s EXPORTED_FUNCTIONS=['_malloc','_free','_main']"

# 聚合所有标志
export CROSS_CFLAGS="${CROSS_CFLAGS:-${_WASM_BASE_CFLAGS} ${_WASM_BROWSER_FLAGS}}"
export CROSS_CXXFLAGS="${CROSS_CXXFLAGS:-${_WASM_BASE_CFLAGS} ${_WASM_BROWSER_FLAGS}}"
export CROSS_LDFLAGS="${CROSS_LDFLAGS:-${_WASM_BROWSER_FLAGS} ${_WASM_THREAD_FLAGS} ${_WASM_FS_FLAGS} ${_WASM_EXPORT_FLAGS}}"

# 库链接标志（按需启用常见库）
export CROSS_EM_LIBS=""

# 链接标准库的 Emscripten 端口
cross_link_em_port() {
    local port_name="$1"
    case "${port_name}" in
        sdl2)       echo "-s USE_SDL=2" ;;
        sdl2_image) echo "-s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"png\",\"jpg\"]'" ;;
        sdl2_mixer) echo "-s USE_SDL_MIXER=2" ;;
        sdl2_net)   echo "-s USE_SDL_NET=2" ;;
        sdl2_ttf)   echo "-s USE_SDL_TTF=2" ;;
        zlib)       echo "-s USE_ZLIB=1" ;;
        bzip2)      echo "-s USE_BZIP2=1" ;;
        libpng)     echo "-s USE_LIBPNG=1" ;;
        libjpeg)    echo "-s USE_LIBJPEG=1" ;;
        freetype)   echo "-s USE_FREETYPE=1" ;;
        harfbuzz)   echo "-s USE_HARFBUZZ=1" ;;
        ogg)        echo "-s USE_OGG=1" ;;
        vorbis)     echo "-s USE_VORBIS=1" ;;
        mpg123)     echo "-s USE_MPG123=1" ;;
        curl)       echo "-s USE_CURL=1" ;;
        regal)      echo "-s USE_REGAL=1" ;;
        webgl2)     echo "-s USE_WEBGL2=1 -lGL" ;;
        sqlite3)    echo "-s USE_SQLITE3=1" ;;
        *)          echo "" ;;
    esac
}

# 兼容 Termux 变量
export CC="${CROSS_CC}"
export CXX="${CROSS_CXX}"
export LD="${CROSS_LD}"
export AR="${CROSS_AR}"

# WASM 特殊变量
export CROSS_WASM_SYSTEM="browser"
export CROSS_WASM_ARCH="wasm32"

# ---------------------------------------------------------------------------
# 4. configure 参数（autotools / cmake / meson）
# ---------------------------------------------------------------------------
export CROSS_HOST_TRIPLE="wasm32-unknown-emscripten"
export CROSS_BUILD_TRIPLE="$(gcc -dumpmachine 2>/dev/null || echo x86_64-linux-gnu)"

# autotools 配置
export CROSS_CONFIGURE_ARGS="\
    --host=${CROSS_HOST_TRIPLE} \
    --build=${CROSS_BUILD_TRIPLE} \
    --disable-shared \
    --enable-static \
    --with-pic"

# CMake 配置
export CROSS_CMAKE_ARGS="\
    -DCMAKE_SYSTEM_NAME=Emscripten \
    -DCMAKE_SYSTEM_PROCESSOR=wasm32 \
    -DCMAKE_C_COMPILER=emcc \
    -DCMAKE_CXX_COMPILER=em++ \
    -DCMAKE_AR=emar \
    -DCMAKE_RANLIB=emranlib \
    -DCMAKE_FIND_ROOT_PATH=${CROSS_PREFIX} \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_STATIC_LIBS=ON"

# Meson 配置
export CROSS_MESON_ARGS="\
    --cross-file=${CROSS_FRAMEWORK_DIR}/cross-files/wasm32-emscripten.meson \
    2>/dev/null || true"

# WASM 打包后缀（无动态库）
export CROSS_SHARED_LIB_EXT=".wasm"
export CROSS_STATIC_LIB_EXT=".a"
export CROSS_EXECUTABLE_EXT=".wasm"

# ---------------------------------------------------------------------------
# 5. 工具链检测和验证
# ---------------------------------------------------------------------------
if cross_emscripten_detect; then
    echo "[toolchain] Emscripten: $(emcc --version 2>/dev/null | head -1 || ${EMSDK_EMCC} --version 2>/dev/null | head -1)"
    echo "[toolchain] 目标: wasm32-unknown-emscripten (浏览器/WebAssembly)"
    echo "[toolchain] 文件系统: 已启用 (MEMFS/OPFS)"
    echo "[toolchain] 线程: $(echo ${_WASM_THREAD_FLAGS} | grep -o 'USE_PTHREADS=[01]' | head -1)"
    echo "[toolchain] 内存: 动态增长 (最大 512MB)"
fi

# 清理内部变量
unset _WASM_BASE_CFLAGS _WASM_BROWSER_FLAGS _WASM_THREAD_FLAGS
unset _WASM_FS_FLAGS _WASM_EXPORT_FLAGS _CROSS_EMSDK_CHECKED

# shellcheck disable=SC2034
CROSS_TOOLCHAIN_READY=true
