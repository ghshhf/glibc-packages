#!/bin/bash
# Build script for ai-tp-address
set -e
echo "Building ai-tp-address..."
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SRC_DIR="${PROJECT_DIR}/ai-tp-address"
if [ ! -d "$SRC_DIR" ]; then
    echo "Error: Source directory not found: $SRC_DIR"
    exit 1
fi
cd "$SRC_DIR"
if command -v gcc &> /dev/null; then
    echo "Compiling with gcc..."
    gcc -Wall -Wextra -Iinclude -c src/ai-tp-address.c -o ai-tp-address.o
    ar rcs libai-tp-address.a ai-tp-address.o
    echo "Static library created: libai-tp-address.a"
    echo "Running tests..."
    make test
else
    echo "Warning: gcc not found, skipping compilation"
fi
echo "ai-tp-address build complete."
