#!/bin/bash

cd build

echo "=== Building Graph ==="
./graph_builder

echo "=== Detect Changes ==="
./impact_analyzer

echo "=== Selective Build ==="
./build_optimizer