#!/usr/bin/env sh

set -e
set -x

cd "$(dirname ${0})"
cmake -S . -B build-linux
cmake --build build-linux --parallel=$(nproc --ignore=4)
