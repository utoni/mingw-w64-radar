#!/usr/bin/env sh

if [ $# -ne 2 ]; then
    printf 'usage: %s\n' '[DPP-ROOT] [KMEM-ROOT]'
    exit 1
fi

set -e
set -x

cd "$(dirname ${0})"
cmake -S . -B build-examples-mingw \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_BUILD_TYPE=Release \
    -DGLFW_BUILD_X11=OFF \
    -DBUILD_DPP_KMEM_EXAMPLE=ON \
    -DDPP_ROOT="${1}" \
    -DKMEM_ROOT="${2}"
cmake --build build-examples-mingw --parallel=$(nproc --ignore=4)
