#!/bin/bash
# =============================================================================
# Core: Path Abstraction Layer
# =============================================================================
# 路径抽象层。提供跨平台的路径工具函数：
#   - 路径转换（Windows <-> Unix）
#   - 规范化（规范化、清理多余分隔符）
#   - 路径检测（是否为绝对路径、是否存在等）
#
# 使用此模块后，包 build.sh 中应优先使用：
#   cross_path_norm        - 规范化路径
#   cross_path_to_native   - 转换为本机风格路径
#   cross_path_join        - 拼接路径
#
# 核心变量:
#   CROSS_PREFIX           - 安装前缀 (e.g. /usr/local, /mingw64)
#   CROSS_PREFIX_BIN       - ${CROSS_PREFIX}/bin
#   CROSS_PREFIX_LIB       - ${CROSS_PREFIX}/lib
#   CROSS_PREFIX_INCLUDE   - ${CROSS_PREFIX}/include
#   CROSS_PREFIX_SHARE     - ${CROSS_PREFIX}/share
#   CROSS_PREFIX_ETC       - ${CROSS_PREFIX}/etc
# =============================================================================

# ---------------------------------------------------------------------------
# 确保前缀存在
# ---------------------------------------------------------------------------
: "${CROSS_PREFIX:=/usr/local}"

# 标准子目录
export CROSS_PREFIX_BIN="${CROSS_PREFIX}/bin"
export CROSS_PREFIX_LIB="${CROSS_PREFIX}/lib"
export CROSS_PREFIX_INCLUDE="${CROSS_PREFIX}/include"
export CROSS_PREFIX_SHARE="${CROSS_PREFIX}/share"
export CROSS_PREFIX_ETC="${CROSS_PREFIX}/etc"

# ---------------------------------------------------------------------------
# 路径规范化：清理多余分隔符、解析 . 和 ..
# ---------------------------------------------------------------------------
cross_path_norm() {
    local path="$1"
    # 去掉末尾的 /（根路径保留）
    if [[ "$path" != "/" ]]; then
        path="${path%/}"
    fi
    # 替换 // 为 /
    while [[ "$path" == *"//"* ]]; do
        path="${path//\/\//\/}"
    done
    echo "$path"
}

# ---------------------------------------------------------------------------
# 拼接路径
# ---------------------------------------------------------------------------
cross_path_join() {
    local base="$1"
    shift
    local joined="$base"
    for part in "$@"; do
        if [[ -z "$part" ]]; then continue; fi
        if [[ "$part" == /* ]]; then
            # 绝对路径替换 base
            joined="$part"
        else
            joined="${joined}/${part}"
        fi
    done
    cross_path_norm "$joined"
}

# ---------------------------------------------------------------------------
# 路径转换为平台本机格式
#   Linux/macOS/Android: /usr/local/bin
#   Windows (MSYS2):     /usr/local/bin (MSYS2 自动处理)
#   原生 Windows:        C:\usr\local\bin
# ---------------------------------------------------------------------------
cross_path_to_native() {
    local path="$1"
    case "${CROSS_PLATFORM}" in
        windows)
            # 在 MSYS2 内部，仍使用 Unix 风格路径
            # 如果需要原生 Windows 路径，此函数会转换
            if [[ "${CROSS_WIN_PATH_STYLE:-}" == "native" ]]; then
                # /c/Users/foo -> C:\Users\foo  或  /usr/bin -> C:\msys64\usr\bin
                if [[ "$path" == /c/* || "$path" == /d/* ]]; then
                    # 已有的驱动器前缀路径
                    local drive="${path:1:1}"
                    echo "${drive^^}:$(echo "${path:2}" | tr '/' '\\')"
                else
                    # 假定映射到 MSYS2 根
                    echo "$(cygpath -w "$path" 2>/dev/null || echo "$path")"
                fi
            else
                echo "$path"
            fi
            ;;
        *)
            echo "$path"
            ;;
    esac
}

# ---------------------------------------------------------------------------
# 从本机格式路径转换为内部路径
# ---------------------------------------------------------------------------
cross_path_from_native() {
    local path="$1"
    case "${CROSS_PLATFORM}" in
        windows)
            # C:\foo -> /c/foo
            if [[ "$path" == *:\\* ]]; then
                local drive="${path:0:1}"
                echo "/${drive,,}$(echo "${path:2}" | tr '\\' '/')"
            else
                echo "$path"
            fi
            ;;
        *)
            echo "$path"
            ;;
    esac
}

# ---------------------------------------------------------------------------
# 判断路径是否为绝对路径
# ---------------------------------------------------------------------------
cross_path_is_absolute() {
    local path="$1"
    case "$path" in
        /*)    return 0 ;;   # Unix 绝对路径
        *:\\*) return 0 ;;   # Windows 绝对路径 (C:\)
        *)     return 1 ;;
    esac
}

# ---------------------------------------------------------------------------
# 确保目录存在
# ---------------------------------------------------------------------------
cross_mkdir_p() {
    mkdir -p "$@" 2>/dev/null || true
}

# ---------------------------------------------------------------------------
# 临时目录
# ---------------------------------------------------------------------------
cross_mktempdir() {
    local prefix="${1:-cross}"
    if command -v mktemp &>/dev/null; then
        mktemp -d "${TMPDIR:-/tmp}/${prefix}.XXXXXX"
    else
        local dir="${TMPDIR:-/tmp}/${prefix}.$$"
        mkdir -p "$dir"
        echo "$dir"
    fi
}
