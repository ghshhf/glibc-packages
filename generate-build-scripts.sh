#!/bin/bash
# =============================================================================
# Cross-Platform Build System - Build Script Generator
# =============================================================================
# 生成各平台的构建脚本，支持 Windows、Linux、macOS、BSD 等平台。
#
# Usage:
#   ./generate-build-scripts.sh        # 生成所有平台的构建脚本
#   ./generate-build-scripts.sh windows  # 只生成 Windows 平台脚本
# =============================================================================

set -e

_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 平台列表
declare -A _PLATFORMS=(
    [linux]="Linux (Debian/Ubuntu/Arch)"
    [windows]="Windows (MSYS2/MinGW-w64)"
    [darwin]="macOS (Homebrew)"
    [bsd]="BSD (FreeBSD/OpenBSD)"
    [android]="Android (Termux)"
)

# 生成 Windows 构建脚本
_generate_windows_script() {
    local script_path="${_SCRIPT_DIR}/build-windows.sh"

    cat > "${script_path}" << 'EOF_WIN'
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
EOF_WIN

    chmod +x "${script_path}"
    echo "[cross] 生成 Windows 构建脚本: ${script_path}"
}

# 生成 macOS 构建脚本
_generate_darwin_script() {
    local script_path="${_SCRIPT_DIR}/build-darwin.sh"

    cat > "${script_path}" << 'EOF_DARWIN'
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
EOF_DARWIN

    chmod +x "${script_path}"
    echo "[cross] 生成 macOS 构建脚本: ${script_path}"
}

# 生成 Linux 构建脚本
_generate_linux_script() {
    local script_path="${_SCRIPT_DIR}/build-linux.sh"

    cat > "${script_path}" << 'EOF_LINUX'
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
EOF_LINUX

    chmod +x "${script_path}"
    echo "[cross] 生成 Linux 构建脚本: ${script_path}"
}

# 生成 BSD 构建脚本
_generate_bsd_script() {
    local script_path="${_SCRIPT_DIR}/build-bsd.sh"

    cat > "${script_path}" << 'EOF_BSD'
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
EOF_BSD

    chmod +x "${script_path}"
    echo "[cross] 生成 BSD 构建脚本: ${script_path}"
}

# 生成 Android 构建脚本
_generate_android_script() {
    local script_path="${_SCRIPT_DIR}/build-android.sh"

    cat > "${script_path}" << 'EOF_ANDROID'
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
EOF_ANDROID

    chmod +x "${script_path}"
    echo "[cross] 生成 Android 构建脚本: ${script_path}"
}

# 主函数
_main() {
    echo "[cross] 生成跨平台构建脚本..."

    if [[ -n "$1" ]]; then
        # 生成指定平台的脚本
        case "$1" in
            linux)
                _generate_linux_script
                ;;
            windows)
                _generate_windows_script
                ;;
            darwin)
                _generate_darwin_script
                ;;
            bsd)
                _generate_bsd_script
                ;;
            android)
                _generate_android_script
                ;;
            *)
                echo "错误: 未知平台 $1"
                exit 1
                ;;
        esac
    else
        # 生成所有平台的脚本
        _generate_linux_script
        _generate_windows_script
        _generate_darwin_script
        _generate_bsd_script
        _generate_android_script
    fi

    echo "[cross] 构建脚本生成完成"
}

_main "$@"
