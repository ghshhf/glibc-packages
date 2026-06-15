#!/bin/bash
# =============================================================================
# Linux Build Script
# =============================================================================
# 在 Linux 中运行此脚本进行构建。
#
# 环境要求:
#   Debian/Ubuntu: apt-get install build-essential cmake make git wget curl
#   Arch Linux:    pacman -S base-devel cmake git wget curl
#   RHEL/Fedora:   dnf groupinstall "Development Tools" && dnf install cmake git wget curl
#
# Usage:
#   ./build-linux.sh gpkg/neofetch     # 构建指定包
#   ./build-linux.sh --list             # 列出可用包
# =============================================================================

set -e

# 检查是否在 Linux 上运行
if [[ "$(uname -s)" != "Linux" ]]; then
    echo "警告: 此脚本设计用于 Linux 平台"
fi

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载跨平台框架
source "${SCRIPT_DIR}/cross-platform/platform-detect.sh" --platform linux

# 调用主构建脚本
exec "${SCRIPT_DIR}/build-cross.sh" "$@"
