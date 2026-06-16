#!/bin/bash
# =============================================================================
# update-sha256.sh — 自动计算并补齐 gpkg 包缺失的 SHA256
# =============================================================================
# 用法: 
#   ./scripts/update-sha256.sh              # 补全所有 SHA256 为空的包
#   ./scripts/update-sha256.sh gpkg/ggml    # 只补指定的包
# =============================================================================
set -euo pipefail

SCRIPTDIR="$(cd "$(dirname "$0")/.." && pwd)"
CACHE_DIR="${SCRIPTDIR}/.sha256-cache"
mkdir -p "${CACHE_DIR}"

update_pkg() {
    local pkg_dir="$1"
    local build_sh="${pkg_dir}/build.sh"

    [[ ! -f "${build_sh}" ]] && return

    # 读取现有 SHA256
    local current_sha
    current_sha="$(grep '^TERMUX_PKG_SHA256=' "${build_sh}" | sed 's/.*SHA256=//;s/"//g' | head -1)"

    # 如果已有值则跳过
    if [[ -n "${current_sha}" ]]; then
        echo "  ⏭️  ${pkg_dir##*/}: SHA256 已存在"
        return
    fi

    # 读取 SRCURL
    local srcurl
    srcurl="$(grep '^TERMUX_PKG_SRCURL=' "${build_sh}" | sed "s/.*SRCURL=//;s/\"//g" | head -1)"

    if [[ -z "${srcurl}" ]]; then
        echo "  ⏭️  ${pkg_dir##*/}: 无 SRCURL（本地源码包）"
        return
    fi

    # 展开 ${TERMUX_PKG_VERSION}
    local version
    version="$(grep '^TERMUX_PKG_VERSION=' "${build_sh}" | sed "s/.*=//;s/\"//g" | head -1)"
    srcurl="${srcurl//\$\{TERMUX_PKG_VERSION\}/${version}}"
    srcurl="${srcurl//\$TERMUX_PKG_VERSION/${version}}"

    local cache_file="${CACHE_DIR}/$(echo "${srcurl}" | sha256sum | cut -d' ' -f1)"

    echo -n "  [下载] ${pkg_dir##*/}..."
    if curl -fsSL -o "${cache_file}" "${srcurl}" 2>/dev/null; then
        local sha
        sha="$(sha256sum "${cache_file}" | cut -d' ' -f1)"
        echo " SHA256=${sha}"

        # 更新 build.sh
        sed -i "s/^TERMUX_PKG_SHA256=$/TERMUX_PKG_SHA256=${sha}/" "${build_sh}"
        echo "  ✅ ${pkg_dir##*/}: SHA256 已更新"
    else
        echo " ❌ 下载失败 (${srcurl})"
    fi
}

# 主逻辑
if [[ $# -gt 0 ]]; then
    for arg in "$@"; do
        update_pkg "${SCRIPTDIR}/${arg}"
    done
else
    for pkg_dir in "${SCRIPTDIR}"/gpkg/*/; do
        update_pkg "${pkg_dir}"
    done
fi

echo ""
echo "=== 完成 ==="
echo "提示: 执行 'git diff' 查看变更"
