# Builds the Release argon.exe and argon_debug.exe and zips them with the Visual C++ runtime DLLs they link against,
# MIT KEMAR's tables (hrtf_kemar.bin, made beforehand by tools\assets\convert_kemar.py), the third-party license notices
# and credits (THIRD_PARTY_LICENSES.txt), and empty game_data and fonts folders, each
# holding a README.txt that says what the player puts there.
# Output: dist\argon-win64.zip   (no game files or fonts: the player supplies their own)
# vulkan-1.dll is not packed: the graphics driver installs the Vulkan loader.
$ErrorActionPreference = 'Stop'

$BuildFolder = Join-Path $PSScriptRoot 'build'
$Executables = @('argon.exe', 'argon_debug.exe')
# MIT KEMAR's tables (tools\assets\convert_kemar.py); argon reads them beside the executable
$KemarTables = Join-Path $PSScriptRoot 'hrtf_kemar.bin'
$DistFolder = Join-Path $PSScriptRoot 'dist'
$StagingFolder = Join-Path $DistFolder 'argon'
$ZipPath = Join-Path $DistFolder 'argon-win64.zip'

function Find-VisualStudio {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found at $vswhere - is Visual Studio 2022 installed?" }
    $installation = & $vswhere -version '[17.0,18.0)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -latest -property installationPath
    if (-not $installation) { throw 'No Visual Studio 2022 with the C++ workload found by vswhere' }
    return $installation
}

$VisualStudio = Find-VisualStudio

function Find-Newest([string]$Folder, [string]$Filter) {
    $found = Get-ChildItem $Folder -Recurse -Filter $Filter | Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $found) { throw "No $Filter under $Folder - is the Visual Studio 2022 C++ workload installed?" }
    return $found.FullName
}

function Get-DllDependencies([string]$Binary) {
    $report = & (Find-Newest "$VisualStudio\VC\Tools\MSVC\*\bin\Hostx64\x64" 'dumpbin.exe') /nologo /dependents $Binary
    if ($LASTEXITCODE -ne 0) { throw "dumpbin could not read $Binary" }
    return $report | Where-Object { $_ -match '^\s+\S+\.dll$' } | ForEach-Object { $_.Trim() }
}

function Find-RuntimeFolder {
    $crt = Get-ChildItem "$VisualStudio\VC\Redist\MSVC\*\x64\Microsoft.VC*.CRT" -Directory | Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $crt) { throw "No Visual C++ runtime redistributable under $VisualStudio\VC\Redist\MSVC" }
    return $crt.FullName
}

function Invoke-ReleaseBuild {
    $environment = Join-Path $PSScriptRoot 'tools\visual_studio_env.bat'
    $configure = "cmake -S `"$PSScriptRoot`" -B `"$BuildFolder`" -G Ninja -DCMAKE_BUILD_TYPE=Release -DARGON_DEVELOP=ON"
    $build = "cmake --build `"$BuildFolder`" --target argon argon_debug"
    # CMake writes warnings to stderr; the exit code says whether the build failed
    $ErrorActionPreference = 'Continue'
    & cmd /c "call `"$environment`" && $configure && $build"
    if ($LASTEXITCODE -ne 0) { throw 'The Release build of argon and argon_debug failed' }
}

# The libraries linked into argon.exe (CMakeLists.txt FetchContent) and their license files in build\_deps
$ThirdPartyLicenses = @(
    @{ Name = 'GLFW 3.4';               File = 'glfw-src\LICENSE.md' },
    @{ Name = 'Dear ImGui 1.91.9';      File = 'imgui-src\LICENSE.txt' },
    @{ Name = 'glm 1.0.1';              File = 'glm-src\copying.txt' },
    @{ Name = 'nlohmann/json 3.11.3';   File = 'json-src\LICENSE.MIT' },
    @{ Name = 'miniaudio 0.11.21';      File = 'miniaudio-src\LICENSE' },
    @{ Name = 'dr_flac, dr_mp3';        File = 'dr_libs-src\LICENSE' },
    @{ Name = 'stb_vorbis';             File = 'stb-src\LICENSE' },
    @{ Name = 'libogg 1.3.6';           File = 'ogg-src\COPYING' },
    @{ Name = 'libtheora 1.2.0';        File = 'theora-src\COPYING' },
    @{ Name = 'libtheora 1.2.0 (On2)';  File = 'theora-src\LICENSE' },
    @{ Name = 'SMAA';                   File = 'smaa-src\LICENSE.txt' }
)

$KemarCredit = @'
hrtf_kemar.bin is converted from the MIT Media Lab's KEMAR HRTF measurements, copyright 1994 MIT Media Laboratory,
provided free with no restrictions on use provided the authors are cited:

    Bill Gardner and Keith Martin, "HRTF Measurements of a KEMAR Dummy-Head Microphone",
    MIT Media Lab Perceptual Computing Technical Report #280, 1994.
'@

function Write-ThirdPartyLicenses([string]$Destination) {
    $depsFolder = Join-Path $BuildFolder '_deps'
    $rule = '=' * 79
    $notice = "argon.exe and argon_debug.exe include the following third-party libraries.`r`n"
    foreach ($library in $ThirdPartyLicenses) {
        $licenseFile = Join-Path $depsFolder $library.File
        if (-not (Test-Path $licenseFile)) { throw "No license file for $($library.Name) at $licenseFile - did the build fetch it?" }
        $notice += "`r`n$rule`r`n$($library.Name)`r`n$rule`r`n`r`n" + (Get-Content $licenseFile -Raw).TrimEnd() + "`r`n"
    }
    $notice += "`r`n$rule`r`nMIT KEMAR HRTF measurements (hrtf_kemar.bin)`r`n$rule`r`n`r`n" + ($KemarCredit -replace "`r?`n", "`r`n") + "`r`n"
    Set-Content -Path $Destination -Value $notice -Encoding utf8 -NoNewline
}

$GameDataReadme = @'
Put the contents of the Quantum Redshift (USA) disc in this folder, so that it holds
data, config, audio, Locale, media, default.xbe, ... directly (game_data\data\tracks must exist).

argon finds this folder by itself and writes its profiles, ghosts and records to save_data
next to it. To keep the game files somewhere else, start argon with --game <folder>.
'@

$FontsReadme = @'
For the text to look sharp, put the game's original fonts in this folder, in .otf format.
Both are distributed with Windows. The file names must be:

    MS PGothic.otf
    Square 721 Extended Bold.otf

Without them argon uses the game's own bitmap fonts.
'@

function Write-FolderReadme([string]$Folder, [string]$Text) {
    New-Item -ItemType Directory -Force $Folder | Out-Null
    Set-Content -Path (Join-Path $Folder 'README.txt') -Value ($Text -replace "`r?`n", "`r`n") -Encoding utf8
}

if (-not (Test-Path $KemarTables)) { throw "No $KemarTables - make it with tools\assets\convert_kemar.py (docs\argon\build.md)" }
Invoke-ReleaseBuild

if (Test-Path $StagingFolder) { Remove-Item $StagingFolder -Recurse -Force }
New-Item -ItemType Directory -Force $StagingFolder | Out-Null

$runtimeFolder = Find-RuntimeFolder
foreach ($executable in $Executables) {
    $binary = Join-Path $BuildFolder $executable
    Copy-Item $binary $StagingFolder
    foreach ($dll in Get-DllDependencies $binary) {
        $runtimeDll = Join-Path $runtimeFolder $dll
        if (Test-Path $runtimeDll) { Copy-Item $runtimeDll $StagingFolder -Force }
    }
}
Copy-Item $KemarTables $StagingFolder
Write-ThirdPartyLicenses (Join-Path $StagingFolder 'THIRD_PARTY_LICENSES.txt')
Write-FolderReadme (Join-Path $StagingFolder 'game_data') $GameDataReadme
Write-FolderReadme (Join-Path $StagingFolder 'fonts') $FontsReadme

if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Compress-Archive -Path "$StagingFolder\*" -DestinationPath $ZipPath
Remove-Item $StagingFolder -Recurse -Force

Get-ChildItem $ZipPath | Select-Object FullName, Length
