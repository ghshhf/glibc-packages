#!/bin/bash
# =============================================================================
# macOS Build Script (Homebrew)
# =============================================================================
# 在 macOS 中运行此脚本进行构建。
#
# 环境要求:
#   1. 安装 Homebrew (https://brew.sh/)
#   2. 安装必要工具:
#      brew install cmake git wget curl
#
# Usage:
#   ./build-darwin.sh gpkg/neofetch     # 构建指定包
#   ./build-darwin.sh --list             # 列出可用包
# =============================================================================

set -e

# 检查是否在 macOS 上运行
if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "错误: 此脚本只能在 macOS 上运行"
    exit 1
fi

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载跨平台框架
source "${SCRIPT_DIR}/cross-platform/platform-detect.sh" --platform darwin

# 调用主构建脚本
exec "${SCRIPT_DIR}/build-cross.sh" "$@"
