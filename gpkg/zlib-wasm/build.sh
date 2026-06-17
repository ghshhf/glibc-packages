# zlib (WebAssembly port)
# Compiled with Emscripten emcc for browser/Node.js environments.
# Uses Emscripten's built-in zlib port (-s USE_ZLIB=1).

TERMUX_PKG_HOMEPAGE=https://zlib.net/
TERMUX_PKG_DESCRIPTION="Compression library implementing the deflate compression method — WebAssembly port"
TERMUX_PKG_LICENSE="ZLIB"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=1.3.2
TERMUX_PKG_SRCURL=https://github.com/madler/zlib/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=b99a0b86c0ba9360ec7e78c4f1e43b1cbdf1e6936c8fa0f6835c0cd694a495a1
TERMUX_PKG_DEPENDS=""
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_PLATFORM="browser"  # WASM target

# Emscripten 端口也可以使用内置端口: -s USE_ZLIB=1
# 但为了完整性和版本控制，我们直接从源码编译

termux_step_pre_configure() {
    # 确保使用 Emscripten 编译器
    export CC=emcc
    export CXX=em++
    export AR=emar
    export RANLIB=emranlib
    export CFLAGS="-O2 -fPIC -D__EMSCRIPTEN__=1 -DWASM_BROWSER=1"
    export LDFLAGS="\
        -s WASM=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s MAXIMUM_MEMORY=512MB \
        -s FILESYSTEM=0 \
        -s MODULARIZE=1 \
        -s EXPORT_NAME=\"createZlibModule\" \
        -s EXPORTED_RUNTIME_METHODS=['FS','getValue','setValue','stackAlloc','stackSave','stackRestore'] \
        -s EXPORTED_FUNCTIONS=['_malloc','_free','_deflate','_deflateInit_','_deflateEnd','_inflate','_inflateInit_','_inflateEnd','_compress','_uncompress','_compressBound','_crc32','_adler32'] \
        -s ERROR_ON_UNDEFINED_SYMBOLS=0"
}

termux_step_configure() {
    # zlib 使用自定义 configure 脚本
    ./configure \
        --prefix="${TERMUX_PREFIX}" \
        --static \
        --64
}

termux_step_make() {
    emmake make -j${TERMUX_PKG_MAKE_PROCESSES:-1}
}

termux_step_make_install() {
    # 安装头文件
    mkdir -p "${TERMUX_PREFIX}/include"
    cp zlib.h zconf.h "${TERMUX_PREFIX}/include/"

    # 安装 .a 静态库
    mkdir -p "${TERMUX_PREFIX}/lib"
    cp libz.a "${TERMUX_PREFIX}/lib/"

    # 重新链接生成 .wasm + .js
    echo "[WASM] 重新链接生成独立的 zlib.wasm + zlib.js"

    emcc -O2 \
        libz.a \
        -o "${TERMUX_PREFIX}/lib/zlib.js" \
        -s WASM=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s MAXIMUM_MEMORY=512MB \
        -s FILESYSTEM=0 \
        -s MODULARIZE=1 \
        -s EXPORT_NAME=\"createZlibModule\" \
        -s EXPORTED_RUNTIME_METHODS="['FS','getValue','setValue','stackAlloc','stackSave','stackRestore']" \
        -s EXPORTED_FUNCTIONS="['_malloc','_free','_deflate','_deflateInit_','_deflateEnd','_inflate','_inflateInit_','_inflateEnd','_compress','_uncompress','_compressBound','_crc32','_adler32']"

    if [[ -f "${TERMUX_PREFIX}/lib/zlib.js" ]]; then
        echo "[WASM] ✓ zlib.wasm + zlib.js 生成成功"
        ls -lh "${TERMUX_PREFIX}/lib/"*.wasm "${TERMUX_PREFIX}/lib/"*.js 2>/dev/null
    fi
}

termux_step_post_make_install() {
    # 输出产品清单
    echo ""
    echo "============================================"
    echo " zlib WebAssembly 包 v${TERMUX_PKG_VERSION}"
    echo "============================================"
    echo " 产物:"
    for f in "${TERMUX_PREFIX}/lib/"zlib.*; do
        if [[ -f "$f" ]]; then
            echo "   $(basename "$f") ($(ls -lh "$f" | awk '{print $5}'))"
        fi
    done
    echo ""
    echo " 导出的 C 函数:"
    echo "   _compress, _uncompress, _compressBound"
    echo "   _deflate, _deflateInit_, _deflateEnd"
    echo "   _inflate, _inflateInit_, _inflateEnd"
    echo "   _crc32, _adler32"
    echo ""
    echo " 浏览器使用:"
    echo "   import createZlibModule from './zlib.js'"
    echo "   const zlib = await createZlibModule()"
    echo "   const compressed = zlib._compress(data)"
    echo "============================================"
}
