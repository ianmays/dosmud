param(
    [string]$Mode = ""
)

$config = Join-Path $PSScriptRoot "prepare-dos.local.ps1"

if (!(Test-Path $config)) {
    Write-Error "Missing prepare-dos.local.ps1"
    exit 1
}

. $config

# Remove old copy
if (Test-Path $destination) {
    Remove-Item -Recurse -Force $destination
}

# Copy fresh
robocopy $source $destination    /MIR

Get-Process $dosexecutable -ErrorAction SilentlyContinue | Stop-Process -Force

$buildArgs = if ($Mode) { " $Mode" } else { "" }

& "$dospath$dosexecutable" `
  -c "mount c $mountpoint" `
  -c "c:" `
  -c "cd $projectdirectory" `
  -c "call build.bat$buildArgs" `
  -c "dosmud.exe"