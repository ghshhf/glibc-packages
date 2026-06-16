#!/bin/bash
PKG_NAME="ai-tp-sync"
PKG_VERSION="1.0.0"
PKG_DEPS="libai-storage ai-tp-discovery"

echo "Building $PKG_NAME $PKG_VERSION"

# 检查依赖
for dep in $PKG_DEPS; do
    if ! pkg-config --exists "$dep" 2>/dev/null; then
        echo "Warning: Dependency '$dep' not found"
    fi
done

# 编译
cd "$(dirname "$0")/../ai-tp-sync" || exit 1
make clean
make all

# 安装
if [ "$1" = "install" ]; then
    make install PREFIX="$2"
fi

echo "Build complete: $PKG_NAME"
exit 0
