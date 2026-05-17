param(
    [string]$Mode = "",
    [switch]$NoBuild
)

$config = Join-Path $PSScriptRoot "dos-prepare.local.ps1"

if (!(Test-Path $config)) {
    Write-Error "Missing dos-prepare.local.ps1"
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

  # Mirror only what the DOS/Open Watcom build needs (see build.bat).
  robocopy $source $destination /MIR `
    /XD .git tests docs .github .cursor .vscode `
    /XF dosmud dosmud.exe Makefile *.output *.o *.obj

  & "$dospath$dosexecutable" `
    -c "mount c $mountpoint" `
    -c "c:" `
    -c "cd $projectdirectory" `
    -c "call build.bat$buildArgs" `
    -c "$projectname.exe"
} else {
  if (!(Test-Path $destination)) {
    Write-Error "Missing prepared DOS tree at $destination. Run make dos-prepare first."
    exit 1
  }

  if (!(Test-Path (Join-Path $destination "$projectname.exe"))) {
    Write-Error "Missing DOS executable at $destination\$projectname.exe. Run make dos-prepare first."
    exit 1
  }

  & "$dospath$dosexecutable" `
    -c "mount c $mountpoint" `
    -c "c:" `
    -c "cd $projectdirectory" `
    -c "$projectname.exe"
}
