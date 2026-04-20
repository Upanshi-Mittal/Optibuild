#!/bin/bash

cd build || exit

./graph_builder
./impact_analyzer
./build_optimizer