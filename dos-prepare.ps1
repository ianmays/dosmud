param(
    [string]$Mode = "",
    [switch]$NoBuild,
    [switch]$NoRun,
    [string]$Seed = ""
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
    $roboArgs = @($Source, $Destination) + $ExtraArgs
    & robocopy @roboArgs
    if ($LASTEXITCODE -ge 8) {
        Write-Error "robocopy failed ($LASTEXITCODE): $Source -> $Destination"
        exit 1
    }
}

function Format-ElapsedSeconds {
    param([TimeSpan]$Duration)
    "{0}.{1:D3}s" -f [Math]::Floor($Duration.TotalSeconds), $Duration.Milliseconds
}

function Start-DosSession {
    param(
        [string[]]$Commands,
        [switch]$Wait
    )
    $dosboxArgs = @(
        '-c', "mount c $mountpoint",
        '-c', 'c:',
        '-c', "cd $projectdirectory"
    )
    foreach ($cmd in $Commands) {
        $dosboxArgs += @('-c', $cmd)
    }
    if ($Wait) {
        $quotedArgs = @()
        foreach ($arg in $dosboxArgs) {
            if ($arg -match '[\s"]') {
                $quotedArgs += '"' + ($arg -replace '"', '\"') + '"'
            } else {
                $quotedArgs += $arg
            }
        }
        $proc = Start-Process -FilePath "$dospath$dosexecutable" -ArgumentList ($quotedArgs -join ' ') -Wait -PassThru
        return $proc.ExitCode
    }
    & "$dospath$dosexecutable" @dosboxArgs
    return $LASTEXITCODE
}

Get-Process $dosexecutable -ErrorAction SilentlyContinue | Stop-Process -Force

$buildArgs = if ($Mode) { " $Mode" } else { "" }
$runCommand = if ($Seed) { "$projectname.exe --seed $Seed" } else { "$projectname.exe" }

if (-not $NoBuild) {
  # Refresh the DOS tree before building.
  Clear-DosDestination $destination
  New-Item -ItemType Directory -Path $destination -Force | Out-Null

  # Copy only build.bat inputs (never repo-root .git, tests, docs, or Linux artifacts).
  Invoke-RobocopyOk 'src' (Join-Path $source 'src') (Join-Path $destination 'src') '/E'
  Invoke-RobocopyOk 'include' (Join-Path $source 'include') (Join-Path $destination 'include') '/E'
  Invoke-RobocopyOk 'harness' (Join-Path $source 'tests\harness') (Join-Path $destination 'harness') '/E'
  Invoke-RobocopyOk 'build.bat' $source $destination 'build.bat'
  Remove-StaleDosMirrorExtras $destination

  $buildStarted = Get-Date
  Start-DosSession -Commands @("call build.bat$buildArgs", 'exit') -Wait | Out-Null
  $buildElapsed = (Get-Date) - $buildStarted
  $buildElapsedText = Format-ElapsedSeconds $buildElapsed
  $buildLog = Join-Path $destination 'build.log'
  $buildExe = Join-Path $destination "$projectname.exe"

  if (Test-Path -LiteralPath $buildLog) {
      Add-Content -LiteralPath $buildLog -Value "elapsed build.bat time: $buildElapsedText"
  }
  Write-Host "elapsed build.bat time: $buildElapsedText"

  if (!(Test-Path -LiteralPath $buildExe)) {
      Write-Error "Missing DOS executable at $buildExe after build.bat. DOS build failed."
      exit 1
  }

  if ((Test-Path -LiteralPath $buildLog) -and
      (-not (Select-String -LiteralPath $buildLog -Pattern 'wcl result: success ERRORLEVEL 0' -Quiet))) {
      Write-Error "DOS build log does not report success. See $buildLog."
      exit 1
  }

  if (-not $NoRun) {
      Start-DosSession @($runCommand)
  }
} else {
  if (!(Test-Path $destination)) {
    Write-Error "Missing prepared DOS tree at $destination. Run make dos-prepare first."
    exit 1
  }

  if (!(Test-Path (Join-Path $destination "$projectname.exe"))) {
    Write-Error "Missing DOS executable at $destination\$projectname.exe. Run make dos-prepare first."
    exit 1
  }

  Start-DosSession @($runCommand)
}
