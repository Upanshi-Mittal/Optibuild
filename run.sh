#!/bin/bash
set -e

cmake -S . -B build
cmake --build build

echo "=== OptiBuild Scan ==="
./build/optibuild scan --project .
