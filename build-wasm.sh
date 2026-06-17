#!/bin/bash
# =============================================================================
# build-wasm.sh — WASM/浏览器平台构建快捷脚本
# =============================================================================
# 基于 build-cross.sh 的封装，自动设置 browser 平台和 emscripten 工具链。
#
# 用法:
#   ./build-wasm.sh [选项] <包目录1> [包目录2] ...
#
# 选项:
#   -j, --jobs N          并行编译数
#   --clean               清理旧的构建产物
#   --arch ARCH           目标架构 (wasm32, wasm64 — 默认 wasm32)
#   --list FILE           从文件读取包列表
#   --npm                 构建完成后自动生成 npm 包
#   --serve PORT          构建后启动静态 HTTP 服务器用于测试
#   --test                使用 node.js 运行 WASM 模块测试
#   -q, --quiet           安静模式
#   --fail-fast           遇到错误立即停止
#   -h, --help            显示帮助
#
# 环境变量:
#   EMSDK        - Emscripten SDK 路径 (如 ~/emsdk)
#
# 示例:
#   ./build-wasm.sh gpkg/zlib
#   ./build-wasm.sh --clean --jobs 8 gpkg/zlib gpkg/libpng
#   ./build-wasm.sh --list wasm-packages.list
#   ./build-wasm.sh --npm --serve 8080 gpkg/openssl
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ---- 参数解析 ----
PACKAGES=()
ARCH="wasm32"
JOBS=""
CLEAN=false
QUIET=false
FAIL_FAST=false
DO_NPM=false
DO_SERVE=""
DO_TEST=false
PKG_LIST_FILE=""

while (( $# )); do
    case "$1" in
        -j|--jobs)       JOBS="$2"; shift 2 ;;
        --clean)         CLEAN=true; shift ;;
        --arch)          ARCH="$2"; shift 2 ;;
        --npm)           DO_NPM=true; shift ;;
        --serve)         DO_SERVE="$2"; shift 2 ;;
        --test)          DO_TEST=true; shift ;;
        -q|--quiet)      QUIET=true; shift ;;
        --fail-fast)     FAIL_FAST=true; shift ;;
        --list)          PKG_LIST_FILE="$2"; shift 2 ;;
        -h|--help)
            sed -n '/^# 用法:/,/^# =============================================================================/p' "${BASH_SOURCE[0]}"
            exit 0
            ;;
        *)
            PACKAGES+=("$1")
            shift
            ;;
    esac
done

# 从文件读取包列表
if [[ -n "${PKG_LIST_FILE}" ]]; then
    while IFS= read -r line; do
        [[ -z "$line" || "$line" == \#* ]] && continue
        PACKAGES+=("$line")
    done < "${PKG_LIST_FILE}"
fi

# 默认包
if (( ${#PACKAGES[@]} == 0 )); then
    PACKAGES=(gpkg/zlib)
    echo "未指定包，使用默认: ${PACKAGES[*]}"
fi

# ---- 检查 Emscripten SDK ----
check_emscripten() {
    # 如果 EMSDK 环境变量已设置
    if [[ -n "${EMSDK:-}" ]]; then
        echo "[WASM] 使用 EMSDK: ${EMSDK}"
        return 0
    fi

    # 检查 PATH 中的 emcc
    if command -v emcc &>/dev/null; then
        echo "[WASM] emcc 已在 PATH 中: $(command -v emcc)"
        return 0
    fi

    # 尝试 source emsdk_env
    for _emsdk_sh in \
        "${HOME}/emsdk/emsdk_env.sh" \
        "${HOME}/dev/emsdk/emsdk_env.sh" \
        "/opt/emsdk/emsdk_env.sh"; do
        if [[ -f "${_emsdk_sh}" ]]; then
            echo "[WASM] 加载 Emscripten: ${_emsdk_sh}"
            source "${_emsdk_sh}"
            return 0
        fi
    done

    echo "[WASM] 警告: 未找到 Emscripten SDK" >&2
    echo "[WASM] 请安装: https://emscripten.org/docs/getting_started/downloads.html" >&2
    echo "[WASM]   git clone https://github.com/emscripten-core/emsdk.git" >&2
    echo "[WASM]   cd emsdk && ./emsdk install latest && ./emsdk activate latest" >&2
    echo "[WASM]   source ./emsdk_env.sh" >&2
    return 1
}

# ---- 处理 npm 打包 ----
post_process_npm() {
    local build_dir="${SCRIPT_DIR}/cross-build/browser-${ARCH}"

    if [[ ! -d "${build_dir}" ]]; then
        echo "[NPM] 未找到构建产物目录: ${build_dir}"
        return 1
    fi

    echo "[NPM] 收集 npm 包..."
    local npm_out="${SCRIPT_DIR}/cross-build/npm-packages"
    mkdir -p "${npm_out}"

    for pkg_dir in "${build_dir}"/*/output; do
        if [[ -d "${pkg_dir}" ]]; then
            # 查找 npm 包
            for tgz in "${pkg_dir}"/*.tgz; do
                if [[ -f "${tgz}" ]]; then
                    cp "${tgz}" "${npm_out}/"
                    echo "[NPM] 复制: $(basename "${tgz}")"
                fi
            done
        fi
    done

    echo "[NPM] 全部包已收集到: ${npm_out}"
    ls -lh "${npm_out}" 2>/dev/null || true

    # 生成 package.json registry
    cat > "${npm_out}/registry.json" <<-JSONEOF
{
  "name": "glibc-packages-wasm-registry",
  "version": "1.0.0",
  "packages": [
JSONEOF

    first=true
    for f in "${npm_out}"/*.tgz; do
        if [[ -f "${f}" ]]; then
            local name="${f%.tgz}"
            name="$(basename "${name}")"
            if [[ "$first" == true ]]; then
                echo "    {\"name\": \"${name}\", \"url\": \"./$(basename "${f}")\"}" >> "${npm_out}/registry.json"
                first=false
            else
                echo "    ,{\"name\": \"${name}\", \"url\": \"./$(basename "${f}")\"}" >> "${npm_out}/registry.json"
            fi
        fi
    done

    echo "  ]" >> "${npm_out}/registry.json"
    echo "}" >> "${npm_out}/registry.json"
}

# ---- 启动测试服务器 ----
start_test_server() {
    local port="${DO_SERVE:-8080}"
    local wasm_dir="${SCRIPT_DIR}/cross-build/browser-${ARCH}"

    echo "[WASM] 启动测试服务器: http://localhost:${port}"
    echo "[WASM] WASM 产物目录: ${wasm_dir}"

    if command -v python3 &>/dev/null; then
        cd "${SCRIPT_DIR}"
        python3 -m http.server "${port}" --bind 0.0.0.0 &
        local pid=$!
        echo "[WASM] 服务器 PID: ${pid}"
        echo "[WASM] 打开浏览器访问: http://localhost:${port}/test/browser-test.html"
        echo "[WASM] 按 Ctrl+C 停止服务器"
        wait $pid
    elif command -v python &>/dev/null; then
        cd "${SCRIPT_DIR}"
        python -m SimpleHTTPServer "${port}" &
        wait $!
    else
        echo "[WASM] 错误: 未找到 python3" >&2
        return 1
    fi
}

# ---- 执行 WASM 模块测试 ----
run_wasm_tests() {
    echo "[WASM] 运行 WASM 模块测试..."

    if ! command -v node &>/dev/null; then
        echo "[WASM] 错误: 需要 Node.js 运行测试" >&2
        return 1
    fi

    local test_script="${SCRIPT_DIR}/scripts/tests/test-wasm.js"
    if [[ -f "${test_script}" ]]; then
        node "${test_script}"
    else
        echo "[WASM] 未找到测试脚本: ${test_script}"
        echo "[WASM] 请创建测试目录: scripts/tests/"
    fi
}

# ============================================================================
# 主流程
# ============================================================================

echo "==============================================================================="
echo " WASM 浏览器平台构建"
echo " 目标架构: ${ARCH}"
echo " 包列表:   ${PACKAGES[*]}"
echo "==============================================================================="
echo

# 1. 检查 Emscripten
check_emscripten || {
    echo "错误: Emscripten SDK 不可用，请先安装" >&2
    exit 1
}

# 2. 构建参数
EXTRA_ARGS=()
[[ "${CLEAN}" == "true" ]] && EXTRA_ARGS+=("--clean")
[[ -n "${JOBS}" ]] && EXTRA_ARGS+=("--jobs" "${JOBS}")
[[ "${QUIET}" == "true" ]] && EXTRA_ARGS+=("--quiet")
[[ "${FAIL_FAST}" == "true" ]] && EXTRA_ARGS+=("--fail-fast")

# 3. 执行 cross-build 构建
echo "--- 开始 Emscripten WASM 编译 ---"
"${SCRIPT_DIR}/build-cross.sh" \
    --platform browser \
    --arch "${ARCH}" \
    --toolchain emscripten \
    "${EXTRA_ARGS[@]}" \
    "${PACKAGES[@]}"

CROSS_EXIT=$?

# 4. 后处理
if [[ ${CROSS_EXIT} -eq 0 ]]; then
    echo
    echo "--- WASM 构建成功 ---"

    # npm 打包
    if [[ "${DO_NPM}" == "true" ]]; then
        post_process_npm
    fi

    # 测试
    if [[ "${DO_TEST}" == "true" ]]; then
        run_wasm_tests
    fi

    # 启动服务器
    if [[ -n "${DO_SERVE}" ]]; then
        start_test_server
    fi
else
    echo "错误: WASM 构建失败 (退出码 ${CROSS_EXIT})" >&2
    exit ${CROSS_EXIT}
fi

echo
echo "==============================================================================="
echo " WASM 构建完成"
echo " 产物目录: ${SCRIPT_DIR}/cross-build/browser-${ARCH}"
echo " 使用方法:"
echo "   1. 查看产物: ls cross-build/browser-${ARCH}/"
echo "   2. 测试:     node -e \"const mod=require('./zlib.js'); mod._compress(...)\""
echo "   3. 浏览器:   将 .wasm 和 .js 文件部署到 Web 服务器"
echo "==============================================================================="
