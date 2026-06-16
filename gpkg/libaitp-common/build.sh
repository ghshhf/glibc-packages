#!/bin/bash
set -e
cd "$(dirname "$0")/../.."
cd libaitp-common
make clean && make test
echo "=== libaitp-common: build & test OK ==="
