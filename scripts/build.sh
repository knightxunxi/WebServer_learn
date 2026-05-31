#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCSL_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"

