@echo off
rem Sets up the Visual Studio 2022 x64 compiler and puts its bundled CMake and Ninja on PATH, wherever Visual Studio is
rem installed (found with vswhere). Called by build.bat, test.bat and package.ps1; the caller keeps the environment.
set "VISUAL_STUDIO_LOCATOR=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VISUAL_STUDIO_LOCATOR%" (
    echo vswhere.exe not found at "%VISUAL_STUDIO_LOCATOR%" - is Visual Studio 2022 installed? 1>&2
    exit /b 1
)
set "VS="
for /f "usebackq delims=" %%i in (`"%VISUAL_STUDIO_LOCATOR%" -version [17.0^,18.0^) -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -latest -property installationPath`) do set "VS=%%i"
if not defined VS (
    echo No Visual Studio 2022 with the C++ workload found by vswhere 1>&2
    exit /b 1
)
rem vcvars64.bat runs vswhere.exe from PATH
for %%f in ("%VISUAL_STUDIO_LOCATOR%") do set "PATH=%%~dpf;%PATH%"
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "PATH=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
exit /b 0
