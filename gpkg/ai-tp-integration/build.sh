#!/bin/bash
# Build script for ai-tp-integration
set -e
echo "Building ai-tp-integration..."
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SRC_DIR="${PROJECT_DIR}/ai-tp-integration"
if [ ! -d "$SRC_DIR" ]; then
    echo "Error: Source directory not found: $SRC_DIR"
    exit 1
fi
cd "$SRC_DIR"
if command -v gcc &> /dev/null; then
    echo "Compiling with gcc..."
    gcc -Wall -Wextra -Iinclude -c src/ai-tp-integration.c -o ai-tp-integration.o
    ar rcs libai-tp-integration.a ai-tp-integration.o
    echo "Static library created: libai-tp-integration.a"
    echo "Running integration tests..."
    make test
else
    echo "Warning: gcc not found, skipping compilation"
fi
echo "ai-tp-integration build complete."
