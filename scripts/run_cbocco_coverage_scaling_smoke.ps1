param(
    [int]$Func = 1,
    [int[]]$Seeds = @(1, 3),
    [int[]]$DimensionLimits = @(10, 20, 50),
    [int]$Contexts = 3,
    [int]$MaxFes = 1000,
    [string]$Config = "",
    [string]$OutputRoot = "",
    [string]$BuildDir = "",
    [string]$CMakeExe = "",
    [string]$PythonExe = ""
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build\flyki_phase7h"
}
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $RepoRoot "results\phase7h_coverage_scaling"
}
if (-not $Config) {
    $Config = Join-Path $RepoRoot "configs\cwvig_presets\capped_default.json"
}
$BenchmarkDir = Join-Path $RepoRoot "benchmark\flyki_overlap"
$ExpectedDimension = 905

function Resolve-Tool([string]$Preferred, [string]$Name) {
    if ($Preferred) {
        return $Preferred
    }
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    throw "Required tool not found: $Name"
}

function Check-Exit([string]$Step) {
    if ($LASTEXITCODE -ne 0) {
        throw "$Step failed with exit code $LASTEXITCODE"
    }
}

function Find-BuiltExe([string]$Name) {
    foreach ($candidate in @(
        (Join-Path $BuildDir "$Name.exe"),
        (Join-Path $BuildDir "Release\$Name.exe"),
        (Join-Path $BuildDir "Debug\$Name.exe")
    )) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }
    throw "Built executable not found: $Name"
}

function Remove-CBOCCTemps() {
    Remove-Item -Force -ErrorAction SilentlyContinue `
        "*.CBOG-CBD.result.txt", `
        "CBCCOtimefile.txt", `
        "errcmaes.err", `
        "actparcmaes.par", `
        "*_cmaes_initials.par", `
        "*_resume.par"
}

function Invoke-CBOCCRun([string]$RunDir, [string[]]$Arguments) {
    New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
    Push-Location $BenchmarkDir
    try {
        Remove-CBOCCTemps
        & $CboccoExe @Arguments > (Join-Path $RunDir "cbocco_stdout.txt") 2> (Join-Path $RunDir "cbocco_stderr.txt")
        $code = $LASTEXITCODE
        Set-Content -Path (Join-Path $RunDir "exit_code.txt") -Value $code -Encoding UTF8
        $resultFile = Get-ChildItem -File -Filter "*.CBOG-CBD.result.txt" | Select-Object -First 1
        if ($resultFile) {
            Move-Item -Force $resultFile.FullName (Join-Path $RunDir $resultFile.Name)
        }
        if (Test-Path "CBCCOtimefile.txt") {
            Move-Item -Force "CBCCOtimefile.txt" (Join-Path $RunDir "CBCCOtimefile.txt")
        }
        if (Test-Path "errcmaes.err") {
            Move-Item -Force "errcmaes.err" (Join-Path $RunDir "errcmaes.err")
        }
        Remove-CBOCCTemps
        if ($code -ne 0) {
            Write-Warning "cbocco failed with exit code $code. See $RunDir"
        }
    }
    finally {
        Pop-Location
    }
}

$CMakeExe = Resolve-Tool $CMakeExe "cmake"
$PythonArgs = @()
if ($PythonExe) {
    $PythonProgram = $PythonExe
} else {
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        $PythonProgram = $python.Source
    } else {
        $py = Get-Command py -ErrorAction SilentlyContinue
        if (-not $py) {
            throw "Required tool not found: python"
        }
        $PythonProgram = $py.Source
        $PythonArgs = @("-3")
    }
}

$configureArgs = @(
    "-S", (Join-Path $RepoRoot "benchmark\flyki_overlap"),
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAES_ROOT=$(Join-Path $RepoRoot "third_party\cmaes")"
)
if (-not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        $configureArgs += @("-G", "Ninja")
    }
    $gcc = Get-Command gcc -ErrorAction SilentlyContinue
    $gxx = Get-Command g++ -ErrorAction SilentlyContinue
    if ($gcc -and $gxx) {
        $configureArgs += @("-DCMAKE_C_COMPILER=$($gcc.Source)", "-DCMAKE_CXX_COMPILER=$($gxx.Source)")
    }
}

& $CMakeExe @configureArgs
Check-Exit "cmake configure"
foreach ($target in @(
    "cbocco",
    "cwvig_grouping_pipeline_cli",
    "grouping_coverage_cli",
    "grouping_source_cli",
    "hard_overlap_audit_cli",
    "hard_overlap_sanitize_cli"
)) {
    & $CMakeExe --build $BuildDir --target $target --config Release
    Check-Exit "cmake build $target"
}

$CboccoExe = Find-BuiltExe "cbocco"
$PipelineExe = Find-BuiltExe "cwvig_grouping_pipeline_cli"
$CoverageExe = Find-BuiltExe "grouping_coverage_cli"
$SourceExe = Find-BuiltExe "grouping_source_cli"
$AuditExe = Find-BuiltExe "hard_overlap_audit_cli"
$SanitizeExe = Find-BuiltExe "hard_overlap_sanitize_cli"

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $OutputRoot
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

foreach ($Seed in $Seeds) {
    $SeedDir = Join-Path $OutputRoot "seed_$Seed"
    New-Item -ItemType Directory -Force -Path $SeedDir | Out-Null

    Invoke-CBOCCRun `
        -RunDir (Join-Path $SeedDir "legacy") `
        -Arguments @("$Func", "CBCCO", "$Seed", "$MaxFes", "--grouping-source", "legacy_by_function", "--root", ".", "--require-full-coverage", "true", "--require-hard-overlap-compatible", "true")

    foreach ($DimensionLimit in $DimensionLimits) {
        $RunDir = Join-Path $SeedDir "dim_$DimensionLimit"
        $PredictedDir = Join-Path $RunDir "predicted"
        $CompletedPo = Join-Path $RunDir "completed_groups.txt"
        $CompletedOo = Join-Path $RunDir "completed_overlap.txt"
        $SanitizedPo = Join-Path $RunDir "sanitized_groups.txt"
        $SanitizedOo = Join-Path $RunDir "sanitized_overlap.txt"
        New-Item -ItemType Directory -Force -Path $RunDir | Out-Null

        & $PipelineExe `
            --config $Config `
            --mode flyki `
            --func $Func `
            --dimension-limit $DimensionLimit `
            --seed $Seed `
            --contexts $Contexts `
            --output-dir $PredictedDir > (Join-Path $RunDir "pipeline_stdout.txt") 2> (Join-Path $RunDir "pipeline_stderr.txt")
        Check-Exit "cwvig_grouping_pipeline_cli seed $Seed dim $DimensionLimit"

        & $CoverageExe `
            --po (Join-Path $PredictedDir "predicted_groups.txt") `
            --oo (Join-Path $PredictedDir "predicted_overlap.txt") `
            --expected-dimension $ExpectedDimension `
            --completion-policy singletons `
            --output-po $CompletedPo `
            --output-oo $CompletedOo `
            --print-summary > (Join-Path $RunDir "completion_summary.txt") 2> (Join-Path $RunDir "completion_stderr.txt")
        Check-Exit "grouping_coverage_cli seed $Seed dim $DimensionLimit"

        & $AuditExe `
            --po $CompletedPo `
            --oo $CompletedOo `
            --expected-dimension $ExpectedDimension `
            --output-report (Join-Path $RunDir "hard_overlap_before.csv") `
            --print-summary > (Join-Path $RunDir "hard_overlap_before.txt") 2> (Join-Path $RunDir "hard_overlap_before_stderr.txt")
        Check-Exit "hard_overlap_audit_cli before seed $Seed dim $DimensionLimit"

        & $SanitizeExe `
            --po $CompletedPo `
            --oo $CompletedOo `
            --expected-dimension $ExpectedDimension `
            --repair-policy demote_min_shared `
            --output-po $SanitizedPo `
            --output-oo $SanitizedOo `
            --output-report (Join-Path $RunDir "sanitizer_report.csv") `
            --print-summary > (Join-Path $RunDir "sanitize_summary.txt") 2> (Join-Path $RunDir "sanitize_stderr.txt")
        Check-Exit "hard_overlap_sanitize_cli seed $Seed dim $DimensionLimit"

        & $SourceExe `
            --source explicit_files `
            --po $SanitizedPo `
            --oo $SanitizedOo `
            --print-summary > (Join-Path $RunDir "sanitized_loader_summary.txt") 2> (Join-Path $RunDir "sanitized_loader_stderr.txt")
        Check-Exit "grouping_source_cli seed $Seed dim $DimensionLimit"

        & $AuditExe `
            --po $SanitizedPo `
            --oo $SanitizedOo `
            --expected-dimension $ExpectedDimension `
            --output-report (Join-Path $RunDir "hard_overlap_after.csv") `
            --print-summary > (Join-Path $RunDir "hard_overlap_after.txt") 2> (Join-Path $RunDir "hard_overlap_after_stderr.txt")
        Check-Exit "hard_overlap_audit_cli after seed $Seed dim $DimensionLimit"

        Invoke-CBOCCRun `
            -RunDir (Join-Path $RunDir "sanitized_cbocco") `
            -Arguments @("$Func", "CBCCO", "$Seed", "$MaxFes", "--grouping-source", "explicit_files", "--po", $SanitizedPo, "--oo", $SanitizedOo, "--require-full-coverage", "true", "--require-hard-overlap-compatible", "true")
    }
}

$SeedArgs = @()
foreach ($Seed in $Seeds) { $SeedArgs += "$Seed" }
$DimensionArgs = @()
foreach ($DimensionLimit in $DimensionLimits) { $DimensionArgs += "$DimensionLimit" }

& $PythonProgram @PythonArgs `
    (Join-Path $RepoRoot "experiments\analyze_cbocco_coverage_scaling.py") `
    --phase-dir $OutputRoot `
    --seeds @SeedArgs `
    --dimension-limits @DimensionArgs `
    --expected-dimension $ExpectedDimension `
    --summary-output (Join-Path $OutputRoot "coverage_scaling_summary.csv") `
    --by-dimension-output (Join-Path $OutputRoot "by_dimension_summary.csv") `
    --by-seed-output (Join-Path $OutputRoot "by_seed_summary.csv") `
    --report (Join-Path $OutputRoot "phase7h_report.md")
Check-Exit "analyze_cbocco_coverage_scaling.py"

Get-Content (Join-Path $OutputRoot "coverage_scaling_summary.csv")
Get-Content (Join-Path $OutputRoot "by_dimension_summary.csv")
