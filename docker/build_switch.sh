#!/bin/bash
set -euo pipefail
# GCC 14 avoids the GCS instructions emitted by GCC 15's unwinder, which
# current homebrew emulators cannot execute. This SDK ships an older CMake.
bootstrap=client/build-switch/cmake-bootstrap
cmake_version=3.31.6
cmake_root="$bootstrap/cmake-$cmake_version-linux-x86_64"
if [ ! -x "$cmake_root/bin/cmake" ]; then
    mkdir -p "$bootstrap"
    curl --fail --location --retry 3 \
        "https://github.com/Kitware/CMake/releases/download/v$cmake_version/cmake-$cmake_version-linux-x86_64.tar.gz" \
        --output "$bootstrap/cmake.tar.gz"
    echo "5a1133ff103c71eb5120e2cc3de922733e7d8a26a98ae716397e8676adb367bf  $bootstrap/cmake.tar.gz" | sha256sum --check
    tar -xzf "$bootstrap/cmake.tar.gz" -C "$bootstrap"
fi
export PATH="$PWD/$cmake_root/bin:$PATH"
build_dir=client/build-switch/gcc14
cmake -S client -B "$build_dir" \
    -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --parallel "${BUILD_JOBS:-$(nproc)}"
ls -lh "$build_dir/dynablaster_revenge.nro"
