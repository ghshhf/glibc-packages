#!/bin/bash
# =============================================================================
# Cross-Platform Framework - Platform Detection
# =============================================================================
# 跨平台框架的入口模块。负责：
#   1. 检测当前操作系统和架构
#   2. 加载对应平台的配置脚本
#   3. 设置统一的抽象变量供上层使用
#
# 使用方式:
#   source cross-platform/platform-detect.sh [--platform <name>] [--arch <arch>]
#
# 支持的平台:
#   android   - Android (Termux)
#   linux     - Linux (本机)
#   windows   - Windows (MSYS2/MinGW)
#   darwin    - macOS
#   bsd       - FreeBSD / NetBSD / OpenBSD
#   browser   - 浏览器 / WebAssembly (Emscripten)
#   wasm      - browser 的别名
#
# 输出变量:
#   CROSS_PLATFORM         - 平台名称 (android|linux|windows|darwin|bsd|browser)
#   CROSS_ARCH             - 架构名称 (x86_64|aarch64|i686|arm|armv7a)
#   CROSS_PREFIX           - 安装前缀
#   CROSS_SCRIPTDIR        - 框架脚本目录
# =============================================================================

set -eo pipefail

# ---------------------------------------------------------------------------
# 1. 获取脚本目录
# ---------------------------------------------------------------------------
_CROSS_DETECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export CROSS_SCRIPTDIR="$(cd "${_CROSS_DETECT_DIR}/.." && pwd)"

# ---------------------------------------------------------------------------
# 2. 解析命令行参数（允许强制指定平台/架构）
# ---------------------------------------------------------------------------
_CROSS_FORCE_PLATFORM=""
_CROSS_FORCE_ARCH=""

while (( $# )); do
    case "$1" in
        --platform)
            _CROSS_FORCE_PLATFORM="$2"
            shift 2
            ;;
        --arch)
            _CROSS_FORCE_ARCH="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

# ---------------------------------------------------------------------------
# 3. 检测操作系统
# ---------------------------------------------------------------------------
_detect_os() {
    if [[ -n "${_CROSS_FORCE_PLATFORM}" ]]; then
        echo "${_CROSS_FORCE_PLATFORM}"
        return
    fi

    case "$(uname -s 2>/dev/null || echo Unknown)" in
        Linux)
            if [[ "$(uname -o 2>/dev/null)" == *Android* ]] || \
               [[ -e "/system/bin/app_process" ]] || \
               [[ -d "/data/data/com.termux" ]]; then
                echo "android"
            else
                echo "linux"
            fi
            ;;
        Darwin)
            echo "darwin"
            ;;
        CYGWIN*|MSYS*|MINGW*|Windows_NT)
            echo "windows"
            ;;
        FreeBSD|NetBSD|OpenBSD|DragonFly)
            echo "bsd"
            ;;
        *)
            echo "linux"  # 默认回退到 Linux
            ;;
    esac
}

# ---------------------------------------------------------------------------
# 4. 检测架构
# ---------------------------------------------------------------------------
_normalize_arch() {
    local raw_arch="${1:-}"
    if [[ -n "${_CROSS_FORCE_ARCH}" ]]; then
        echo "${_CROSS_FORCE_ARCH}"
        return
    fi

    case "$raw_arch" in
        x86_64|amd64|AMD64)
            echo "x86_64"
            ;;
        aarch64|arm64|ARM64)
            echo "aarch64"
            ;;
        i686|i386|x86|X86)
            echo "i686"
            ;;
        armv7a|armv7l|arm)
            echo "arm"
            ;;
        wasm32|wasm64|wasm)
            echo "wasm32"
            ;;
        *)
            echo "x86_64"  # 默认回退
            ;;
    esac
}

# ---------------------------------------------------------------------------
# 5. 设置全局平台变量
# ---------------------------------------------------------------------------
export CROSS_PLATFORM="$(_detect_os)"
export CROSS_ARCH="$(_normalize_arch "$(uname -m 2>/dev/null)")"

# ---------------------------------------------------------------------------
# 6. 加载对应平台的配置
# ---------------------------------------------------------------------------
_CROSS_PLATFORM_SCRIPT="${_CROSS_DETECT_DIR}/platforms/${CROSS_PLATFORM}.sh"
if [[ ! -f "${_CROSS_PLATFORM_SCRIPT}" ]]; then
    echo "[cross-platform] 警告: 未找到平台配置: ${_CROSS_PLATFORM_SCRIPT}"
    echo "[cross-platform] 使用 linux 作为回退平台..."
    export CROSS_PLATFORM="linux"
    _CROSS_PLATFORM_SCRIPT="${_CROSS_DETECT_DIR}/platforms/linux.sh"
fi

echo "[cross-platform] 检测到平台: ${CROSS_PLATFORM} (${CROSS_ARCH})"
echo "[cross-platform] 加载平台配置: ${_CROSS_PLATFORM_SCRIPT}"

source "${_CROSS_PLATFORM_SCRIPT}"

# ---------------------------------------------------------------------------
# 7. 先设置安装前缀（使用平台配置提供的默认值）
# ---------------------------------------------------------------------------
if [[ -z "${CROSS_PREFIX:-}" ]]; then
    if [[ -n "${CROSS_PREFIX_DEFAULT:-}" ]]; then
        export CROSS_PREFIX="${CROSS_PREFIX_DEFAULT}"
    else
        case "${CROSS_PLATFORM}" in
            android) export CROSS_PREFIX="/data/data/com.termux/files/usr" ;;
            linux)   export CROSS_PREFIX="/usr/local" ;;
            windows) export CROSS_PREFIX="/mingw64" ;;
            darwin)  export CROSS_PREFIX="/usr/local" ;;
            bsd)     export CROSS_PREFIX="/usr/local" ;;
            browser|wasm) export CROSS_PREFIX="/usr/local" ;;
        esac
    fi
fi

# ---------------------------------------------------------------------------
# 8. 加载核心模块（此时 CROSS_PREFIX 已设置）
# ---------------------------------------------------------------------------
source "${_CROSS_DETECT_DIR}/core/paths.sh"
source "${_CROSS_DETECT_DIR}/core/deps.sh"
source "${_CROSS_DETECT_DIR}/core/build-steps.sh"

echo "[cross-platform] 安装前缀: ${CROSS_PREFIX}"
echo "[cross-platform] 框架初始化完成 ✓"

# 清理内部变量
unset _CROSS_DETECT_DIR _CROSS_FORCE_PLATFORM _CROSS_FORCE_ARCH _CROSS_PLATFORM_SCRIPT
