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

function Clear-DosDestination {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }
    Remove-Item -LiteralPath $Path -Recurse -Force
    if (Test-Path -LiteralPath $Path) {
        Write-Error "Could not remove existing DOS tree at $Path (close DOSBox/files using it and retry)."
        exit 1
    }
}

# Strip dirs/files that must never live in the Windows DOS tree (e.g. leftover .git from old mirrors).
function Remove-StaleDosMirrorExtras {
    param([string]$Root)
    foreach ($name in @('.git', 'tests', 'docs', '.github', '.cursor', '.vscode')) {
        $p = Join-Path $Root $name
        if (Test-Path -LiteralPath $p) {
            Remove-Item -LiteralPath $p -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    foreach ($leaf in @('dosmud', 'dosmud.exe', 'dosmud_unit', 'Makefile')) {
        $p = Join-Path $Root $leaf
        if (Test-Path -LiteralPath $p) {
            Remove-Item -LiteralPath $p -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

function Invoke-RobocopyOk {
    param(
        [string]$Label,
        [string]$Source,
        [string]$Destination,
        [string[]]$ExtraArgs = @()
    )
    Write-Host ""
    Write-Host "robocopy ($Label): $Source -> $Destination"
    $args = @($Source, $Destination) + $ExtraArgs
    & robocopy @args
    if ($LASTEXITCODE -ge 8) {
        Write-Error "robocopy failed ($LASTEXITCODE): $Source -> $Destination"
        exit 1
    }
}

Get-Process $dosexecutable -ErrorAction SilentlyContinue | Stop-Process -Force

$buildArgs = if ($Mode) { " $Mode" } else { "" }

if (-not $NoBuild) {
  # Refresh the DOS tree before building.
  Clear-DosDestination $destination
  New-Item -ItemType Directory -Path $destination -Force | Out-Null

  # Copy only build.bat inputs (never repo-root .git, tests, docs, or Linux artifacts).
  Invoke-RobocopyOk 'src' (Join-Path $source 'src') (Join-Path $destination 'src') '/E'
  Invoke-RobocopyOk 'include' (Join-Path $source 'include') (Join-Path $destination 'include') '/E'
  Invoke-RobocopyOk 'build.bat' $source $destination 'build.bat'
  Remove-StaleDosMirrorExtras $destination

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
