#!/bin/bash
set -e
if [ ! -d build ]; then
  mkdir build
fi
if [ ! -f CMakeCache.txt ]; then
  cmake -G Ninja -S . -B build \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_C_STANDARD=11 \
    -DCMAKE_C_STANDARD_REQUIRED=ON \
    -DCMAKE_C_EXTENSIONS=OFF \
    -DCMAKE_BUILD_TYPE=Release
fi
cmake --build build
