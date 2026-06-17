#!/bin/bash
# =============================================================================
# build-cross.sh — SkyNet 跨平台构建主入口（增强版）
# =============================================================================
# 为 5 大平台 × 4 架构提供统一的构建入口。
#
# 新增特性（相对原版本）：
#   • 一次传入多个包目录（空格分隔或 --list <FILE>）
#   • 构建摘要报告（成功 / 失败 / 跳过 统计）
#   • 支持"纯脚本包"（无 TERMUX_PKG_SRCURL，直接复制文件到 dest）
#   • 每个包在独立子 shell 中构建，避免环境变量交叉污染
#   • 完整的错误恢复：一个包失败不影响其他包
#
# 使用方法:
#   ./build-cross.sh [选项] <包目录1> [包目录2] ...
#
# 示例:
#   ./build-cross.sh gpkg/zlib gpkg/ncurses
#   ./build-cross.sh --platform linux --arch x86_64 --clean gpkg/zlib
#   ./build-cross.sh --platform windows --toolchain mingw-cross gpkg/libxml2
#   echo -e "gpkg/zlib\ngpkg/ncurses" | ./build-cross.sh --list -
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# 1. 目录和参数解析
# ---------------------------------------------------------------------------
CROSS_SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CROSS_FRAMEWORK_DIR="${CROSS_SCRIPTDIR}/cross-platform"
export CROSS_SCRIPTDIR CROSS_FRAMEWORK_DIR

# 全局变量
declare -a _CROSS_PKGS=()
_CROSS_ARG_PLATFORM=""
_CROSS_ARG_ARCH=""
_CROSS_ARG_PREFIX=""
_CROSS_ARG_TOOLCHAIN=""
_CROSS_ARG_SRCURL=""
_CROSS_ARG_VERSION=""
_CROSS_ARG_JOBS=""
_CROSS_ARG_CLEAN=false
_CROSS_ARG_ONLY_STEP=""
_CROSS_ARG_QUIET=false
_CROSS_ARG_CONTINUE_ON_ERROR=true
_CROSS_ARG_DEP_ORDER=false

while (( $# )); do
    case "$1" in
        --platform)    _CROSS_ARG_PLATFORM="$2"; shift 2 ;;
        --arch)        _CROSS_ARG_ARCH="$2";     shift 2 ;;
        --prefix)      _CROSS_ARG_PREFIX="$2";   shift 2 ;;
        --toolchain)   _CROSS_ARG_TOOLCHAIN="$2"; shift 2 ;;
        --src)         _CROSS_ARG_SRCURL="$2";   shift 2 ;;
        --version)     _CROSS_ARG_VERSION="$2";  shift 2 ;;
        --package-format) CROSS_PACKAGE_FORMAT="$2"; shift 2 ;;
        --output-format)  export CROSS_OUTPUT_FORMAT="$2"; shift 2 ;;
        -j|--jobs)     _CROSS_ARG_JOBS="$2";     shift 2 ;;
        --clean)       _CROSS_ARG_CLEAN=true;    shift ;;
        --only-configure) _CROSS_ARG_ONLY_STEP="configure"; shift ;;
        --only-build)  _CROSS_ARG_ONLY_STEP="build"; shift ;;
        --only-install) _CROSS_ARG_ONLY_STEP="install"; shift ;;
        -q|--quiet)    _CROSS_ARG_QUIET=true;    shift ;;
        --fail-fast)   _CROSS_ARG_CONTINUE_ON_ERROR=false; shift ;;
        --dep-order)   _CROSS_ARG_DEP_ORDER=true; shift ;;
        --list)
            if [[ "$2" == "-" ]]; then
                while IFS= read -r line; do
                    [[ -z "$line" || "$line" == \#* ]] && continue
                    _CROSS_PKGS+=("$line")
                done
            elif [[ -f "$2" ]]; then
                while IFS= read -r line; do
                    [[ -z "$line" || "$line" == \#* ]] && continue
                    _CROSS_PKGS+=("$line")
                done < "$2"
            fi
            shift 2
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
            _CROSS_PKGS+=("$1")
            shift
            ;;
    esac
done

# 检查必须参数
if (( ${#_CROSS_PKGS[@]} == 0 )); then
    echo "错误: 必须指定至少一个包目录" >&2
    echo "用法: ./build-cross.sh [选项] <包目录1> [包目录2] ..." >&2
    exit 1
fi

# 规范化包路径（转换为绝对路径）
declare -a _CROSS_PKG_ABS=()
for p in "${_CROSS_PKGS[@]}"; do
    if [[ -d "${p}" && -f "${p}/build.sh" ]]; then
        _CROSS_PKG_ABS+=("$(cd "${p}" && pwd)")
    else
        echo "警告: 跳过无效包: ${p}" >&2
    fi
done
_CROSS_PKGS=("${_CROSS_PKG_ABS[@]}")
unset _CROSS_PKG_ABS

if (( ${#_CROSS_PKGS[@]} == 0 )); then
    echo "错误: 没有有效的包可以构建" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# 2. 加载平台框架（platform-detect → core/*）
# ---------------------------------------------------------------------------
if [[ "${_CROSS_ARG_QUIET}" != "true" ]]; then
    echo "==============================================================================="
    echo " glibc-packages 跨平台构建工具 v2"
    echo "==============================================================================="
    echo
fi

_CROSS_DETECT_ARGS=()
[[ -n "${_CROSS_ARG_PLATFORM}" ]] && _CROSS_DETECT_ARGS+=(--platform "${_CROSS_ARG_PLATFORM}")
[[ -n "${_CROSS_ARG_ARCH}" ]]     && _CROSS_DETECT_ARGS+=(--arch "${_CROSS_ARG_ARCH}")

source "${CROSS_FRAMEWORK_DIR}/platform-detect.sh" "${_CROSS_DETECT_ARGS[@]}"

# prefix 覆盖
[[ -n "${_CROSS_ARG_PREFIX}" ]] && export CROSS_PREFIX="${_CROSS_ARG_PREFIX}"

# 自动选择工具链
if [[ -z "${_CROSS_ARG_TOOLCHAIN}" ]]; then
    case "${CROSS_PLATFORM}" in
        android) _CROSS_ARG_TOOLCHAIN="cgct-android" ;;
        linux)   _CROSS_ARG_TOOLCHAIN="$(command -v gcc >/dev/null && echo gcc-native || echo clang-native)" ;;
        windows)
            if [[ -n "${MSYSTEM:-}" ]]; then
                _CROSS_ARG_TOOLCHAIN="msys2-mingw"
            else
                _CROSS_ARG_TOOLCHAIN="mingw-cross"
            fi
            ;;
        darwin|bsd) _CROSS_ARG_TOOLCHAIN="clang-native" ;;
        *) _CROSS_ARG_TOOLCHAIN="gcc-native" ;;
    esac
fi

_CROSS_TOOLCHAIN_SCRIPT="${CROSS_FRAMEWORK_DIR}/toolchains/${_CROSS_ARG_TOOLCHAIN}.sh"
if [[ -f "${_CROSS_TOOLCHAIN_SCRIPT}" ]]; then
    source "${_CROSS_TOOLCHAIN_SCRIPT}"
    echo "[工具链] ${_CROSS_ARG_TOOLCHAIN}"
else
    echo "[工具链] 警告: 未找到脚本 ${_CROSS_TOOLCHAIN_SCRIPT}，使用默认" >&2
fi

# 加载包变量适配器（Termux → Cross 兼容层）
source "${CROSS_FRAMEWORK_DIR}/core/pkg-adapter.sh"

# 加载构建步骤框架（真正的构建核心）
source "${CROSS_FRAMEWORK_DIR}/core/build-steps.sh"

# 应用全局 CFLAGS
export CFLAGS="${CROSS_DEFAULT_CFLAGS:-} ${CFLAGS:-}"
export LDFLAGS="${CROSS_DEFAULT_LDFLAGS:-} ${LDFLAGS:-}"

# 构建并发数
: "${CROSS_MAKE_JOBS:=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)}"
[[ -n "${_CROSS_ARG_JOBS}" ]] && CROSS_MAKE_JOBS="${_CROSS_ARG_JOBS}"
export CROSS_MAKE_JOBS

# 清理旧的构建产物
_CROSS_BUILD_ROOT="${CROSS_SCRIPTDIR}/cross-build/${CROSS_PLATFORM}-${CROSS_ARCH}"
if [[ "${_CROSS_ARG_CLEAN}" == "true" ]]; then
    echo "[清理] rm -rf ${_CROSS_BUILD_ROOT}"
    rm -rf "${_CROSS_BUILD_ROOT}"
fi
mkdir -p "${_CROSS_BUILD_ROOT}"

if [[ "${_CROSS_ARG_QUIET}" != "true" ]]; then
    echo "[平台] ${CROSS_PLATFORM} (${CROSS_ARCH})"
    echo "[前缀] ${CROSS_PREFIX}"
    echo "[包格式] ${CROSS_PACKAGE_FORMAT}"
    echo "[包数量] ${#_CROSS_PKGS[@]}"
    echo "[并发] make -j${CROSS_MAKE_JOBS}"
    echo
fi

# ---------------------------------------------------------------------------
# 依赖排序：用 tsort 拓扑排序包的构建顺序
# ---------------------------------------------------------------------------
# 只有在用户明确 --dep-order 时启用；否则保持输入顺序（避免意外行为）
_CROSS_ORDERED=("${_CROSS_PKGS[@]}")
if [[ "${_CROSS_ARG_DEP_ORDER}" == "true" ]] && (( ${#_CROSS_PKGS[@]} > 1 )); then
    # 收集所有包名（目录 basename）
    declare -A _pkg_name_to_dir=()
    for _pd in "${_CROSS_PKGS[@]}"; do
        _nm="$(basename "${_pd}")"
        _pkg_name_to_dir["${_nm}"]="${_pd}"
        # 为 TERMUX_PKG_DEPENDS 里出现的 "${_nm}-glibc" 也建映射
        _pkg_name_to_dir["${_nm}-glibc"]="${_pd}"
    done

    # 生成依赖边：emit "dep_basename pkg_basename" 对
    _tsort_input=""
    for _pd in "${_CROSS_PKGS[@]}"; do
        _nm="$(basename "${_pd}")"
        _deps_line=$(grep -m1 'TERMUX_PKG_DEPENDS=' "${_pd}/build.sh" 2>/dev/null | sed "s/.*=\"//;s/\"//")
        if [[ -n "${_deps_line}" ]]; then
            IFS=',' read -ra _deps <<< "${_deps_line}"
            for _dep in "${_deps[@]}"; do
                _dep="${_dep// /}"
                _dir="${_pkg_name_to_dir[${_dep}]:-}"
                if [[ -n "${_dir}" ]]; then
                    # 使用依赖对应的目录 basename，而非依赖原名
                    _dep_basename="$(basename "${_dir}")"
                    _tsort_input+="${_dep_basename} ${_nm}"$'\n'
                fi
            done
        fi
        # 自引用节点（防止 tsort 丢掉孤立节点）
        _tsort_input+="${_nm} ${_nm}"$'\n'
    done

    # 执行 tsort
    _sorted_names=$(echo "${_tsort_input}" | tsort 2>/dev/null)
    if [[ $? -eq 0 ]] && [[ -n "${_sorted_names}" ]]; then
        _CROSS_ORDERED=()
        declare -A _seen=()
        while IFS= read -r _nm; do
            [[ -z "${_nm}" ]] && continue
            # 去重：同一个目录只构建一次
            _dir="${_pkg_name_to_dir[${_nm}]:-}"
            if [[ -n "${_dir}" ]] && [[ -z "${_seen[${_dir}]:-}" ]]; then
                _seen["${_dir}"]=1
                _CROSS_ORDERED+=("${_dir}")
            fi
        done <<< "${_sorted_names}"
        echo "[排序] 已按依赖关系拓扑排序"
    else
        echo "[排序] tsort 失败，保持输入顺序" >&2
    fi
    unset _pkg_name_to_dir _tsort_input _sorted_names _nm _pd _deps _deps_line _dir _seen _dep_basename
fi

# ---------------------------------------------------------------------------
# 3. 包构建（每个包在独立子 shell 中）
# ---------------------------------------------------------------------------

declare -i _success_count=0 _fail_count=0 _skip_count=0
declare -a _fail_list=()

for pkg_dir in "${_CROSS_ORDERED[@]}"; do
    pkg_name="$(basename "${pkg_dir}")"
    echo "────────────────────────────────────────────────"
    echo "▶ 构建: ${pkg_name}"
    echo "  目录: ${pkg_dir}"

    # 为每个包分配独立的构建目录
    _pkg_workdir="${_CROSS_BUILD_ROOT}/${pkg_name}"
    mkdir -p "${_pkg_workdir}"

    # 子 shell 中构建 —— 这样环境变量不会互相污染，一个失败也不影响其他包
    (
        # --- 路径 ---
        export CROSS_PKG_DIR="${pkg_dir}"
        export CROSS_SRCDIR="${_pkg_workdir}/src"
        export CROSS_BUILDDIR="${_pkg_workdir}/build"
        export CROSS_DESTDIR="${_pkg_workdir}/dest"
        export CROSS_OUTPUTDIR="${_pkg_workdir}/output"

        # --- 从 gpkg/*/build.sh 读取变量 ---
        cross_adapter_load_pkg "${pkg_dir}/build.sh"

        # --- BUILD_IN_SRC：如果包要求源码与构建同一目录 ---
        if [[ "${CROSS_PKG_BUILD_IN_SRC}" == "true" ]]; then
            export CROSS_BUILDDIR="${CROSS_SRCDIR}"
        fi

        # --- Termux 兼容变量 ---
        export TERMUX_PREFIX="${CROSS_PREFIX}"
        export TERMUX_ARCH="${CROSS_ARCH}"
        export TERMUX_PKG_SRCDIR="${CROSS_SRCDIR}"
        export TERMUX_PKG_BUILDDIR="${CROSS_BUILDDIR}"

        # --- 6 步标准构建流水线（统一使用 cross-step 框架） ---
        cross_step_prepare      || exit 1
        cross_step_configure    || exit 1
        cross_step_build        || exit 1
        cross_step_install      || exit 1
        cross_step_post_install || exit 1
        cross_step_package      || exit 1

        # --- 步骤 7: SSI .swbn 标准组件包（当 --output-format swbn 时） ---
        if [[ "${CROSS_OUTPUT_FORMAT:-}" == "swbn" ]]; then
            cross_step_swbn_package || exit 1
        fi

        echo "  ✓ 成功"
    )

    _rc=$?
    if [[ $_rc -eq 0 ]]; then
        _success_count+=1
    else
        _fail_count+=1
        _fail_list+=("${pkg_name}")
        echo "  ✗ 失败 (退出码 ${_rc})"
        if [[ "${_CROSS_ARG_CONTINUE_ON_ERROR}" != "true" ]]; then
            echo "错误: 已启用 fail-fast，停止构建" >&2
            exit 1
        fi
    fi
    echo
done

# ---------------------------------------------------------------------------
# 4. 构建摘要报告
# ---------------------------------------------------------------------------
echo "==============================================================================="
echo " 构建摘要 — ${CROSS_PLATFORM}/${CROSS_ARCH}"
echo "==============================================================================="
echo "  总包数: ${#_CROSS_PKGS[@]}"
echo "  成功:   ${_success_count}"
echo "  失败:   ${_fail_count}"
if (( ${#_fail_list[@]} > 0 )); then
    echo "  失败列表: ${_fail_list[*]}"
fi
echo "  产物目录: ${_CROSS_BUILD_ROOT}"
echo "==============================================================================="

if (( _fail_count > 0 )); then
    exit 1
fi
exit 0
