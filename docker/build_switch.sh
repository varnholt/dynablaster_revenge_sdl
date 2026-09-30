#!/bin/bash
set -euo pipefail
cmake -S client -B client/build-switch \
    -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build client/build-switch --parallel "${BUILD_JOBS:-$(nproc)}"
ls -lh client/build-switch/dynablaster_revenge.nro
