#!/usr/bin/env sh
# Builds argon (build.sh) and runs its tests with CTest (test.bat on Windows).
set -e
cd "$(dirname "$0")"
./build.sh
cd build
ctest --output-on-failure
