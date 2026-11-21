@echo off
rem Builds argon (build.bat) and runs its tests (CTest: build\argon_tests.exe).
setlocal
call "%~dp0build.bat" || exit /b 1
call "%~dp0tools\visual_studio_env.bat" || exit /b 1
cd /d "%~dp0build"
ctest --output-on-failure || exit /b 1
