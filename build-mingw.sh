#!/usr/bin/env sh

set -e
set -x

cd "$(dirname ${0})"
cmake -S . -B build-mingw \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_BUILD_TYPE=Release \
    -DGLFW_BUILD_X11=OFF
cmake --build build-mingw --parallel=$(nproc --ignore=4)
