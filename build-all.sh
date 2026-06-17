#!/bin/bash
# =============================================================================
# build-all.sh — SkyNet 本地一键多平台构建
# =============================================================================
#
# 用法:
#   ./build-all.sh [包目录1 包目录2 ...]
#   ./build-all.sh --list packages.txt                 # 从文件读取
#   ./build-all.sh --platform linux --clean gpkg/zlib  # 只构建某个平台
#   ./build-all.sh --platform wasm gpkg/zlib           # 构建 WASM/浏览器目标
#
# 默认行为:
#   自动检测当前系统，为所有可用的平台构建包:
#     • Linux: 本机 (x86_64) + 如果有交叉工具链则尝试 aarch64/i686
#     • Windows: 在 MSYS2 shell 内使用 MinGW-w64 本机工具链
#     • macOS: 本机 clang
#     • FreeBSD: 本机 gcc/clang
#     • WASM: 如果检测到 Emscripten SDK (emcc)，自动添加 WASM/浏览器构建
#
# 如果不指定包目录，默认构建以下 "里程碑" 包:
#   gpkg/zlib gpkg/ncurses gpkg/neofetch gpkg/libxml2
# =============================================================================

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# ---- 参数解析 ----
PLATFORMS=()
PACKAGES=()
PKG_LIST_FILE=""
CLEAN=false
JOBS=""

while (( $# )); do
    case "$1" in
        --platform)  PLATFORMS+=("$2"); shift 2 ;;
        --arch)      _CROSS_ARCH="$2"; shift 2 ;;
        --jobs|-j)   JOBS="$2"; shift 2 ;;
        --clean)     CLEAN=true; shift ;;
        --list)      PKG_LIST_FILE="$2"; shift 2 ;;
        -h|--help)
            sed -n '/^# 用法:/,/^# =============================================================================/p' "${BASH_SOURCE[0]}"
            exit 0
            ;;
        *)
            [[ -d "$1" ]] && PACKAGES+=("$1") || echo "警告: 跳过不存在目录 $1" >&2
            shift
            ;;
    esac
done

# ---- 包列表: 从文件 or 命令行 or 默认 ----
if [[ -n "${PKG_LIST_FILE}" ]]; then
    while IFS= read -r line; do
        [[ -z "$line" || "$line" == \#* ]] && continue
        PACKAGES+=("$line")
    done < "$PKG_LIST_FILE"
fi

if (( ${#PACKAGES[@]} == 0 )); then
    PACKAGES=(gpkg/zlib gpkg/ncurses gpkg/neofetch gpkg/libxml2)
    echo "未指定包，使用默认集合: ${PACKAGES[*]}"
fi

# ---- 检测当前操作系统 ----
HOST_OS="$(uname -s 2>/dev/null || echo Unknown)"
HOST_ARCH="$(uname -m 2>/dev/null || echo x86_64)"

echo "==============================================================================="
echo " glibc-packages · 本地多平台构建"
echo "  主机 OS:   ${HOST_OS}"
echo "  主机 arch: ${HOST_ARCH}"
echo "  包列表:     ${PACKAGES[*]}"
echo "==============================================================================="
echo

# ---- 根据主机 OS 和可用工具链决定要构建的 (platform, arch) 组合 ----
declare -a BUILD_JOBS=()  # 格式: "platform:arch:toolchain"

if (( ${#PLATFORMS[@]} > 0 )); then
    # 用户显式指定平台
    for p in "${PLATFORMS[@]}"; do
        case "$p" in
            linux)
                BUILD_JOBS+=("linux:x86_64:gcc-native")
                command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 && BUILD_JOBS+=("linux:aarch64:gcc-native")
                command -v i686-linux-gnu-gcc >/dev/null 2>&1 && BUILD_JOBS+=("linux:i686:gcc-native")
                ;;
            windows)
                if [[ -n "${MSYSTEM:-}" ]]; then
                    BUILD_JOBS+=("windows:x86_64:msys2-mingw")
                else
                    echo "  注意: Windows 平台需在 MSYS2 shell 内运行，跳过"
                fi
                ;;
            darwin)
                BUILD_JOBS+=("darwin:${HOST_ARCH/aarch64/aarch64}:clang-native")
                ;;
            bsd|freebsd)
                BUILD_JOBS+=("bsd:x86_64:gcc-native")
                ;;
            android)
                echo "  注意: Android 平台建议使用 build-android.sh 配合完整 NDK/CGCT"
                BUILD_JOBS+=("android:aarch64:cgct-android")
                ;;
            wasm|browser)
                if command -v emcc &>/dev/null || [[ -n "${EMSDK:-}" ]]; then
                    BUILD_JOBS+=("browser:wasm32:emscripten")
                    echo "  WASM: Emscripten 已就绪"
                else
                    echo "  注意: WASM 平台需要 Emscripten SDK，跳过"
                    echo "         安装: git clone https://github.com/emscripten-core/emsdk.git"
                    echo "               cd emsdk && ./emsdk install latest && source ./emsdk_env.sh"
                fi
                ;;
        esac
    done
else
    # 自动检测 — 根据主机选择合适的平台组合
    case "${HOST_OS}" in
        Linux*)
            BUILD_JOBS+=("linux:x86_64:gcc-native")
            if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
                BUILD_JOBS+=("linux:aarch64:gcc-native")
            else
                echo "  [提示] 安装 aarch64 交叉编译器以增加构建目标:"
                echo "         sudo apt install gcc-aarch64-linux-gnu"
            fi
            if command -v i686-linux-gnu-gcc >/dev/null 2>&1; then
                BUILD_JOBS+=("linux:i686:gcc-native")
            fi
            # WASM
            if command -v emcc &>/dev/null || [[ -n "${EMSDK:-}" ]]; then
                BUILD_JOBS+=("browser:wasm32:emscripten")
                echo "  WASM: Emscripten 已就绪"
            fi
            ;;
        Darwin*)
            BUILD_JOBS+=("darwin:${HOST_ARCH/aarch64/aarch64}:clang-native")
            # WASM
            if command -v emcc &>/dev/null || [[ -n "${EMSDK:-}" ]]; then
                BUILD_JOBS+=("browser:wasm32:emscripten")
            fi
            ;;
        FreeBSD|NetBSD|OpenBSD)
            BUILD_JOBS+=("bsd:x86_64:gcc-native")
            ;;
        CYGWIN*|MSYS*|MINGW*)
            BUILD_JOBS+=("windows:x86_64:msys2-mingw")
            # WASM
            if command -v emcc &>/dev/null || [[ -n "${EMSDK:-}" ]]; then
                BUILD_JOBS+=("browser:wasm32:emscripten")
            fi
            ;;
        *)
            echo "警告: 未知操作系统 ${HOST_OS}，按 Linux 处理" >&2
            BUILD_JOBS+=("linux:x86_64:gcc-native")
            ;;
    esac
fi

if (( ${#BUILD_JOBS[@]} == 0 )); then
    echo "错误: 没有可构建的平台/架构组合" >&2
    exit 1
fi

# ---- 为每个平台执行构建 ----
declare -i total_success=0 total_fail=0
declare -a FAIL_RECORDS=()

echo "计划构建:"
for job in "${BUILD_JOBS[@]}"; do
    IFS=':' read -r plat arch tc <<< "$job"
    echo "  • ${plat} / ${arch} / ${tc}"
done
echo

for job in "${BUILD_JOBS[@]}"; do
    IFS=':' read -r plat arch tc <<< "$job"
    echo
    echo "┌─────────────────────────────────────────────────────────"
    echo "│ 开始构建: ${plat} / ${arch} / ${tc}"
    echo "└─────────────────────────────────────────────────────────"

    EXTRA_ARGS=()
    [[ "${CLEAN}" == "true" ]] && EXTRA_ARGS+=("--clean")
    [[ -n "${JOBS}" ]] && EXTRA_ARGS+=("--jobs" "${JOBS}")

    if "${SCRIPT_DIR}/build-cross.sh" \
        --platform "${plat}" \
        --arch "${arch}" \
        --toolchain "${tc}" \
        "${EXTRA_ARGS[@]}" \
        "${PACKAGES[@]}"; then
        total_success+=1
    else
        total_fail+=1
        FAIL_RECORDS+=("${plat}/${arch}")
    fi
done

# ---- 最终报告 ----
echo
echo "==============================================================================="
echo "  全部构建完成"
echo "==============================================================================="
echo "  平台总数: ${#BUILD_JOBS[@]}"
echo "  成功:     ${total_success}"
echo "  失败:     ${total_fail}"
if (( ${#FAIL_RECORDS[@]} > 0 )); then
    echo "  失败平台: ${FAIL_RECORDS[*]}"
fi
echo "  产物目录: ${SCRIPT_DIR}/cross-build/"
echo "==============================================================================="

if (( total_fail > 0 )); then
    exit 1
fi
exit 0
