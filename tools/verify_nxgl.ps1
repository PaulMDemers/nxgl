param(
    [string[]]$Targets = @("default"),
    [switch]$Clean,
    [string]$NxdkDir = ""
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$workspace = Split-Path -Parent $repo
$outDir = Join-Path $repo "dist\verification"
$logPath = Join-Path $outDir "nxgl_verify.log"
$summaryPath = Join-Path $outDir "nxgl_verify_summary.json"

function Convert-ToMsysPath {
    param([string]$Path)

    $normalized = $Path -replace "\\", "/"
    if ($normalized -match "^([A-Za-z]):/(.*)$") {
        return "/$($matches[1].ToLowerInvariant())/$($matches[2])"
    }
    return $normalized
}

function Find-Bash {
    $candidates = @(
        "C:\msys64\usr\bin\bash.exe",
        "C:\devkitPro\msys2\usr\bin\bash.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    throw "MSYS2 bash not found. Checked: $($candidates -join ', ')"
}

function Resolve-NxdkDir {
    param([string]$Override)

    if (-not [string]::IsNullOrWhiteSpace($Override)) {
        return (Resolve-Path $Override).Path
    }
    if (-not [string]::IsNullOrWhiteSpace($env:NXDK_DIR) -and (Test-Path $env:NXDK_DIR)) {
        return (Resolve-Path $env:NXDK_DIR).Path
    }

    $candidate = Join-Path $workspace ".nxdk"
    if (Test-Path $candidate) {
        return (Resolve-Path $candidate).Path
    }

    throw "NXDK_DIR not set and no sibling .nxdk checkout found at $candidate"
}

function Get-CommandsForTarget {
    param([string]$Target)

    switch ($Target.ToLowerInvariant()) {
        "default" {
            return @(
                "make",
                "make print-vars",
                "make example",
                "make validation-autorun",
                "make -C validation texture",
                "make -C validation lighting",
                "make -C validation raster"
            )
        }
        "root" { return @("make", "make print-vars") }
        "example" { return @("make example") }
        "autorun" { return @("make validation-autorun") }
        "texture" { return @("make -C validation texture") }
        "lighting" { return @("make -C validation lighting") }
        "raster" { return @("make -C validation raster") }
        "all-probes" { return @("make -C validation") }
        default { throw "Unknown verification target: $Target" }
    }
}

New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$layoutScript = Join-Path $PSScriptRoot "verify_nxgl_layout.ps1"
& $layoutScript

$bash = Find-Bash
$nxdkPath = Resolve-NxdkDir $NxdkDir
$repoMsys = Convert-ToMsysPath $repo.Path
$nxdkMsys = Convert-ToMsysPath $nxdkPath
$commands = New-Object "System.Collections.Generic.List[string]"

$expandedTargets = New-Object "System.Collections.Generic.List[string]"
foreach ($targetToken in $Targets) {
    foreach ($targetPart in ($targetToken -split ",")) {
        if (-not [string]::IsNullOrWhiteSpace($targetPart)) {
            $expandedTargets.Add($targetPart.Trim())
        }
    }
}
if ($expandedTargets.Count -eq 0) {
    $expandedTargets.Add("default")
}

if ($Clean) {
    $commands.Add("make -C validation clean")
}
foreach ($target in $expandedTargets) {
    foreach ($command in (Get-CommandsForTarget $target)) {
        $commands.Add($command)
    }
}

$scriptLines = @(
    "set -e",
    "cd '$repoMsys'",
    "export MSYSTEM=MINGW64",
    "export NXDK_DIR='$nxdkMsys'",
    "export PATH='$nxdkMsys/bin:$nxdkMsys/tools/cg/win:/mingw64/bin:/usr/bin:'`$PATH"
) + $commands

$script = $scriptLines -join "`n"
$started = Get-Date
$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    $output = & $bash -lc $script 2>&1
    $exitCode = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $previousErrorActionPreference
}
$finished = Get-Date

$output | Set-Content -LiteralPath $logPath -Encoding ASCII

$summary = [ordered]@{
    started_at = $started.ToString("o")
    finished_at = $finished.ToString("o")
    duration_seconds = [Math]::Round(($finished - $started).TotalSeconds, 2)
    repo = $repo.Path
    nxdk_dir = $nxdkPath
    targets = @($expandedTargets)
    clean = $Clean.IsPresent
    exit_code = $exitCode
    commands = @($commands)
    log = $logPath
}
$summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $summaryPath -Encoding ASCII

if ($exitCode -ne 0) {
    Write-Host ($output -join "`n")
    throw "NXGL verification failed. See $logPath"
}

Write-Host "NXGL verification passed. Summary: $summaryPath"
