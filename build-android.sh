#!/bin/bash
# =============================================================================
# Android Build Script (Termux)
# =============================================================================
# 在 Termux 中运行此脚本进行 Android 平台构建。
#
# 环境要求:
#   1. 安装 Termux (https://termux.dev/)
#   2. 安装必要工具:
#      pkg install build-essential cmake make git wget curl
#
# Usage:
#   ./build-android.sh packages/neofetch     # 构建指定包
#   ./build-android.sh --list                # 列出可用包
# =============================================================================

set -e

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载跨平台框架
source "${SCRIPT_DIR}/cross-platform/platform-detect.sh" --platform android

# 调用主构建脚本
exec "${SCRIPT_DIR}/build-cross.sh" "$@"
