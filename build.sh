#!/usr/bin/env sh
# Builds argon (the game alone) and argon_debug (the developer build) on Linux with CMake and Ninja (build.bat on
# Windows).
# Output: build/argon, build/argon_debug   (first run downloads GLFW, Dear ImGui, glm, ... into build/_deps)
# Needs: a C++20 compiler, cmake, ninja, the Vulkan headers and loader, glslc (shaderc), and GLFW's X11 / Wayland
# development packages.
set -e
cd "$(dirname "$0")"
[ -f build/build.ninja ] || cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DARGON_DEVELOP=ON
cmake --build build
echo "Built $(pwd)/build/argon and $(pwd)/build/argon_debug"
