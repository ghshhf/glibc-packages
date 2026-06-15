#!/bin/bash
# =============================================================================
# build-cross.sh - 跨平台包构建入口
# =============================================================================
# 这是 glibc-packages 跨平台框架的主入口脚本。
# 它会自动检测当前平台，加载相应的配置，然后构建指定的包。
#
# 使用方法:
#   ./build-cross.sh [选项] <包目录>
#
# 选项:
#   --platform <name>    指定平台: android | linux | windows | darwin | bsd
#   --arch <arch>        指定架构: x86_64 | aarch64 | i686 | arm
#   --prefix <path>      指定安装前缀 (默认自动选择)
#   --toolchain <name>   指定工具链: gcc-native | clang-native | mingw-cross
#                                       | msys2-mingw | cgct-android
#   --src <url>          覆盖源码 URL
#   --version <ver>      覆盖版本号
#   -j, --jobs <N>       并行编译线程数
#   --clean              清理之前的构建产物
#   --only-configure     只运行 configure 步骤
#   --only-build         只运行 build 步骤
#   --only-install       只运行 install 步骤
#   -h, --help           显示帮助
#
# 示例:
#   # 自动检测平台，构建 zlib
#   ./build-cross.sh gpkg/zlib
#
#   # 强制 Windows 平台，使用 MSYS2 MinGW 工具链
#   ./build-cross.sh --platform windows --toolchain msys2-mingw gpkg/zlib
#
#   # 交叉编译 Android (需要 CGCT 工具链)
#   ./build-cross.sh --platform android --arch aarch64 gpkg/zlib
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# 1. 获取脚本所在目录
# ---------------------------------------------------------------------------
CROSS_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CROSS_FRAMEWORK_DIR="${CROSS_SCRIPT_DIR}/cross-platform"
export CROSS_SCRIPTDIR="${CROSS_SCRIPT_DIR}"

# ---------------------------------------------------------------------------
# 2. 解析命令行参数
# ---------------------------------------------------------------------------
_CROSS_ARG_PLATFORM=""
_CROSS_ARG_ARCH=""
_CROSS_ARG_PREFIX=""
_CROSS_ARG_TOOLCHAIN=""
_CROSS_ARG_SRCURL=""
_CROSS_ARG_VERSION=""
_CROSS_ARG_JOBS=""
_CROSS_ARG_CLEAN=false
_CROSS_ARG_ONLY_STEP=""
_CROSS_PKG_DIR=""

while (( $# )); do
    case "$1" in
        --platform)
            _CROSS_ARG_PLATFORM="$2"
            shift 2
            ;;
        --arch)
            _CROSS_ARG_ARCH="$2"
            shift 2
            ;;
        --prefix)
            _CROSS_ARG_PREFIX="$2"
            shift 2
            ;;
        --toolchain)
            _CROSS_ARG_TOOLCHAIN="$2"
            shift 2
            ;;
        --src)
            _CROSS_ARG_SRCURL="$2"
            shift 2
            ;;
        --version)
            _CROSS_ARG_VERSION="$2"
            shift 2
            ;;
        -j|--jobs)
            _CROSS_ARG_JOBS="$2"
            shift 2
            ;;
        --clean)
            _CROSS_ARG_CLEAN=true
            shift
            ;;
        --only-configure)
            _CROSS_ARG_ONLY_STEP="configure"
            shift
            ;;
        --only-build)
            _CROSS_ARG_ONLY_STEP="build"
            shift
            ;;
        --only-install)
            _CROSS_ARG_ONLY_STEP="install"
            shift
            ;;
        -h|--help)
            sed -n '/^# 使用方法:/,/^# =============================================================================/p' "${BASH_SOURCE[0]}"
            exit 0
            ;;
        -*)
            echo "错误: 未知选项 '$1'" >&2
            exit 1
            ;;
        *)
            _CROSS_PKG_DIR="$1"
            shift
            ;;
    esac
done

# 检查必须参数
if [[ -z "${_CROSS_PKG_DIR}" ]]; then
    echo "错误: 必须指定包目录" >&2
    echo "用法: ./build-cross.sh [选项] <包目录>" >&2
    exit 1
fi

# 转换为绝对路径（避免 cd 后找不到文件
_CROSS_PKG_DIR="$(cd "${_CROSS_PKG_DIR}" && pwd)"

if [[ ! -d "${_CROSS_PKG_DIR}" ]]; then
    echo "错误: 包目录不存在: ${_CROSS_PKG_DIR}" >&2
    exit 1
fi

if [[ ! -f "${_CROSS_PKG_DIR}/build.sh" ]]; then
    echo "错误: 包目录中找不到 build.sh: ${_CROSS_PKG_DIR}/build.sh" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# 3. 加载跨平台框架 - 平台检测
# ---------------------------------------------------------------------------
echo "==============================================================================="
echo " glibc-packages 跨平台构建工具"
echo "==============================================================================="
echo

# 构造平台检测参数
_CROSS_DETECT_ARGS=()
if [[ -n "${_CROSS_ARG_PLATFORM}" ]]; then
    _CROSS_DETECT_ARGS+=(--platform "${_CROSS_ARG_PLATFORM}")
fi
if [[ -n "${_CROSS_ARG_ARCH}" ]]; then
    _CROSS_DETECT_ARGS+=(--arch "${_CROSS_ARG_ARCH}")
fi

# 加载平台检测
source "${CROSS_FRAMEWORK_DIR}/platform-detect.sh" "${_CROSS_DETECT_ARGS[@]}"

# 覆盖 PREFIX（如果用户指定）
if [[ -n "${_CROSS_ARG_PREFIX}" ]]; then
    export CROSS_PREFIX="${_CROSS_ARG_PREFIX}"
    echo "[cross] 使用自定义前缀: ${CROSS_PREFIX}"
fi

# 覆盖线程数
if [[ -n "${_CROSS_ARG_JOBS}" ]]; then
    export CROSS_MAKE_JOBS="${_CROSS_ARG_JOBS}"
fi

echo
echo "平台: ${CROSS_PLATFORM}"
echo "架构: ${CROSS_ARCH}"
echo "前缀: ${CROSS_PREFIX}"
echo "包格式: ${CROSS_PACKAGE_FORMAT}"
echo "主机三元组: ${CROSS_HOST_TRIPLE:-<本机>}"
echo "目标三元组: ${CROSS_TARGET_TRIPLE:-<本机>}"

# ---------------------------------------------------------------------------
# 4. 根据平台自动选择工具链
# ---------------------------------------------------------------------------
echo
echo "-------------------------------------------------------------------------------"
echo "工具链配置"
echo "-------------------------------------------------------------------------------"

# 自动选择工具链
if [[ -z "${_CROSS_ARG_TOOLCHAIN}" ]]; then
    case "${CROSS_PLATFORM}" in
        android)
            _CROSS_ARG_TOOLCHAIN="cgct-android"
            ;;
        linux)
            if command -v gcc &>/dev/null; then
                _CROSS_ARG_TOOLCHAIN="gcc-native"
            elif command -v clang &>/dev/null; then
                _CROSS_ARG_TOOLCHAIN="clang-native"
            else
                _CROSS_ARG_TOOLCHAIN="gcc-native"
            fi
            ;;
        windows)
            # 在 MSYS2 内部使用本机 MinGW 工具链；否则使用交叉编译
            if [[ -n "${MSYSTEM:-}" ]]; then
                _CROSS_ARG_TOOLCHAIN="msys2-mingw"
            else
                _CROSS_ARG_TOOLCHAIN="mingw-cross"
            fi
            ;;
        darwin)
            _CROSS_ARG_TOOLCHAIN="clang-native"
            ;;
        bsd)
            if command -v clang &>/dev/null; then
                _CROSS_ARG_TOOLCHAIN="clang-native"
            else
                _CROSS_ARG_TOOLCHAIN="gcc-native"
            fi
            ;;
        *)
            _CROSS_ARG_TOOLCHAIN="gcc-native"
            ;;
    esac
fi

# 加载工具链
_CROSS_TOOLCHAIN_SCRIPT="${CROSS_FRAMEWORK_DIR}/toolchains/${_CROSS_ARG_TOOLCHAIN}.sh"
if [[ -f "${_CROSS_TOOLCHAIN_SCRIPT}" ]]; then
    source "${_CROSS_TOOLCHAIN_SCRIPT}"
    echo "工具链: ${_CROSS_ARG_TOOLCHAIN}"
else
    echo "错误: 工具链脚本不存在: ${_CROSS_TOOLCHAIN_SCRIPT}" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# 5. 清理旧的构建产物（如果 --clean）
# ---------------------------------------------------------------------------
if [[ "${_CROSS_ARG_CLEAN}" == "true" ]]; then
    echo
    echo "清理旧的构建产物..."
    rm -rf "${CROSS_SCRIPTDIR}/cross-build/${CROSS_PLATFORM}-${CROSS_ARCH}"
fi

# ---------------------------------------------------------------------------
# 6. 设置包构建目录
# ---------------------------------------------------------------------------
_CROSS_BUILD_ROOT="${CROSS_SCRIPTDIR}/cross-build/${CROSS_PLATFORM}-${CROSS_ARCH}"
_CROSS_PKG_BASENAME="$(basename "${_CROSS_PKG_DIR}")"
_CROSS_PKG_WORKDIR="${_CROSS_BUILD_ROOT}/${_CROSS_PKG_BASENAME}"

mkdir -p "${_CROSS_PKG_WORKDIR}"
cd "${_CROSS_PKG_WORKDIR}"

export CROSS_PKG_NAME="${_CROSS_PKG_BASENAME}"
export CROSS_SRCDIR="${_CROSS_PKG_WORKDIR}/src"
export CROSS_BUILDDIR="${_CROSS_PKG_WORKDIR}/build"
export CROSS_DESTDIR="${_CROSS_PKG_WORKDIR}/dest"
export CROSS_OUTPUTDIR="${_CROSS_PKG_WORKDIR}/output"

mkdir -p "${CROSS_SRCDIR}" "${CROSS_BUILDDIR}" "${CROSS_DESTDIR}" "${CROSS_OUTPUTDIR}"

echo
echo "构建目录: ${_CROSS_PKG_WORKDIR}"
echo "包名: ${CROSS_PKG_NAME}"

# ---------------------------------------------------------------------------
# 7. 加载包 build.sh
# ---------------------------------------------------------------------------
echo
echo "-------------------------------------------------------------------------------"
echo "加载包: ${_CROSS_PKG_DIR}"
echo "-------------------------------------------------------------------------------"

# 复制源码补丁等资源到工作目录
if compgen -G "${_CROSS_PKG_DIR}/*.patch" > /dev/null; then
    cp "${_CROSS_PKG_DIR}"/*.patch "${CROSS_SRCDIR}/" 2>/dev/null || true
fi

# 从包 build.sh 中提取基本信息
_CROSS_PKG_BUILDSH="${_CROSS_PKG_DIR}/build.sh"

# 尝试用 bash source 来获取变量值（支持 shell 变量引用如 ${_PKG_VERSION}）
# 只提取需要的变量，不会执行 termux_step_* 函数
_cross_extract_vars() {
    bash -c '
        source "'"${_CROSS_PKG_BUILDSH}"'" 2>/dev/null
        echo "V:${TERMUX_PKG_VERSION:-1.0.0}"
        echo "U:${TERMUX_PKG_SRCURL:-}"
        echo "S:${TERMUX_PKG_SHA256:-}"
        echo "D:${TERMUX_PKG_DEPENDS:-}"
        echo "E:${TERMUX_PKG_DESCRIPTION:-Package}"
    ' 2>/dev/null || {
        # 回退到简单的 grep 提取
        echo "V:$(grep -E "^TERMUX_PKG_VERSION=" "'"${_CROSS_PKG_BUILDSH}"'" | head -1 | cut -d= -f2-)"
        echo "U:$(grep -E "^TERMUX_PKG_SRCURL=" "'"${_CROSS_PKG_BUILDSH}"'" | head -1 | cut -d= -f2-)"
        echo "S:$(grep -E "^TERMUX_PKG_SHA256=" "'"${_CROSS_PKG_BUILDSH}"'" | head -1 | cut -d= -f2)"
        echo "D:$(grep -E "^TERMUX_PKG_DEPENDS=" "'"${_CROSS_PKG_BUILDSH}"'" | head -1 | cut -d= -f2-)"
        echo "E:$(grep -E "^TERMUX_PKG_DESCRIPTION=" "'"${_CROSS_PKG_BUILDSH}"'" | head -1 | cut -d= -f2-)"
    }
}

# 解析提取的变量
while IFS= read -r line; do
    case "${line:0:2}" in
        V:) export CROSS_PKG_VERSION="${line:2}" ;;
        U:) export CROSS_PKG_SRCURL="${line:2}" ;;
        S:) export CROSS_PKG_SHA256="${line:2}" ;;
        D:) export CROSS_PKG_DEPENDS="${line:2}" ;;
        E:) export CROSS_PKG_DESCRIPTION="${line:2}" ;;
    esac
done < <(_cross_extract_vars)

# 移除引号
CROSS_PKG_VERSION="${CROSS_PKG_VERSION//\"/}"
CROSS_PKG_SRCURL="${CROSS_PKG_SRCURL//\"/}"
CROSS_PKG_SHA256="${CROSS_PKG_SHA256//\"/}"
CROSS_PKG_DEPENDS="${CROSS_PKG_DEPENDS//\"/}"
CROSS_PKG_DESCRIPTION="${CROSS_PKG_DESCRIPTION//\"/}"

# 兼容 Termux 风格变量（供包 build.sh 使用）
# TERMUX_PREFIX 用于安装路径（如 /usr/local）
export TERMUX_PREFIX="${CROSS_PREFIX}"
export TERMUX_ARCH="${CROSS_ARCH}"
# 同时导出 TERMUX_PKG_* 变量供包 build.sh 使用
export TERMUX_PKG_NAME="${_CROSS_PKG_BASENAME}"
export TERMUX_PKG_VERSION="${CROSS_PKG_VERSION}"
export TERMUX_PKG_SRCDIR="${CROSS_SRCDIR}"
export TERMUX_PKG_BUILDDIR="${CROSS_BUILDDIR}"

# 展开 URL 中的变量（如 ${TERMUX_PKG_VERSION}）
CROSS_PKG_SRCURL="${CROSS_PKG_SRCURL//\$\{TERMUX_PKG_VERSION\}/${CROSS_PKG_VERSION}}"
CROSS_PKG_SRCURL="${CROSS_PKG_SRCURL//\$TERMUX_PKG_VERSION/${CROSS_PKG_VERSION}}"

# 命令行参数覆盖
if [[ -n "${_CROSS_ARG_VERSION}" ]]; then
    export CROSS_PKG_VERSION="${_CROSS_ARG_VERSION}"
fi
if [[ -n "${_CROSS_ARG_SRCURL}" ]]; then
    export CROSS_PKG_SRCURL="${_CROSS_ARG_SRCURL}"
fi

echo "版本: ${CROSS_PKG_VERSION}"
echo "源码: ${CROSS_PKG_SRCURL:-<未指定>}"
echo "依赖: ${CROSS_PKG_DEPENDS:-<无>}"

# ---------------------------------------------------------------------------
# 8. 依赖检查（转换为平台原生命名）
# ---------------------------------------------------------------------------
if [[ -n "${CROSS_PKG_DEPENDS}" ]]; then
    echo
    echo "检查依赖..."
    IFS=',' read -ra _CROSS_DEP_ARRAY <<< "${CROSS_PKG_DEPENDS}"
    for dep in "${_CROSS_DEP_ARRAY[@]}"; do
        dep="$(echo "$dep" | tr -d ' ')"
        local_name="$(cross_normalize_dep_name "$dep")"
        if cross_check_dep_installed "$dep" >/dev/null 2>&1; then
            echo "  ✓ ${local_name}"
        else
            echo "  ✗ ${local_name} (未安装，继续构建...)"
        fi
    done
fi

# ---------------------------------------------------------------------------
# 9. 加载包的 build.sh 中的自定义函数
# ---------------------------------------------------------------------------
# Source the package build.sh to get custom functions (termux_step_*)
source "${_CROSS_PKG_BUILDSH}" 2>/dev/null || true

# ---------------------------------------------------------------------------
# 10. 执行构建
# ---------------------------------------------------------------------------
echo
echo "==============================================================================="
echo "开始构建"
echo "==============================================================================="
echo

# 定义执行构建步骤的函数
_execute_build() {
    cd "${CROSS_SRCDIR}" || exit 1
    cross_step_prepare

    # 调用 termux_step_pre_configure (如果存在)
    if declare -f termux_step_pre_configure &>/dev/null; then
        echo "[cross] 执行 pre_configure hook..."
        termux_step_pre_configure
    fi

    cross_step_configure
    cross_step_build
    cross_step_install

    # 调用 termux_step_post_make_install (如果存在)
    if declare -f termux_step_post_make_install &>/dev/null; then
        echo "[cross] 执行 post_make_install hook..."
        termux_step_post_make_install
    fi

    cross_step_post_install
    cross_step_package
}

# 根据 --only-* 参数决定执行哪些步骤
case "${_CROSS_ARG_ONLY_STEP}" in
    configure)
        cd "${CROSS_SRCDIR}" || exit 1
        cross_step_prepare
        if declare -f termux_step_pre_configure &>/dev/null; then
            termux_step_pre_configure
        fi
        cross_step_configure
        ;;
    build)
        cd "${CROSS_SRCDIR}" || exit 1
        cross_step_build
        ;;
    install)
        if declare -f termux_step_post_make_install &>/dev/null; then
            termux_step_post_make_install
        fi
        cross_step_install
        cross_step_post_install
        cross_step_package
        ;;
    *)
        _execute_build
        ;;
esac

# ---------------------------------------------------------------------------
# 10. 完成
# ---------------------------------------------------------------------------
echo
echo "==============================================================================="
echo "构建完成 ✓"
echo "==============================================================================="
echo
echo "包: ${CROSS_PKG_NAME}-${CROSS_PKG_VERSION}"
echo "平台: ${CROSS_PLATFORM} (${CROSS_ARCH})"
echo "产物目录: ${CROSS_OUTPUTDIR}"
echo

if compgen -G "${CROSS_OUTPUTDIR}/*" > /dev/null; then
    echo "生成的文件:"
    ls -lh "${CROSS_OUTPUTDIR}"
fi

# 清理内部变量
unset _CROSS_DETECT_ARGS _CROSS_ARG_PLATFORM _CROSS_ARG_ARCH _CROSS_ARG_PREFIX
unset _CROSS_ARG_TOOLCHAIN _CROSS_ARG_SRCURL _CROSS_ARG_VERSION
unset _CROSS_ARG_JOBS _CROSS_ARG_CLEAN _CROSS_ARG_ONLY_STEP
unset _CROSS_PKG_DIR _CROSS_PKG_BASENAME _CROSS_PKG_WORKDIR
unset _CROSS_BUILD_ROOT _CROSS_PKG_BUILDSH
unset _CROSS_TOOLCHAIN_SCRIPT
