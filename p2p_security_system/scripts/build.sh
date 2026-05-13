#!/bin/bash
set -e
mkdir -p bin obj logs
make clean
make all
echo "Build completed!"
