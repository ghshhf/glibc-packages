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
# 步骤 2: configure - 自动检测构建系统
# ---------------------------------------------------------------------------
cross_step_configure() {
    cd "${CROSS_SRCDIR}" || return 1

    # 优先使用包自定义的 configure hook
    if declare -f termux_step_configure >/dev/null 2>&1; then
        echo "  [步骤] configure (自定义)"
        termux_step_configure || return 1
        return 0
    fi

    # 自动检测
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
        cd "${CROSS_SRCDIR}" || return 1
        termux_step_make || return 1
        return 0
    fi

    # 检测 Rust/Cargo 项目
    if [[ -f "${CROSS_SRCDIR}/Cargo.toml" ]]; then
        echo "  [步骤] make (Cargo)"
        cd "${CROSS_SRCDIR}" || return 1
        cargo build --release ${CROSS_CARGO_ARGS:-} || return 1
        return 0
    fi

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
# 关键：对 Termux 风格的 termux_step_make_install，需要临时把 TERMUX_PREFIX
# 设为 ${CROSS_DESTDIR}${CROSS_PREFIX}，让 install 目标正确写入 dest 目录
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
        termux_step_make_install || return 1
        # 恢复
        export TERMUX_PREFIX="${CROSS_PREFIX}"
        unset DESTDIR
        return 0
    fi

    # 标准安装路径
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
# ---------------------------------------------------------------------------
cross_step_package() {
    local pkg_name="${CROSS_PKG_NAME:-package}"
    local pkg_version="${CROSS_PKG_VERSION:-1.0.0}"
    mkdir -p "${CROSS_OUTPUTDIR}"

    # 选择扩展名
    local _pkg_ext=".tar.gz"
    case "${CROSS_PACKAGE_FORMAT:-tar.gz}" in
        pacman|pkg.tar.zst)
            if command -v zstd >/dev/null 2>&1; then
                _pkg_ext=".pkg.tar.zst"
            fi
            ;;
        # debian/rpm 等需要专门工具；统一用 .tar.gz
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

echo "[cross-steps] framework loaded ✓"
