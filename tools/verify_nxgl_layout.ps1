param(
    [int]$ExpectedProbeCount = 84,
    [int]$ExpectedAutorunVersion = 24
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$makefilePath = Join-Path $repo "Makefile"
$nxglMkPath = Join-Path $repo "nxgl.mk"
$validationMakefilePath = Join-Path $repo "validation\Makefile"
$autorunMakefilePath = Join-Path $repo "validation\autorun_suite\Makefile"
$helloMakefilePath = Join-Path $repo "examples\hello_triangle\Makefile"

function Add-Failure {
    param([string]$Message)
    $script:failures += $Message
}

function Get-MakefileList {
    param(
        [string]$Text,
        [string]$Name
    )

    $match = [regex]::Match($Text, "(?ms)^$([regex]::Escape($Name))\s*:=\s*\\\r?\n(?<body>.*?)(?:\r?\n\r?\n)")
    if (-not $match.Success) {
        throw "Could not find $Name list."
    }

    $items = @()
    foreach ($line in ($match.Groups["body"].Value -split "\r?\n")) {
        $item = ($line -replace "\\", "").Trim()
        if (-not [string]::IsNullOrWhiteSpace($item)) {
            $items += $item
        }
    }
    return $items
}

function Get-NxglMkPaths {
    param(
        [string]$Text,
        [string]$Name
    )

    $match = [regex]::Match($Text, "(?ms)^$([regex]::Escape($Name))\s*:=\s*\\\r?\n(?<body>.*?)(?:\r?\n\r?\n|$)")
    if (-not $match.Success) {
        throw "Could not find $Name in nxgl.mk."
    }

    $paths = @()
    foreach ($line in ($match.Groups["body"].Value -split "\r?\n")) {
        $item = ($line -replace "\\", "").Trim()
        if ([string]::IsNullOrWhiteSpace($item)) {
            continue
        }
        $paths += ($item -replace "\$\(NXGL_DIR\)/", "")
    }
    return $paths
}

function Get-ShaderSourcePath {
    param([string]$InlPath)

    $withoutExt = [regex]::Replace($InlPath, "\.inl$", "")
    $dir = Split-Path $withoutExt -Parent
    $leaf = Split-Path $withoutExt -Leaf

    if ($leaf -match "_vs$" -or $leaf -eq "vs") {
        return Join-Path $dir "$leaf.vs.cg"
    }
    return Join-Path $dir "$leaf.ps.cg"
}

$failures = @()

foreach ($required in @(
    "README.md",
    "LICENSE",
    "include\nxgl.h",
    "src\nxgl.c",
    "src\backend\nxgl_backend.c",
    "src\backend\nxgl_backend.h",
    "nxgl.mk",
    "examples\hello_triangle\main.c",
    "examples\hello_triangle\Makefile",
    "validation\Makefile",
    "validation\autorun_suite\main.c",
    "validation\autorun_suite\Makefile"
)) {
    if (-not (Test-Path (Join-Path $repo $required))) {
        Add-Failure "Missing required file: $required"
    }
}

$rootMakefile = Get-Content -LiteralPath $makefilePath -Raw
foreach ($target in @("example", "validation", "validation-autorun", "print-vars")) {
    if ($rootMakefile -notmatch "(?m)^$([regex]::Escape($target))\s*:") {
        Add-Failure "Root Makefile missing target: $target"
    }
}

$nxglMk = Get-Content -LiteralPath $nxglMkPath -Raw
$nxglSrcs = Get-NxglMkPaths $nxglMk "NXGL_SRCS"
$nxglShaders = Get-NxglMkPaths $nxglMk "NXGL_SHADER_OBJS"
foreach ($path in $nxglSrcs) {
    if (-not (Test-Path (Join-Path $repo $path))) {
        Add-Failure "nxgl.mk references missing source: $path"
    }
}
foreach ($shader in $nxglShaders) {
    $source = Get-ShaderSourcePath $shader
    if (-not (Test-Path (Join-Path $repo $source))) {
        Add-Failure "nxgl.mk shader output has no source: $shader -> $source"
    }
}

$helloMakefile = Get-Content -LiteralPath $helloMakefilePath -Raw
foreach ($needle in @('include $(NXGL_DIR)/nxgl.mk', '$(NXGL_SRCS)', '$(NXGL_SHADER_OBJS)', '$(NXGL_CFLAGS)')) {
    if (-not $helloMakefile.Contains($needle)) {
        Add-Failure "hello_triangle Makefile missing consumer pattern: $needle"
    }
}

$validationMakefile = Get-Content -LiteralPath $validationMakefilePath -Raw
$validationProbes = @(Get-MakefileList $validationMakefile "VALIDATION_PROBES")
if ($validationProbes.Count -ne $ExpectedProbeCount) {
    Add-Failure "VALIDATION_PROBES has $($validationProbes.Count) entries, expected $ExpectedProbeCount."
}

foreach ($probe in $validationProbes) {
    $probeDir = Join-Path $repo (Join-Path "validation" $probe)
    if (-not (Test-Path $probeDir)) {
        Add-Failure "Missing validation probe directory: $probe"
        continue
    }
    foreach ($requiredFile in @("Makefile", "main.c")) {
        if (-not (Test-Path (Join-Path $probeDir $requiredFile))) {
            Add-Failure "Missing $requiredFile in validation probe: $probe"
        }
    }
}

$actualProbeDirs = @(Get-ChildItem -LiteralPath (Join-Path $repo "validation") -Directory |
    Where-Object { $_.Name -match "^\d+_gl_.*_probe$" } |
    ForEach-Object { $_.Name })
foreach ($probeDir in $actualProbeDirs) {
    if ($validationProbes -notcontains $probeDir) {
        Add-Failure "Validation probe directory is not listed in validation/Makefile: $probeDir"
    }
}

$autorunMakefile = Get-Content -LiteralPath $autorunMakefilePath -Raw
if ($autorunMakefile -notmatch "(?m)^AUTORUN_VERSION\s*=\s*(\d+)") {
    Add-Failure "autorun_suite Makefile missing AUTORUN_VERSION."
} elseif ([int]$matches[1] -ne $ExpectedAutorunVersion) {
    Add-Failure "AUTORUN_VERSION is $($matches[1]), expected $ExpectedAutorunVersion."
}
foreach ($needle in @('$(NXGL_SRCS)', '$(NXGL_SHADER_OBJS)', 'generate_nxgl_autorun_suite.sh')) {
    if (-not $autorunMakefile.Contains($needle)) {
        Add-Failure "autorun_suite Makefile missing expected integration: $needle"
    }
}

if ($failures.Count -gt 0) {
    throw "NXGL layout verification failed:`n$($failures -join "`n")"
}

Write-Host "Verified NXGL layout: $($validationProbes.Count) validation probes, example consumer, autorun v$ExpectedAutorunVersion."
