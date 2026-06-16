#!/bin/bash
# ai-tp-scheduler build script for glibc-packages
# AI-TP OS 计算层核心：AI 任务调度与资源管理

set -e

PKG_NAME="ai-tp-scheduler"
PKG_VERSION="1.0.0"
PKG_DIR="$(cd "$(dirname "$0")/../.." && pwd)/${PKG_NAME}"

echo "=== Building ${PKG_NAME} v${PKG_VERSION} ==="

# 检查依赖
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "Error: $1 not found. Please install $2."
        exit 1
    fi
}

check_dep gcc "GCC compiler"
check_dep make "GNU Make"
check_dep ar   "binutils"

# 检查 ai-tp-scheduler 源码
if [ ! -d "${PKG_DIR}" ]; then
    echo "Error: ${PKG_DIR} not found."
    exit 1
fi

# 检查头文件
if [ ! -f "${PKG_DIR}/include/ai-tp-scheduler.h" ]; then
    echo "Error: ai-tp-scheduler.h not found."
    exit 1
fi

echo "  Sources: ${PKG_DIR}"

# 编译
cd "${PKG_DIR}"
make clean 2>/dev/null || true
make all

# 验证
if [ -f libaitp-scheduler.a ]; then
    echo "  Build OK: libaitp-scheduler.a"
else
    echo "  Build FAILED"
    exit 1
fi

# 运行测试
echo "  Running tests..."
if make test; then
    echo "  Tests PASSED"
else
    echo "  Tests FAILED"
    exit 1
fi

echo "=== ${PKG_NAME} v${PKG_VERSION} build complete ==="
