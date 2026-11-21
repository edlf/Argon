# Start xemu with the GDB stub on tcp:1234, booting an Xbox ISO (never written to).
# xemu: the XEMU environment variable, the full path of the xemu.exe to run.
# -Wait: freeze the CPU until a debugger continues.
# usage: powershell -ExecutionPolicy Bypass -File run_xemu.ps1 -Iso <game.iso> [-Wait]
param([Parameter(Mandatory = $true)][string]$Iso, [switch]$Wait)

$xemu = $env:XEMU
if (-not $xemu) { Write-Error "Set the XEMU environment variable to the full path of xemu.exe"; exit 1 }
if (-not (Test-Path -LiteralPath $xemu -PathType Leaf)) { Write-Error "XEMU points at no file: $xemu"; exit 1 }
if (Get-Process xemu -ErrorAction SilentlyContinue) { Write-Error "xemu is already running - close it first (the ISO is in use)"; exit 1 }
if (-not (Test-Path -LiteralPath $Iso)) { Write-Error "ISO not found: $Iso"; exit 1 }

$xargs = @("-dvd_path", "`"$Iso`"", "-s")
if ($Wait) { $xargs += "-S" }
Start-Process -FilePath $xemu -ArgumentList $xargs -WorkingDirectory (Split-Path $xemu) | Out-Null
Write-Output "xemu started on $Iso (gdb on 127.0.0.1:1234)"
