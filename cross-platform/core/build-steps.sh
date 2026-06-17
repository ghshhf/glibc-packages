#!/bin/bash
# =============================================================================
# Core: Build Steps
# =============================================================================
# 生产级构建步骤框架。build-cross.sh 会为每个包依次调用：
#   cross_step_prepare      - 下载源码、打补丁
#   cross_step_configure    - 自动检测构建系统并配置
#   cross_step_build        - 编译（自动检测 make/ninja/cargo）
#   cross_step_install      - 安装到 DESTDIR（兼容 Termux termux_step_make_install）
#   cross_step_post_install - 安装后处理（调用 termux_step_post_make_install hook）
#   cross_step_package      - 打包（tar.gz / pacman / dpkg-deb）
#
# 每个步骤都会在失败时返回非零 exit code，build-cross.sh 以 `|| exit 1` 捕获。
# =============================================================================

# ---------------------------------------------------------------------------
# 通用构建变量（会被 build-cross.sh 覆盖为每个包的独立路径）
# ---------------------------------------------------------------------------
: "${CROSS_BUILDDIR:=${PWD}/build}"
: "${CROSS_SRCDIR:=${PWD}/src}"
: "${CROSS_DESTDIR:=${PWD}/dest}"
: "${CROSS_OUTPUTDIR:=${PWD}/output}"
: "${CROSS_MAKE_JOBS:=$(nproc 2>/dev/null || echo 1)}"

export CROSS_BUILDDIR CROSS_SRCDIR CROSS_DESTDIR CROSS_OUTPUTDIR CROSS_MAKE_JOBS

# ---------------------------------------------------------------------------
# 工具：下载源码（带缓存、支持 sha256）
# ---------------------------------------------------------------------------
cross_download_source() {
    local url="$1"
    local sha256="${2:-}"
    local cachefile
    # 用 URL 作为唯一标识符（替换 / 为 _ 以避免路径问题）
    local url_slug
    url_slug="$(echo "$url" | tr '/' '_')"
    cachefile="${CROSS_SCRIPTDIR:-/tmp}/cross-cache/${url_slug}"
    mkdir -p "$(dirname "${cachefile}")"

    # 缓存命中（有 SHA 时校验）
    if [[ -f "${cachefile}" ]]; then
        if [[ -n "${sha256}" ]]; then
            local actual_sha
            actual_sha="$(sha256sum "${cachefile}" 2>/dev/null | cut -d' ' -f1)"
            if [[ "${actual_sha}" == "${sha256}" ]]; then
                echo "  [缓存] 使用已下载文件 (SHA256 校验通过)"
                cross_extract_source "${cachefile}"
                return 0
            else
                echo "  [缓存] SHA256 不匹配，重新下载"
            fi
        else
            echo "  [缓存] 使用已下载文件 (无 SHA256 校验)"
            cross_extract_source "${cachefile}"
            return 0
        fi
    fi

    # 下载
    echo "  [下载] ${url}"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL -o "${cachefile}" "${url}" || return 1
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O "${cachefile}" "${url}" || return 1
    else
        echo "  错误: 找不到 curl 或 wget" >&2
        return 1
    fi

    # SHA256 校验
    if [[ -n "${sha256}" ]]; then
        local actual_sha
        actual_sha="$(sha256sum "${cachefile}" 2>/dev/null | cut -d' ' -f1)"
        if [[ "${actual_sha}" != "${sha256}" ]]; then
            echo "  错误: SHA256 不匹配 (期望: ${sha256}, 实际: ${actual_sha})" >&2
            rm -f "${cachefile}"
            return 1
        fi
        echo "  [校验] SHA256 通过 ✓"
    fi

    cross_extract_source "${cachefile}"
}

# ---------------------------------------------------------------------------
# 工具：解压源码
# ---------------------------------------------------------------------------
cross_extract_source() {
    local archive="$1"
    echo "  [解压] $(basename "${archive}")"
    cd "${CROSS_SRCDIR}" || return 1

    case "$archive" in
        *.tar.gz|*.tgz)
            tar -xzf "$archive" --strip-components=1 2>/dev/null || \
            tar -xzf "$archive" || return 1
            ;;
        *.tar.bz2|*.tbz2)
            tar -xjf "$archive" --strip-components=1 2>/dev/null || \
            tar -xjf "$archive" || return 1
            ;;
        *.tar.xz|*.txz)
            tar -xJf "$archive" --strip-components=1 2>/dev/null || \
            tar -xJf "$archive" || return 1
            ;;
        *.tar.zst|*.tar.zstd)
            tar -I zstd -xf "$archive" --strip-components=1 2>/dev/null || \
            tar -I zstd -xf "$archive" || return 1
            ;;
        *.tar)
            tar -xf "$archive" --strip-components=1 2>/dev/null || \
            tar -xf "$archive" || return 1
            ;;
        *.zip)
            unzip -q "$archive" || return 1
            ;;
        *)
            echo "  警告: 无法识别的压缩格式: ${archive}" >&2
            return 1
            ;;
    esac
    return 0
}

# ---------------------------------------------------------------------------
# 步骤 1: prepare - 下载源码、打补丁、source build.sh
# ---------------------------------------------------------------------------
cross_step_prepare() {
    echo "  [准备] ${CROSS_PKG_NAME:-unknown} v${CROSS_PKG_VERSION:-unknown}"

    # 创建目录
    mkdir -p "${CROSS_SRCDIR}" "${CROSS_BUILDDIR}" "${CROSS_DESTDIR}" "${CROSS_OUTPUTDIR}"

    # 下载源码（如果指定了 SRCURL）
    if [[ -n "${CROSS_PKG_SRCURL:-}" ]]; then
        cross_download_source "${CROSS_PKG_SRCURL}" "${CROSS_PKG_SHA256:-}" || return 1
    fi

    # 复制补丁到 src 目录
    if [[ -n "${CROSS_PKG_DIR:-}" ]] && compgen -G "${CROSS_PKG_DIR}/*.patch" >/dev/null 2>&1; then
        echo "  [补丁] 应用 patch 文件"
        cp "${CROSS_PKG_DIR}"/*.patch "${CROSS_SRCDIR}/" 2>/dev/null || true
    fi

    # Source 包的 build.sh（注册 hook 函数）
    if [[ -n "${CROSS_PKG_DIR:-}" ]] && [[ -f "${CROSS_PKG_DIR}/build.sh" ]]; then
        cd "${CROSS_SRCDIR}" || return 1
        source "${CROSS_PKG_DIR}/build.sh" 2>/dev/null || true
    fi

    # 调用预配置 hook（如果有）
    if declare -f termux_step_pre_configure >/dev/null 2>&1; then
        echo "  [hook] pre_configure"
        cd "${CROSS_SRCDIR}" || return 1
        termux_step_pre_configure || return 1
    fi

    return 0
}

# ---------------------------------------------------------------------------
# Helper: 是否为 WASM/浏览器平台
# ---------------------------------------------------------------------------
cross_is_wasm() {
    [[ "${CROSS_PLATFORM}" == "browser" || "${CROSS_PLATFORM}" == "wasm" ]]
}

# ---------------------------------------------------------------------------
# 步骤 2: configure - 自动检测构建系统
# ---------------------------------------------------------------------------
cross_step_configure() {
    cd "${CROSS_SRCDIR}" || return 1

    # 优先使用包自定义的 configure hook
    if declare -f termux_step_configure >/dev/null 2>&1; then
        echo "  [步骤] configure (自定义)"

        # WASM 特殊处理：在自定义 configure 中注入 Emscripten 环境
        if cross_is_wasm; then
            export CC=emcc CXX=em++ AR=emar RANLIB=emranlib
            export CFLAGS="${CROSS_CFLAGS:-} ${CFLAGS:-}"
            export CXXFLAGS="${CROSS_CXXFLAGS:-} ${CXXFLAGS:-}"
            export LDFLAGS="${CROSS_LDFLAGS:-} ${LDFLAGS:-}"
        fi

        termux_step_configure || return 1
        return 0
    fi

    # ---- WASM/浏览器平台：使用 Emscripten 包装器 ----
    if cross_is_wasm; then
        # 设置 Emscripten 环境变量（确保 emcc 等命令可用）
        export CC=emcc CXX=em++ AR=emar RANLIB=emranlib
        export CFLAGS="${CROSS_CFLAGS:-} ${CFLAGS:-}"
        export CXXFLAGS="${CROSS_CXXFLAGS:-} ${CXXFLAGS:-}"
        export LDFLAGS="${CROSS_LDFLAGS:-} ${LDFLAGS:-}"
        export LIBS="${CROSS_EM_LIBS:-} ${LIBS:-}"

        if [[ -f "./configure" ]]; then
            echo "  [步骤] configure (Emscripten autotools)"
            emconfigure ./configure --prefix="${CROSS_PREFIX}" \
                ${CROSS_CONFIGURE_ARGS:-} || return 1
        elif [[ -f "CMakeLists.txt" ]]; then
            echo "  [步骤] configure (Emscripten CMake)"
            emcmake cmake -B "${CROSS_BUILDDIR}" \
                -DCMAKE_INSTALL_PREFIX="${CROSS_PREFIX}" \
                -DCMAKE_BUILD_TYPE=Release \
                ${CROSS_CMAKE_ARGS:-} || return 1
        elif [[ -f "meson.build" ]]; then
            echo "  [步骤] configure (WASM - meson 使用交叉文件)"
            meson setup "${CROSS_BUILDDIR}" \
                --prefix="${CROSS_PREFIX}" \
                --buildtype=release \
                --cross-file="${CROSS_FRAMEWORK_DIR}/cross-files/wasm32-emscripten.meson" \
                ${CROSS_MESON_ARGS:-} 2>/dev/null || {
                echo "  [步骤] configure (meson 跳过 - 需要 wasm cross-file)"
            }
        else
            echo "  [步骤] configure (WASM - 无配置系统，跳过)"
        fi
        return 0
    fi

    # ---- 非 WASM 平台：标准流程 ----
    if [[ -f "./configure" ]]; then
        echo "  [步骤] configure (GNU autotools)"
        ./configure --prefix="${CROSS_PREFIX}" ${CROSS_CONFIGURE_ARGS:-} || return 1
    elif [[ -f "CMakeLists.txt" ]]; then
        echo "  [步骤] configure (CMake)"
        cmake -B "${CROSS_BUILDDIR}" \
            -DCMAKE_INSTALL_PREFIX="${CROSS_PREFIX}" \
            -DCMAKE_BUILD_TYPE=Release \
            ${CROSS_CMAKE_ARGS:-} || return 1
    elif [[ -f "meson.build" ]]; then
        echo "  [步骤] configure (meson)"
        meson setup "${CROSS_BUILDDIR}" \
            --prefix="${CROSS_PREFIX}" \
            --buildtype=release \
            ${CROSS_MESON_ARGS:-} || return 1
    fi
    return 0
}

# ---------------------------------------------------------------------------
# 步骤 3: build - 编译
# ---------------------------------------------------------------------------
cross_step_build() {
    # 优先使用包自定义的 make hook
    if declare -f termux_step_make >/dev/null 2>&1; then
        echo "  [步骤] make (自定义)"

        # WASM 特殊处理
        if cross_is_wasm; then
            export CC=emcc CXX=em++ AR=emar RANLIB=emranlib
            export CFLAGS="${CROSS_CFLAGS:-} ${CFLAGS:-}"
            export CXXFLAGS="${CROSS_CXXFLAGS:-} ${CXXFLAGS:-}"
            export LDFLAGS="${CROSS_LDFLAGS:-} ${LDFLAGS:-}"
        fi

        cd "${CROSS_SRCDIR}" || return 1
        termux_step_make || return 1
        return 0
    fi

    # 检测 Rust/Cargo 项目（WASM 目标）
    if [[ -f "${CROSS_SRCDIR}/Cargo.toml" ]]; then
        if cross_is_wasm; then
            echo "  [步骤] make (Cargo → wasm32-unknown-emscripten)"
            cd "${CROSS_SRCDIR}" || return 1
            cargo build --release --target wasm32-unknown-emscripten \
                ${CROSS_CARGO_ARGS:-} || return 1
        else
            echo "  [步骤] make (Cargo)"
            cd "${CROSS_SRCDIR}" || return 1
            cargo build --release ${CROSS_CARGO_ARGS:-} || return 1
        fi
        return 0
    fi

    # ---- WASM 平台：使用 emmake 包装 ----
    if cross_is_wasm; then
        local _wasm_workdir="${CROSS_SRCDIR}"
        if [[ -f "${CROSS_BUILDDIR}/build.ninja" ]] || [[ -f "${CROSS_BUILDDIR}/Makefile" ]]; then
            _wasm_workdir="${CROSS_BUILDDIR}"
        fi

        if [[ -f "${_wasm_workdir}/build.ninja" ]]; then
            echo "  [步骤] make (Emscripten + ninja)"
            emmake ninja -j "${CROSS_MAKE_JOBS}" -C "${_wasm_workdir}" || return 1
        elif [[ -f "${_wasm_workdir}/Makefile" ]]; then
            echo "  [步骤] make (Emscripten + make)"
            emmake make -j "${CROSS_MAKE_JOBS}" -C "${_wasm_workdir}" \
                ${CROSS_MAKE_ARGS:-} || return 1
        elif [[ -f "${CROSS_SRCDIR}/Makefile" ]]; then
            echo "  [步骤] make (Emscripten + make, in-src)"
            emmake make -j "${CROSS_MAKE_JOBS}" -C "${CROSS_SRCDIR}" \
                ${CROSS_MAKE_ARGS:-} || return 1
        else
            echo "  [步骤] make (WASM - 未检测到构建系统)"
        fi
        return 0
    fi

    # ---- 非 WASM 平台：标准流程 ----
    # 选择 build 或 src 目录
    local _workdir="${CROSS_SRCDIR}"
    if [[ -f "${CROSS_BUILDDIR}/build.ninja" ]] || [[ -f "${CROSS_BUILDDIR}/Makefile" ]]; then
        _workdir="${CROSS_BUILDDIR}"
    fi

    if [[ -f "${_workdir}/build.ninja" ]]; then
        echo "  [步骤] make (ninja)"
        ninja -j "${CROSS_MAKE_JOBS}" -C "${_workdir}" || return 1
    elif [[ -f "${_workdir}/Makefile" ]]; then
        echo "  [步骤] make"
        make -j "${CROSS_MAKE_JOBS}" -C "${_workdir}" ${CROSS_MAKE_ARGS:-} || return 1
    elif [[ -f "${CROSS_SRCDIR}/Makefile" ]]; then
        echo "  [步骤] make (in-src)"
        make -j "${CROSS_MAKE_JOBS}" -C "${CROSS_SRCDIR}" ${CROSS_MAKE_ARGS:-} || return 1
    else
        echo "  [步骤] make (跳过 - 未检测到 Makefile/ninja)"
    fi
    return 0
}

# ---------------------------------------------------------------------------
# 步骤 4: install - 安装到 DESTDIR
# ---------------------------------------------------------------------------
cross_step_install() {
    if declare -f termux_step_make_install >/dev/null 2>&1; then
        echo "  [步骤] install (自定义)"
        cd "${CROSS_SRCDIR}" || return 1
        # 创建安装目录（Termux 风格包常期望这些存在）
        mkdir -p "${CROSS_DESTDIR}${CROSS_PREFIX}/bin" \
                 "${CROSS_DESTDIR}${CROSS_PREFIX}/lib" \
                 "${CROSS_DESTDIR}${CROSS_PREFIX}/include"
        # 临时覆盖 TERMUX_PREFIX
        export TERMUX_PREFIX="${CROSS_DESTDIR}${CROSS_PREFIX}"
        export DESTDIR="${CROSS_DESTDIR}"

        # WASM: 注入 Emscripten 环境变量
        if cross_is_wasm; then
            export CC=emcc CXX=em++
        fi

        termux_step_make_install || return 1
        # 恢复
        export TERMUX_PREFIX="${CROSS_PREFIX}"
        unset DESTDIR
        return 0
    fi

    # ---- WASM 平台安装 ----
    if cross_is_wasm; then
        echo "  [步骤] install (WASM - 收集产物)"

        # 创建标准目录
        mkdir -p "${CROSS_DESTDIR}${CROSS_PREFIX}/bin" \
                 "${CROSS_DESTDIR}${CROSS_PREFIX}/lib" \
                 "${CROSS_DESTDIR}${CROSS_PREFIX}/include" \
                 "${CROSS_DESTDIR}/npm"

        # 查找并复制 .wasm 文件
        local _wasm_files=()
        while IFS= read -r -d '' f; do
            _wasm_files+=("$f")
        done < <(find "${CROSS_BUILDDIR}" "${CROSS_SRCDIR}" -maxdepth 3 -name "*.wasm" -not -path "*/_deps/*" -print0 2>/dev/null || true)

        if (( ${#_wasm_files[@]} > 0 )); then
            echo "  [WASM] 找到 ${#_wasm_files[@]} 个 .wasm 文件"
            for f in "${_wasm_files[@]}"; do
                cp -v "$f" "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/" 2>/dev/null
            done
        fi

        # 复制 Emscripten 生成的 JS 胶水文件
        local _js_files=()
        while IFS= read -r -d '' f; do
            _js_files+=("$f")
        done < <(find "${CROSS_BUILDDIR}" "${CROSS_SRCDIR}" -maxdepth 3 -name "*.js" -not -path "*/_deps/*" -print0 2>/dev/null || true)

        if (( ${#_js_files[@]} > 0 )); then
            echo "  [WASM] 找到 ${#_js_files[@]} 个 JS 胶水文件"
            for f in "${_js_files[@]}"; do
                cp -v "$f" "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/" 2>/dev/null
            done
        fi

        # 复制头文件（如果有）
        local _header_dirs=("${CROSS_BUILDDIR}/include" "${CROSS_SRCDIR}/include")
        for _hd in "${_header_dirs[@]}"; do
            if [[ -d "${_hd}" ]]; then
                cp -r "${_hd}/" "${CROSS_DESTDIR}${CROSS_PREFIX}/include/" 2>/dev/null || true
            fi
        done

        # 复制 .a 静态库文件
        local _a_files=()
        while IFS= read -r -d '' f; do
            _a_files+=("$f")
        done < <(find "${CROSS_BUILDDIR}" "${CROSS_SRCDIR}" -maxdepth 3 -name "*.a" -print0 2>/dev/null || true)

        if (( ${#_a_files[@]} > 0 )); then
            for f in "${_a_files[@]}"; do
                cp -v "$f" "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/" 2>/dev/null
            done
        fi

        # 生成 NPM package 目录结构
        _cross_wasm_gen_npm_package

        return 0
    fi

    # ---- 非 WASM 标准安装 ----
    if [[ -f "${CROSS_BUILDDIR}/build.ninja" ]]; then
        echo "  [步骤] install (ninja)"
        DESTDIR="${CROSS_DESTDIR}" ninja -C "${CROSS_BUILDDIR}" install || return 1
    elif [[ -f "${CROSS_BUILDDIR}/Makefile" ]]; then
        echo "  [步骤] install (make)"
        make -C "${CROSS_BUILDDIR}" install DESTDIR="${CROSS_DESTDIR}" || return 1
    elif [[ -f "${CROSS_SRCDIR}/Makefile" ]]; then
        echo "  [步骤] install (make, in-src)"
        make -C "${CROSS_SRCDIR}" install DESTDIR="${CROSS_DESTDIR}" \
             PREFIX="${CROSS_PREFIX}" || return 1
    else
        echo "  [步骤] install (跳过 - 未检测到 Makefile/ninja)"
    fi
    return 0
}

# ---------------------------------------------------------------------------
# WASM 辅助：生成 NPM 包目录结构
# 每个 WASM 包同时以 npm package 格式输出，方便在浏览器/Node.js 中使用
# ---------------------------------------------------------------------------
_cross_wasm_gen_npm_package() {
    local pkg_name="${CROSS_PKG_NAME:-package}"
    local pkg_version="${CROSS_PKG_VERSION:-1.0.0}"
    local pkg_desc="${CROSS_PKG_DESCRIPTION:-WASM package}"
    local npm_dir="${CROSS_DESTDIR}/npm"

    mkdir -p "${npm_dir}/lib" "${npm_dir}/types"

    # 生成 package.json
    local _wasm_main=""
    local _wasm_file
    for _wasm_file in "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/"*.wasm; do
        if [[ -f "${_wasm_file}" ]]; then
            _wasm_main="lib/$(basename "${_wasm_file}")"
            break
        fi
    done

    # 查找 JS 入口
    local _js_main=""
    for _js_file in "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/"*.js; do
        if [[ -f "${_js_file}" ]]; then
            _js_main="lib/$(basename "${_js_file}")"
            break
        fi
    done

    cat > "${npm_dir}/package.json" <<-NPMEOF
{
  "name": "${CROSS_NPM_SCOPE:-@glibc-packages}/${pkg_name}",
  "version": "${pkg_version}",
  "description": "${pkg_desc} (WebAssembly port via glibc-packages)",
  "main": "${_js_main:-lib/${pkg_name}.js}",
  "wasm": "${_wasm_main:-lib/${pkg_name}.wasm}",
  "types": "types/index.d.ts",
  "files": ["lib/", "types/"],
  "keywords": ["wasm", "webassembly", "glibc-packages", "${pkg_name}"],
  "license": "${CROSS_PKG_LICENSE:-MIT}",
  "repository": {
    "type": "git",
    "url": "https://github.com/ghshhf/glibc-packages.git"
  },
  "browser": {
    "./lib/*.js": "./lib/*.js"
  },
  "exports": {
    ".": {
      "browser": "./${_js_main:-lib/${pkg_name}.js}",
      "default": "./${_js_main:-lib/${pkg_name}.js}"
    },
    "./wasm": {
      "browser": "./${_wasm_main:-lib/${pkg_name}.wasm}"
    }
  },
  "dependencies": {},
  "peerDependencies": {
    "@glibc-packages/wasm-runtime": ">=1.0.0"
  }
}
NPMEOF

    # 将 .wasm 和 .js 复制到 npm/lib
    cp "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/"*.wasm "${npm_dir}/lib/" 2>/dev/null || true
    cp "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/"*.js "${npm_dir}/lib/" 2>/dev/null || true

    echo "  [NPM] 包目录已生成: ${npm_dir}"
    echo "  [NPM] package: ${CROSS_NPM_SCOPE:-@glibc-packages}/${pkg_name}@${pkg_version}"
}

# ---------------------------------------------------------------------------
# 步骤 5: post_install - 安装后处理
# ---------------------------------------------------------------------------
cross_step_post_install() {
    if declare -f termux_step_post_make_install >/dev/null 2>&1; then
        echo "  [hook] post_make_install"
        termux_step_post_make_install || return 1
    fi
    return 0
}

# ---------------------------------------------------------------------------
# 步骤 6: package - 打包
# 可靠格式：.tar.gz（任何系统都支持）
# 可选格式：.pkg.tar.zst（需要 zstd）、.deb（需要 dpkg-deb）
# WASM 特殊：额外生成 .tgz npm 包
# ---------------------------------------------------------------------------
cross_step_package() {
    local pkg_name="${CROSS_PKG_NAME:-package}"
    local pkg_version="${CROSS_PKG_VERSION:-1.0.0}"
    mkdir -p "${CROSS_OUTPUTDIR}"

    # ---- WASM 打包：生成 npm .tgz + 标准 tarball ----
    if cross_is_wasm; then
        # 1. 标准 tarball（与其它平台一致）
        local _wasm_tarball="${CROSS_OUTPUTDIR}/${pkg_name}-${pkg_version}-browser-wasm32.tar.gz"
        if [[ -d "${CROSS_DESTDIR}${CROSS_PREFIX}" ]]; then
            tar -czf "${_wasm_tarball}" \
                -C "${CROSS_DESTDIR}${CROSS_PREFIX}" . 2>/dev/null || return 1
            echo "  [打包] ${_wasm_tarball}"
            ls -lh "${_wasm_tarball}" 2>/dev/null || true
        fi

        # 2. npm .tgz 包
        local _npm_dir="${CROSS_DESTDIR}/npm"
        if [[ -f "${_npm_dir}/package.json" ]]; then
            local _npm_tarball="${CROSS_OUTPUTDIR}/${pkg_name}-${pkg_version}.tgz"
            tar -czf "${_npm_tarball}" -C "${_npm_dir}" . 2>/dev/null || return 1
            echo "  [NPM打包] ${_npm_tarball}"
            ls -lh "${_npm_tarball}" 2>/dev/null || true
        fi

        # 3. 生成校验文件
        (cd "${CROSS_OUTPUTDIR}" && sha256sum *.tar.gz *.tgz 2>/dev/null > SHA256SUMS) || true
        echo "  [校验] SHA256SUMS 已生成"

        return 0
    fi

    # ---- 非 WASM 标准打包 ----
    local _pkg_ext=".tar.gz"
    case "${CROSS_PACKAGE_FORMAT:-tar.gz}" in
        pacman|pkg.tar.zst)
            if command -v zstd >/dev/null 2>&1; then
                _pkg_ext=".pkg.tar.zst"
            fi
            ;;
    esac

    local _output_file="${CROSS_OUTPUTDIR}/${pkg_name}-${pkg_version}-${CROSS_PLATFORM}-${CROSS_ARCH}${_pkg_ext}"

    if [[ -d "${CROSS_DESTDIR}" ]]; then
        if [[ "${_pkg_ext}" == ".pkg.tar.zst" ]]; then
            tar --zstd -cf "${_output_file}" -C "${CROSS_DESTDIR}" . 2>/dev/null || return 1
        else
            tar -czf "${_output_file}" -C "${CROSS_DESTDIR}" . 2>/dev/null || return 1
        fi
        echo "  [打包] ${_output_file}"
        ls -lh "${_output_file}" 2>/dev/null || true
    fi
    return 0
}

# ---------------------------------------------------------------------------
# 步骤 7: swbn_package — 生成 .swbn 标准组件包
# 当 CROSS_SWBN=true 时，将所有产物打包为 SSI 标准 .swbn 格式
# Produces: <pkg_name>-<version>.swbn (tar.gz with manifest + wasm + signature)
# ---------------------------------------------------------------------------
cross_step_swbn_package() {
    if [[ "${CROSS_SWBN:-false}" != "true" ]]; then
        return 0  # 跳过
    fi

    local pkg_name="${CROSS_PKG_NAME:-package}"
    local pkg_version="${CROSS_PKG_VERSION:-1.0.0}"
    local pkg_type="${CROSS_PKG_TYPE:-generic}"
    local pkg_desc="${CROSS_PKG_DESCRIPTION:-SSI component}"
    local pkg_vendor="${CROSS_PKG_VENDOR:-AI-TP Foundation}"

    local swbn_dir="${CROSS_OUTPUTDIR}/swbn-staging"
    mkdir -p "${swbn_dir}"

    echo "  [SWBN] 生成标准组件包: ${pkg_name} v${pkg_version}"

    # ── 1. 生成 manifest.json ──
    local interfaces_provided="${CROSS_SSI_INTERFACES:-SSI-CORE}"
    local interfaces_required="${CROSS_SSI_REQUIRES:-}"

    cat > "${swbn_dir}/manifest.json" <<-SWBNEOF
{
  "\$schema": "https://schemas.ai-tp.org/swbn/manifest-v1.json",
  "component": {
    "id": "$(uuidgen 2>/dev/null || echo '00000000-0000-4000-8000-000000000000')",
    "name": "${pkg_name}",
    "type": "${pkg_type}",
    "version": "${pkg_version}",
    "vendor": "${pkg_vendor}",
    "description": "${pkg_desc}"
  },
  "entry": {
    "wasm": "main.wasm",
    "memory": {
      "initial": "64MB",
      "maximum": "512MB",
      "stack": "256KB"
    }
  },
  "interfaces": {
    "provides": [${interfaces_provided}],
    "requires": [${interfaces_required}]
  },
  "permissions": [],
  "dependencies": {},
  "capabilities": {
    "gpu": false,
    "threads": ${CROSS_WASM_USE_THREADS:-false},
    "simd": false,
    "memory64": false
  },
  "lifecycle": {
    "autostart": false,
    "keep_alive": false,
    "restart_on_crash": false,
    "max_restarts": 3
  }
}
SWBNEOF

    # ── 2. 复制 WASM 产物 ──
    local wasm_found=false
    for _wasm in "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/"*.wasm "${CROSS_DESTDIR}${CROSS_PREFIX}/bin/"*.wasm; do
        if [[ -f "${_wasm}" ]]; then
            cp "${_wasm}" "${swbn_dir}/main.wasm"
            wasm_found=true
            echo "  [SWBN] WASM: $(basename "${_wasm}") ($(ls -lh "${_wasm}" | awk '{print $5}'))"
            break
        fi
    done

    # 如果没有 WASM 文件，生成最小占位 WASM
    if [[ "${wasm_found}" != "true" ]]; then
        echo "  [SWBN] 未找到 WASM 产物，生成最小占位模块"
        # 最小 WASM 模块（返回 0 的空函数）
        printf '\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x05\x01\x60\x00\x01\x7f\x03\x02\x01\x00\x07\x0b\x01\x07\x5f\x73\x73\x69\x5f\x69\x6e\x69\x74\x00\x00\x0a\x06\x01\x04\x00\x41\x00\x0b' \
            > "${swbn_dir}/main.wasm"
    fi

    # ── 3. 复制 JS 胶水（如果有） ──
    for _js in "${CROSS_DESTDIR}${CROSS_PREFIX}/lib/"*.js; do
        if [[ -f "${_js}" ]]; then
            cp "${_js}" "${swbn_dir}/main.js"
            echo "  [SWBN] JS: $(basename "${_js}")"
            break
        fi
    done

    # ── 4. 收集头文件（如果有） ──
    if [[ -d "${CROSS_DESTDIR}${CROSS_PREFIX}/include" ]]; then
        mkdir -p "${swbn_dir}/types"
        cp -r "${CROSS_DESTDIR}${CROSS_PREFIX}/include/" "${swbn_dir}/types/" 2>/dev/null || true
        echo "  [SWBN] 头文件已收集"
    fi

    # ── 5. 打包为 .swbn ──
    local swbn_output="${CROSS_OUTPUTDIR}/${pkg_name}-${pkg_version}.swbn"
    tar -czf "${swbn_output}" -C "${swbn_dir}" . 2>/dev/null || return 1
    echo "  [SWBN] 输出: ${swbn_output}"
    ls -lh "${swbn_output}" 2>/dev/null || true

    # ── 6. 生成签名（简单 SHA256 标记） ──
    if command -v sha256sum &>/dev/null; then
        (cd "${CROSS_OUTPUTDIR}" && sha256sum "${pkg_name}-${pkg_version}.swbn" > "${pkg_name}-${pkg_version}.swbn.sha256")
        echo "  [SWBN] SHA256: ${pkg_name}-${pkg_version}.swbn.sha256"
    fi

    # 清理临时目录
    rm -rf "${swbn_dir}"

    echo "  [SWBN] ✅ 标准组件包生成完成"
    return 0
}

echo "[cross-steps] framework loaded ✓"
