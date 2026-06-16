#!/bin/bash
# ai-tp-pow build script for glibc-packages
set -e

PKG_NAME="ai-tp-pow"
PKG_VERSION="1.0.0"
PKG_DIR="$(cd "$(dirname "$0")/../.." && pwd)/${PKG_NAME}"

echo "=== Building ${PKG_NAME} v${PKG_VERSION} ==="

check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "Error: $1 not found. Please install $2."
        exit 1
    fi
}

check_dep gcc "GCC compiler"
check_dep make "GNU Make"
check_dep ar   "binutils"

if [ ! -d "${PKG_DIR}" ]; then
    echo "Error: ${PKG_DIR} not found."
    exit 1
fi

cd "${PKG_DIR}"
make clean 2>/dev/null || true
make all

if [ -f libaitp-pow.a ]; then
    echo "  Build OK: libaitp-pow.a"
else
    echo "  Build FAILED"
    exit 1
fi

echo "  Running tests..."
if make test; then
    echo "  Tests PASSED"
else
    echo "  Tests FAILED"
    exit 1
fi

echo "=== ${PKG_NAME} v${PKG_VERSION} build complete ==="

