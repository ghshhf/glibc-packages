#!/bin/bash
# =============================================================================
# BSD Build Script (FreeBSD/OpenBSD)
# =============================================================================
# 在 BSD 系统中运行此脚本进行构建。
#
# 环境要求:
#   FreeBSD: pkg install bash cmake gmake git wget curl
#   OpenBSD: pkg_add bash cmake gmake git wget curl
#
# Usage:
#   ./build-bsd.sh gpkg/neofetch     # 构建指定包
#   ./build-bsd.sh --list             # 列出可用包
# =============================================================================

set -e

# 检查是否在 BSD 上运行
OS_NAME="$(uname -s)"
if [[ ! "${OS_NAME}" =~ "BSD" ]]; then
    echo "警告: 此脚本设计用于 BSD 平台"
fi

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载跨平台框架
source "${SCRIPT_DIR}/cross-platform/platform-detect.sh" --platform bsd

# 调用主构建脚本
exec "${SCRIPT_DIR}/build-cross.sh" "$@"
