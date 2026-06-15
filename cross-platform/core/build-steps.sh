#!/bin/bash
# =============================================================================
# Core: Build Steps
# =============================================================================
# 统一的构建步骤框架。每个包的 build.sh 会调用这些步骤。
#
# 标准构建流程:
#   1. cross_step_prepare        - 准备构建环境（下载源码、解压、打补丁）
#   2. cross_step_configure      - 配置（autotools: ./configure, CMake: cmake）
#   3. cross_step_build          - 编译（make / ninja / cargo build）
#   4. cross_step_install        - 安装到 DESTDIR
#   5. cross_step_post_install   - 安装后处理（创建符号链接等）
#   6. cross_step_package        - 打包（pacman / deb / tar）
#
# 包 build.sh 可以覆盖任何步骤。
# =============================================================================

# ---------------------------------------------------------------------------
# 通用构建变量
# ---------------------------------------------------------------------------
: "${CROSS_BUILDDIR:=${PWD}/build}"
: "${CROSS_SRCDIR:=${PWD}/src}"
: "${CROSS_DESTDIR:=${PWD}/dest}"
: "${CROSS_MAKE_JOBS:=$(nproc 2>/dev/null || echo 1)}"

export CROSS_BUILDDIR CROSS_SRCDIR CROSS_DESTDIR CROSS_MAKE_JOBS

# ---------------------------------------------------------------------------
# 步骤 0: 初始化（在包 build.sh 最开始调用）
# ---------------------------------------------------------------------------
cross_step_init() {
    # 设置 PKG_NAME / PKG_VERSION / PKG_SRCURL / PKG_SHA256
    # 这些应该在 build.sh 开头已定义
    :
}

# ---------------------------------------------------------------------------
# 步骤 1: 准备 - 下载和准备源码
# ---------------------------------------------------------------------------
cross_step_prepare() {
    echo "[cross] 准备构建: ${CROSS_PKG_NAME:-unknown} (版本 ${CROSS_PKG_VERSION:-unknown})"

    # 创建目录
    mkdir -p "${CROSS_BUILDDIR}" "${CROSS_SRCDIR}" "${CROSS_DESTDIR}"

    # 如果提供了源码 URL，则下载
    if [[ -n "${CROSS_PKG_SRCURL:-}" ]]; then
        cross_download_source "${CROSS_PKG_SRCURL}" "${CROSS_PKG_SHA256:-}"
    fi

    # 可选：调用用户定义的 prepare hook
    if declare -f cross_hook_prepare &>/dev/null; then
        cross_hook_prepare
    fi
}

# ---------------------------------------------------------------------------
# 源码下载
# ---------------------------------------------------------------------------
cross_download_source() {
    local url="$1"
    local sha256="${2:-}"
    local filename
    filename="$(basename "$url")"
    local cachedir="${TMPDIR:-/tmp}/cross-cache"
    local cachefile="${cachedir}/${filename}"

    mkdir -p "${cachedir}"

    # 如果缓存存在且 SHA 匹配，跳过下载
    if [[ -f "${cachefile}" ]] && [[ -n "${sha256}" ]]; then
        local actual_sha
        actual_sha="$(sha256sum "${cachefile}" 2>/dev/null | cut -d' ' -f1)"
        if [[ "${actual_sha}" == "${sha256}" ]]; then
            echo "[cross] 使用缓存的源码: ${cachefile}"
            cross_extract_source "${cachefile}"
            return
        fi
    fi

    # 下载
    echo "[cross] 下载源码: ${url}"
    if command -v curl &>/dev/null; then
        curl -fsSL -o "${cachefile}" "${url}"
    elif command -v wget &>/dev/null; then
        wget -q -O "${cachefile}" "${url}"
    else
        echo "[cross] 错误: 找不到 curl 或 wget" >&2
        return 1
    fi

    # 校验 SHA256
    if [[ -n "${sha256}" ]]; then
        local actual_sha
        actual_sha="$(sha256sum "${cachefile}" | cut -d' ' -f1)"
        if [[ "${actual_sha}" != "${sha256}" ]]; then
            echo "[cross] 错误: SHA256 不匹配" >&2
            echo "  期望: ${sha256}" >&2
            echo "  实际: ${actual_sha}" >&2
            rm -f "${cachefile}"
            return 1
        fi
        echo "[cross] SHA256 校验通过 ✓"
    fi

    cross_extract_source "${cachefile}"
}

# ---------------------------------------------------------------------------
# 解压源码
# ---------------------------------------------------------------------------
cross_extract_source() {
    local archive="$1"
    echo "[cross] 解压源码: ${archive}"
    cd "${CROSS_SRCDIR}" || return 1

    case "$archive" in
        *.tar.gz|*.tgz)
            tar -xzf "$archive" --strip-components=1 2>/dev/null || \
            tar -xzf "$archive"
            ;;
        *.tar.bz2|*.tbz2)
            tar -xjf "$archive" --strip-components=1 2>/dev/null || \
            tar -xjf "$archive"
            ;;
        *.tar.xz|*.txz)
            tar -xJf "$archive" --strip-components=1 2>/dev/null || \
            tar -xJf "$archive"
            ;;
        *.tar.zst|*.tar.zstd)
            tar -I zstd -xf "$archive" --strip-components=1 2>/dev/null || \
            tar -I zstd -xf "$archive"
            ;;
        *.tar)
            tar -xf "$archive" --strip-components=1 2>/dev/null || \
            tar -xf "$archive"
            ;;
        *.zip)
            unzip -q "$archive"
            ;;
        *)
            echo "[cross] 无法识别的压缩格式: ${archive}" >&2
            return 1
            ;;
    esac

    echo "[cross] 源码解压完成 ✓"
}

# ---------------------------------------------------------------------------
# 步骤 2: 配置
# ---------------------------------------------------------------------------
cross_step_configure() {
    echo "[cross] 配置..."
    cd "${CROSS_SRCDIR}" || return 1

    # 调用用户定义的 configure hook（如果存在）
    if declare -f cross_hook_configure &>/dev/null; then
        cross_hook_configure
        return
    fi

    # 自动检测构建系统
    if [[ -f "./configure" ]]; then
        ./configure --prefix="${CROSS_PREFIX}" ${CROSS_CONFIGURE_ARGS:-}
    elif [[ -f "CMakeLists.txt" ]]; then
        cmake -B "${CROSS_BUILDDIR}" \
            -DCMAKE_INSTALL_PREFIX="${CROSS_PREFIX}" \
            -DCMAKE_BUILD_TYPE=Release \
            ${CROSS_CMAKE_ARGS:-}
    elif [[ -f "meson.build" ]]; then
        meson setup "${CROSS_BUILDDIR}" \
            --prefix="${CROSS_PREFIX}" \
            --buildtype=release \
            ${CROSS_MESON_ARGS:-}
    elif [[ -f "Makefile" || -f "makefile" || -f "GNUmakefile" ]]; then
        echo "[cross] 使用 Makefile，跳过 configure 步骤"
    else
        echo "[cross] 警告: 未检测到构建系统文件" >&2
    fi
}

# ---------------------------------------------------------------------------
# 步骤 3: 编译
# ---------------------------------------------------------------------------
cross_step_build() {
    echo "[cross] 编译 (${CROSS_MAKE_JOBS} 个线程)..."

    # 调用包自定义的 make 函数（termux_step_make）
    if declare -f termux_step_make &>/dev/null; then
        echo "[cross] 执行 termux_step_make..."
        cd "${CROSS_SRCDIR}" || return 1
        termux_step_make
        return
    fi

    if declare -f cross_hook_build &>/dev/null; then
        cross_hook_build
        return
    fi

    # 检测 Rust/Cargo 项目
    if [[ -f "${CROSS_SRCDIR}/Cargo.toml" ]]; then
        echo "[cross] 检测到 Rust/Cargo 项目"
        cd "${CROSS_SRCDIR}" || return 1
        cargo build --release ${CROSS_CARGO_ARGS:-}
        return
    fi

    # 优先使用 build 目录（CMake/Meson）
    if [[ -f "${CROSS_BUILDDIR}/Makefile" ]] || \
       [[ -f "${CROSS_BUILDDIR}/build.ninja" ]]; then
        cd "${CROSS_BUILDDIR}" || return 1
    else
        cd "${CROSS_SRCDIR}" || return 1
    fi

    if command -v ninja &>/dev/null && [[ -f "build.ninja" ]]; then
        ninja -j "${CROSS_MAKE_JOBS}"
    else
        make -j "${CROSS_MAKE_JOBS}" ${CROSS_MAKE_ARGS:-}
    fi
}

# ---------------------------------------------------------------------------
# 步骤 4: 安装
# ---------------------------------------------------------------------------
cross_step_install() {
    echo "[cross] 安装到 ${CROSS_DESTDIR}${CROSS_PREFIX}..."

    # 调用包自定义的 make_install 函数（termux_step_make_install）
    if declare -f termux_step_make_install &>/dev/null; then
        echo "[cross] 执行 termux_step_make_install..."
        # 确保安装目录存在
        mkdir -p "${CROSS_DESTDIR}${CROSS_PREFIX}/bin"
        mkdir -p "${CROSS_DESTDIR}${CROSS_PREFIX}/lib"
        mkdir -p "${CROSS_DESTDIR}${CROSS_PREFIX}/include"
        cd "${CROSS_SRCDIR}" || return 1
        # 设置 DESTDIR 环境变量和 TERMUX_PREFIX（适配使用 DESTDIR 的包）
        export DESTDIR="${CROSS_DESTDIR}"
        export TERMUX_PREFIX="${CROSS_DESTDIR}${CROSS_PREFIX}"
        termux_step_make_install
        return
    fi

    if declare -f cross_hook_install &>/dev/null; then
        cross_hook_install
        return
    fi

    if [[ -f "${CROSS_BUILDDIR}/build.ninja" ]]; then
        DESTDIR="${CROSS_DESTDIR}" ninja -C "${CROSS_BUILDDIR}" install
    elif [[ -f "${CROSS_BUILDDIR}/Makefile" ]]; then
        make -C "${CROSS_BUILDDIR}" install DESTDIR="${CROSS_DESTDIR}"
    elif [[ -f "${CROSS_SRCDIR}/Makefile" ]]; then
        make -C "${CROSS_SRCDIR}" install DESTDIR="${CROSS_DESTDIR}" \
            PREFIX="${CROSS_PREFIX}"
    else
        echo "[cross] 警告: 未检测到 Makefile，跳过 install 步骤" >&2
    fi
}

# ---------------------------------------------------------------------------
# 步骤 5: 安装后处理
# ---------------------------------------------------------------------------
cross_step_post_install() {
    if declare -f cross_hook_post_install &>/dev/null; then
        echo "[cross] 安装后处理..."
        cross_hook_post_install
    fi
}

# ---------------------------------------------------------------------------
# 步骤 6: 打包
# ---------------------------------------------------------------------------
cross_step_package() {
    echo "[cross] 打包..."

    local pkg_name="${CROSS_PKG_NAME:-package}"
    local pkg_version="${CROSS_PKG_VERSION:-1.0.0}"
    local output_dir="${CROSS_OUTPUTDIR:-${PWD}/output}"
    mkdir -p "${output_dir}"

    case "${CROSS_PACKAGE_FORMAT}" in
        pacman)
            cross_package_pacman "${output_dir}/${pkg_name}-${pkg_version}-${CROSS_ARCH}${CROSS_PACKAGE_EXT}"
            ;;
        debian)
            cross_package_debian "${output_dir}/${pkg_name}_${pkg_version}_${CROSS_ARCH}.deb"
            ;;
        *)
            # 默认 tar.gz
            tar -czf "${output_dir}/${pkg_name}-${pkg_version}-${CROSS_ARCH}.tar.gz" \
                -C "${CROSS_DESTDIR}" .
            ;;
    esac

    echo "[cross] 包已生成: ${output_dir}/${pkg_name}_${pkg_version}_${CROSS_ARCH}${CROSS_PACKAGE_EXT}"
}

# ---------------------------------------------------------------------------
# Pacman 包 (Arch Linux / MSYS2 / Termux)
# ---------------------------------------------------------------------------
cross_package_pacman() {
    local output_file="$1"

    # 简单实现：创建 tar 包
    tar -cf - -C "${CROSS_DESTDIR}" . | \
        case "${CROSS_PACKAGE_EXT}" in
            *.xz) xz -c -z - ;;
            *.zst) zstd -c -z --ultra -22 --rm ;;
            *) cat ;;
        esac > "${output_file}"
}

# ---------------------------------------------------------------------------
# Debian .deb 包
# ---------------------------------------------------------------------------
cross_package_debian() {
    local output_file="$1"

    # 创建 DEBIAN 目录
    mkdir -p "${CROSS_DESTDIR}/DEBIAN"
    cat > "${CROSS_DESTDIR}/DEBIAN/control" <<EOF
Package: ${CROSS_PKG_NAME:-package}
Version: ${CROSS_PKG_VERSION:-1.0.0}
Section: utils
Priority: optional
Architecture: ${CROSS_ARCH}
Maintainer: cross-build
Description: ${CROSS_PKG_DESCRIPTION:-Package built with cross-platform framework}
EOF

    dpkg-deb --build "${CROSS_DESTDIR}" "${output_file}" 2>/dev/null || \
        tar -czf "${output_file}" -C "${CROSS_DESTDIR}" .
}

# ---------------------------------------------------------------------------
# 完整的构建流水线（一站式调用）
# ---------------------------------------------------------------------------
cross_build_all() {
    cross_step_init
    cross_step_prepare
    cross_step_configure
    cross_step_build
    cross_step_install
    cross_step_post_install
    cross_step_package
    echo "[cross] 构建完成 ✓"
}
