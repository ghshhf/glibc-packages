#!/bin/bash
set -e
cd "$(dirname "$0")/../.."
cd ai-tp-lan
make clean && make test
echo "=== ai-tp-lan: build & test OK ==="
