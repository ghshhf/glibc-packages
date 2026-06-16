#!/bin/bash
# =============================================================================
# Windows Build Script (MSYS2/MinGW-w64)
# =============================================================================
# 在 MSYS2 shell 中运行此脚本进行 Windows 平台构建。
#
# 环境要求:
#   1. 安装 MSYS2 (https://www.msys2.org/)
#   2. 在 MSYS2 中安装必要工具:
#      pacman -Syu --noconfirm
#      pacman -S --noconfirm base-devel mingw-w64-x86_64-toolchain cmake git wget
#
# Usage:
#   ./build-windows.sh gpkg/neofetch     # 构建指定包
#   ./build-windows.sh --list             # 列出可用包
# =============================================================================

set -e

# 设置 MSYS2 环境
if [[ -z "${MSYSTEM}" ]]; then
    echo "错误: 请在 MSYS2 shell 中运行此脚本 (MINGW64/MINGW32/CLANG64)"
    exit 1
fi

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载跨平台框架
source "${SCRIPT_DIR}/cross-platform/platform-detect.sh" --platform windows

# 调用主构建脚本
exec "${SCRIPT_DIR}/build-cross.sh" "$@"
