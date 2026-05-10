#!/bin/bash

echo "Building Bank Crypto Project..."
echo

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "CMake configuration failed"
    exit 1
fi

cmake --build . --config Release -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

echo
echo "Build successful!"
echo "Executable: build/bank_demo"
