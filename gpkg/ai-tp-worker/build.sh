#!/bin/bash
set -e
cd "$(dirname "$0")/../.."
cd ai-tp-worker
make clean && make test
echo "=== ai-tp-worker: build & test OK ==="
