#!/bin/bash
# =============================================================================
# Core: Package Variable Adapter
# =============================================================================
# 读取 gpkg/<name>/build.sh，从中抽取 Termux 风格变量（TERMUX_PKG_*），
# 导出为跨平台框架使用的 CROSS_PKG_* 变量。
#
# 用法（在每个包的子 shell 中调用）：
#   source cross-platform/core/pkg-adapter.sh
#   cross_adapter_load_pkg "gpkg/zlib/build.sh"
#
# 导出变量：
#   CROSS_PKG_NAME / _VERSION / _SRCURL / _SHA256
#   CROSS_PKG_DEPENDS / _DESCRIPTION / _HOMEPAGE / _LICENSE
#   CROSS_PKG_BUILD_IN_SRC (true/false)
# =============================================================================

# ---------------------------------------------------------------------------
# 主入口：加载一个 gpkg/*/build.sh
# ---------------------------------------------------------------------------
cross_adapter_load_pkg() {
    local pkg_build_sh="$1"

    if [[ ! -f "${pkg_build_sh}" ]]; then
        echo "[cross-adapter] 错误: 找不到包文件: ${pkg_build_sh}" >&2
        return 1
    fi

    local pkg_dir pkg_name
    pkg_dir="$(cd "$(dirname "${pkg_build_sh}")" && pwd)"
    pkg_name="$(basename "${pkg_dir}")"

    # ---- Step 1: 在子 shell 中安全地读取变量 ----
    # 用 bash -c 避免污染调用 shell（除了 TERMUX_PKG_VERSION 等简单字符串外）
    local _vars
    _vars="$(
        _pkg_sh="${pkg_build_sh}"
        # 提供空的 termux helper，避免少数 gpkg 在解析期就调用它们
        termux_error_exit() { :; }
        # 读取
        source "${_pkg_sh}" 2>/dev/null || true
        # 打印
        echo "V:${TERMUX_PKG_VERSION:-1.0.0}"
        echo "U:${TERMUX_PKG_SRCURL:-}"
        echo "S:${TERMUX_PKG_SHA256:-}"
        echo "D:${TERMUX_PKG_DEPENDS:-}"
        echo "E:${TERMUX_PKG_DESCRIPTION:-Package}"
        echo "H:${TERMUX_PKG_HOMEPAGE:-}"
        echo "L:${TERMUX_PKG_LICENSE:-}"
        echo "I:${TERMUX_PKG_BUILD_IN_SRC:-false}"
    )" || true

    # ---- Step 2: 解析并导出 CROSS_PKG_* ----
    local _pkg_version _pkg_srcurl _pkg_sha256 _pkg_deps _pkg_desc
    local _pkg_homepage _pkg_license _pkg_in_src

    _pkg_version="$(echo "${_vars}"  | grep '^V:' | head -1 | sed 's/^V://; s/"//g')"
    _pkg_srcurl="$(echo "${_vars}"   | grep '^U:' | head -1 | sed 's/^U://; s/"//g')"
    _pkg_sha256="$(echo "${_vars}"   | grep '^S:' | head -1 | sed 's/^S://; s/"//g')"
    _pkg_deps="$(echo "${_vars}"     | grep '^D:' | head -1 | sed 's/^D://; s/"//g')"
    _pkg_desc="$(echo "${_vars}"     | grep '^E:' | head -1 | sed 's/^E://; s/"//g')"
    _pkg_homepage="$(echo "${_vars}" | grep '^H:' | head -1 | sed 's/^H://; s/"//g')"
    _pkg_license="$(echo "${_vars}"  | grep '^L:' | head -1 | sed 's/^L://; s/"//g')"
    _pkg_in_src="$(echo "${_vars}"   | grep '^I:' | head -1 | sed 's/^I://; s/"//g')"

    # 展开 URL 中的 ${TERMUX_PKG_VERSION}
    _pkg_srcurl="${_pkg_srcurl//\$\{TERMUX_PKG_VERSION\}/${_pkg_version}}"
    _pkg_srcurl="${_pkg_srcurl//\$TERMUX_PKG_VERSION/${_pkg_version}}"

    export CROSS_PKG_DIR="${pkg_dir}"
    export CROSS_PKG_NAME="${pkg_name}"
    export CROSS_PKG_VERSION="${_pkg_version}"
    export CROSS_PKG_SRCURL="${_pkg_srcurl}"
    export CROSS_PKG_SHA256="${_pkg_sha256}"
    export CROSS_PKG_DEPENDS="${_pkg_deps}"
    export CROSS_PKG_DESCRIPTION="${_pkg_desc}"
    export CROSS_PKG_HOMEPAGE="${_pkg_homepage}"
    export CROSS_PKG_LICENSE="${_pkg_license}"

    # 规范化为小写 true/false
    case "${_pkg_in_src}" in
        true|True|TRUE|yes|1) export CROSS_PKG_BUILD_IN_SRC=true ;;
        *)                    export CROSS_PKG_BUILD_IN_SRC=false ;;
    esac

    # 同时为 Termux 兼容层设值
    export TERMUX_PKG_NAME="${pkg_name}"
    export TERMUX_PKG_VERSION="${_pkg_version}"

    # ---- Step 3: 打印摘要 ----
    echo "  版本: ${_pkg_version}"
    echo "  源码: ${_pkg_srcurl:-<纯脚本包，无源码>}"
    [[ -n "${_pkg_deps}" ]] && echo "  依赖: ${_pkg_deps}"
    [[ "${CROSS_PKG_BUILD_IN_SRC}" == "true" ]] && echo "  [模式] in-src 构建（源码与构建同一目录）"

    return 0
}

echo "[cross-adapter] module loaded ✓"
