@echo off
rem Builds argon (the game alone) and argon_debug (the developer build) with the Visual Studio 2022 compiler, CMake and Ninja.
rem Output: build\argon.exe, build\argon_debug.exe   (first run downloads GLFW + Dear ImGui into build\_deps)
setlocal
call "%~dp0tools\visual_studio_env.bat" || exit /b 1
cd /d "%~dp0"
if not exist build\build.ninja cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DARGON_DEVELOP=ON || exit /b 1
cmake --build build || exit /b 1
echo Built %~dp0build\argon.exe and %~dp0build\argon_debug.exe
