param(
    [string]$Mode = "",
    [switch]$NoBuild
)

$config = Join-Path $PSScriptRoot "prepare-dos.local.ps1"

if (!(Test-Path $config)) {
    Write-Error "Missing prepare-dos.local.ps1"
    exit 1
}

. $config

Get-Process $dosexecutable -ErrorAction SilentlyContinue | Stop-Process -Force

$buildArgs = if ($Mode) { " $Mode" } else { "" }

if (-not $NoBuild) {
  # Refresh the DOS tree before building.
  if (Test-Path $destination) {
    Remove-Item -Recurse -Force $destination
  }

  robocopy $source $destination    /MIR

  & "$dospath$dosexecutable" `
    -c "mount c $mountpoint" `
    -c "c:" `
    -c "cd $projectdirectory" `
    -c "call build.bat$buildArgs" `
    -c "$projectname.exe"
} else {
  if (!(Test-Path $destination)) {
    Write-Error "Missing prepared DOS tree at $destination. Run make prepare-dos first."
    exit 1
  }

  if (!(Test-Path (Join-Path $destination "$projectname.exe"))) {
    Write-Error "Missing DOS executable at $destination\$projectname.exe. Run make prepare-dos first."
    exit 1
  }

  & "$dospath$dosexecutable" `
    -c "mount c $mountpoint" `
    -c "c:" `
    -c "cd $projectdirectory" `
    -c "$projectname.exe"
}
