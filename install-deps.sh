#!/bin/bash
# =============================================================================
# Cross-Platform Dependency Management Script
# =============================================================================
# 统一的依赖管理入口，自动检测平台并安装必要的构建依赖。
#
# Usage:
#   ./install-deps.sh              # 安装所有依赖
#   ./install-deps.sh <pkg1> <pkg2>  # 安装指定包的依赖
#   ./install-deps.sh --list        # 列出支持的平台和依赖
#   ./install-deps.sh --check       # 检查当前环境的依赖状态
# =============================================================================

set -e

# 获取脚本目录
_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 保存命令行参数（platform-detect.sh 会消费参数）
_CROSS_DEPS_ARGS=("$@")

# 加载平台检测（使用空参数调用，避免消费我们的参数）
source "${_SCRIPT_DIR}/cross-platform/platform-detect.sh" ""

# 恢复命令行参数
set -- "${_CROSS_DEPS_ARGS[@]}"

# 加载依赖映射
source "${_SCRIPT_DIR}/cross-platform/core/deps.sh"

# ============================================================================
# 依赖数据库
# ============================================================================

# 每个平台的基础构建依赖
declare -A _CROSS_BASE_DEPS=(
    [linux]="build-essential cmake make git wget curl"
    [windows]="base-devel mingw-w64-x86_64-toolchain cmake git wget"
    [darwin]="xcode-command-line-tools cmake git wget curl"
    [bsd]="bash cmake gmake git wget curl"
)

# 常用包的依赖映射
declare -A _CROSS_PKG_DEPS=(
    [neofetch]="bash"
    [zlib]="glibc"
    [time]="glibc"
    [htop]="glibc ncurses-glibc"
    [sd]="rust cargo"
    [ncurses]="glibc"
    [readline]="glibc ncurses-glibc"
    [openssl]="glibc zlib-glibc"
    [sqlite]="glibc zlib-glibc"
    [python]="glibc zlib-glibc openssl-glibc"
    [git]="glibc zlib-glibc openssl-glibc libcurl-glibc"
)

# ============================================================================
# 平台安装命令
# ============================================================================

_cross_install_linux() {
    local deps="$1"
    local pkg_list=""

    for dep in $deps; do
        native_dep=$(cross_normalize_dep_name "$dep")
        pkg_list="${pkg_list} ${native_dep}"
    done

    # 检测包管理器
    if command -v apt-get &>/dev/null; then
        echo "[cross] 使用 apt-get 安装依赖..."
        sudo apt-get update
        sudo apt-get install -y $pkg_list
    elif command -v pacman &>/dev/null; then
        echo "[cross] 使用 pacman 安装依赖..."
        sudo pacman -Sy --noconfirm $pkg_list
    elif command -v dnf &>/dev/null; then
        echo "[cross] 使用 dnf 安装依赖..."
        sudo dnf install -y $pkg_list
    else
        echo "[cross] 错误：未检测到支持的包管理器"
        return 1
    fi
}

_cross_install_windows() {
    local deps="$1"
    local pkg_list=""

    for dep in $deps; do
        native_dep=$(cross_normalize_dep_name "$dep")
        pkg_list="${pkg_list} ${native_dep}"
    done

    if command -v pacman &>/dev/null; then
        echo "[cross] 使用 pacman (MSYS2) 安装依赖..."
        pacman -Sy --noconfirm $pkg_list
    else
        echo "[cross] 错误：需要在 MSYS2 shell 中运行"
        return 1
    fi
}

_cross_install_darwin() {
    local deps="$1"
    local pkg_list=""

    for dep in $deps; do
        native_dep=$(cross_normalize_dep_name "$dep")
        pkg_list="${pkg_list} ${native_dep}"
    done

    # 首先确保安装 Xcode 命令行工具
    if ! xcode-select -p &>/dev/null; then
        echo "[cross] 安装 Xcode Command Line Tools..."
        xcode-select --install
    fi

    if command -v brew &>/dev/null; then
        echo "[cross] 使用 Homebrew 安装依赖..."
        brew install $pkg_list
    else
        echo "[cross] 错误：未检测到 Homebrew"
        return 1
    fi
}

_cross_install_bsd() {
    local deps="$1"
    local pkg_list=""

    for dep in $deps; do
        native_dep=$(cross_normalize_dep_name "$dep")
        pkg_list="${pkg_list} ${native_dep}"
    done

    if command -v pkg &>/dev/null; then
        echo "[cross] 使用 pkg (FreeBSD) 安装依赖..."
        sudo pkg install -y $pkg_list
    elif command -v pkg_add &>/dev/null; then
        echo "[cross] 使用 pkg_add (OpenBSD) 安装依赖..."
        sudo pkg_add $pkg_list
    else
        echo "[cross] 错误：未检测到支持的包管理器"
        return 1
    fi
}

# ============================================================================
# 主安装函数
# ============================================================================

cross_install_deps() {
    local deps="$1"

    echo "[cross] 安装依赖: ${deps}"
    echo "[cross] 平台: ${CROSS_PLATFORM}"

    case "${CROSS_PLATFORM}" in
        linux)
            _cross_install_linux "$deps"
            ;;
        windows)
            _cross_install_windows "$deps"
            ;;
        darwin)
            _cross_install_darwin "$deps"
            ;;
        bsd)
            _cross_install_bsd "$deps"
            ;;
        *)
            echo "[cross] 错误：不支持的平台 ${CROSS_PLATFORM}"
            return 1
            ;;
    esac
}

# ============================================================================
# 列出依赖
# ============================================================================

cross_list_deps() {
    echo "=== 支持的平台 ==="
    echo "linux   - Linux (Debian/Ubuntu/Arch/RHEL)"
    echo "windows - Windows (MSYS2/MinGW-w64)"
    echo "darwin  - macOS (Homebrew)"
    echo "bsd     - BSD (FreeBSD/OpenBSD)"
    echo ""
    echo "=== 支持的包 ==="
    for pkg in "${!_CROSS_PKG_DEPS[@]}"; do
        echo "  $pkg: ${_CROSS_PKG_DEPS[$pkg]}"
    done
}

# ============================================================================
# 检查依赖状态
# ============================================================================

cross_check_deps() {
    local deps="$1"

    echo "[cross] 检查依赖状态..."
    echo "[cross] 平台: ${CROSS_PLATFORM}"

    local missing=""
    for dep in $deps; do
        if ! cross_check_dep_installed "$dep"; then
            native_dep=$(cross_normalize_dep_name "$dep")
            missing="${missing} ${dep} (${native_dep})"
        else
            native_dep=$(cross_normalize_dep_name "$dep")
            echo "[cross] ✓ ${dep} (${native_dep}) - 已安装"
        fi
    done

    if [[ -n "$missing" ]]; then
        echo "[cross] ✗ 缺失依赖:$missing"
        return 1
    else
        echo "[cross] 所有依赖已安装"
        return 0
    fi
}

# ============================================================================
# 主入口
# ============================================================================

_main() {
    # 平台检测已在加载 platform-detect.sh 时自动完成
    # 不需要调用 cross_detect_platform()

    # 处理参数
    case "$1" in
        --list)
            cross_list_deps
            exit 0
            ;;
        --check)
            shift  # 移除 --check 参数
            if [[ -n "$1" ]]; then
                # 检查指定包的依赖
                deps="${_CROSS_PKG_DEPS[$1]}"
                if [[ -z "$deps" ]]; then
                    deps="$1"
                fi
            else
                # 检查基础依赖
                deps="${_CROSS_BASE_DEPS[${CROSS_PLATFORM}]}"
            fi
            cross_check_deps "$deps"
            exit $?
            ;;
        --install-base)
            deps="${_CROSS_BASE_DEPS[${CROSS_PLATFORM}]}"
            cross_install_deps "$deps"
            exit $?
            ;;
        "")
            # 默认安装基础依赖
            deps="${_CROSS_BASE_DEPS[${CROSS_PLATFORM}]}"
            cross_install_deps "$deps"
            exit $?
            ;;
        *)
            # 安装指定包的依赖
            local all_deps=""
            for pkg in "$@"; do
                pkg_deps="${_CROSS_PKG_DEPS[$pkg]}"
                if [[ -n "$pkg_deps" ]]; then
                    all_deps="${all_deps} ${pkg_deps}"
                else
                    all_deps="${all_deps} $pkg"
                fi
            done
            cross_install_deps "$all_deps"
            exit $?
            ;;
    esac
}

# 执行主函数
_main "$@"
