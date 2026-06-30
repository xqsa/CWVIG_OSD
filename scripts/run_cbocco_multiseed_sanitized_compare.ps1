param(
    [string]$BuildDir = "",
    [string]$CMakeExe = "",
    [string]$PythonExe = ""
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build\flyki_phase7g"
}
$PhaseDir = Join-Path $RepoRoot "results\phase7g"
$BenchmarkDir = Join-Path $RepoRoot "benchmark\flyki_overlap"

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
            return
        }
        if (-not (Get-ChildItem -Path $RunDir -File -Filter "*.CBOG-CBD.result.txt")) {
            Write-Warning "cbocco did not produce a result file. See $RunDir"
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

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $PhaseDir
New-Item -ItemType Directory -Force -Path $PhaseDir | Out-Null

foreach ($Seed in @(1, 3, 5)) {
    $SeedDir = Join-Path $PhaseDir "seed_$Seed"
    $GroupingDir = Join-Path $SeedDir "grouping"
    $AuditDir = Join-Path $SeedDir "audit"
    New-Item -ItemType Directory -Force -Path $GroupingDir, $AuditDir | Out-Null

    $PredictedDir = Join-Path $GroupingDir "predicted_10d"
    $CompletedPo = Join-Path $GroupingDir "completed_groups.txt"
    $CompletedOo = Join-Path $GroupingDir "completed_overlap.txt"
    $SanitizedPo = Join-Path $GroupingDir "sanitized_groups.txt"
    $SanitizedOo = Join-Path $GroupingDir "sanitized_overlap.txt"

    & $PipelineExe `
        --config (Join-Path $RepoRoot "configs\cwvig_presets\capped_default.json") `
        --mode flyki `
        --func 1 `
        --dimension-limit 10 `
        --seed $Seed `
        --contexts 3 `
        --output-dir $PredictedDir > (Join-Path $GroupingDir "pipeline_stdout.txt") 2> (Join-Path $GroupingDir "pipeline_stderr.txt")
    Check-Exit "cwvig_grouping_pipeline_cli seed $Seed"

    & $CoverageExe `
        --po (Join-Path $PredictedDir "predicted_groups.txt") `
        --oo (Join-Path $PredictedDir "predicted_overlap.txt") `
        --expected-dimension 905 `
        --completion-policy singletons `
        --output-po $CompletedPo `
        --output-oo $CompletedOo `
        --print-summary > (Join-Path $GroupingDir "completion_summary.txt") 2> (Join-Path $GroupingDir "completion_stderr.txt")
    Check-Exit "grouping_coverage_cli seed $Seed"

    & $AuditExe `
        --po $CompletedPo `
        --oo $CompletedOo `
        --expected-dimension 905 `
        --print-summary `
        --output-report (Join-Path $AuditDir "hard_overlap_audit_before.csv") > (Join-Path $AuditDir "hard_overlap_audit_before.txt") 2> (Join-Path $AuditDir "hard_overlap_audit_before_stderr.txt")
    Check-Exit "hard_overlap_audit_cli before seed $Seed"

    & $SanitizeExe `
        --po $CompletedPo `
        --oo $CompletedOo `
        --expected-dimension 905 `
        --repair-policy demote_min_shared `
        --output-po $SanitizedPo `
        --output-oo $SanitizedOo `
        --output-report (Join-Path $AuditDir "sanitizer_report.csv") `
        --print-summary > (Join-Path $AuditDir "sanitize_summary.txt") 2> (Join-Path $AuditDir "sanitize_stderr.txt")
    Check-Exit "hard_overlap_sanitize_cli seed $Seed"

    & $SourceExe `
        --source explicit_files `
        --po $SanitizedPo `
        --oo $SanitizedOo `
        --print-summary > (Join-Path $GroupingDir "sanitized_loader_summary.txt") 2> (Join-Path $GroupingDir "sanitized_loader_stderr.txt")
    Check-Exit "grouping_source_cli sanitized seed $Seed"

    & $AuditExe `
        --po $SanitizedPo `
        --oo $SanitizedOo `
        --expected-dimension 905 `
        --print-summary `
        --output-report (Join-Path $AuditDir "hard_overlap_audit_after.csv") > (Join-Path $AuditDir "hard_overlap_audit_after.txt") 2> (Join-Path $AuditDir "hard_overlap_audit_after_stderr.txt")
    Check-Exit "hard_overlap_audit_cli after seed $Seed"

    Invoke-CBOCCRun `
        -RunDir (Join-Path $SeedDir "legacy") `
        -Arguments @("1", "CBCCO", "$Seed", "1000", "--grouping-source", "legacy_by_function", "--root", ".", "--require-full-coverage", "true", "--require-hard-overlap-compatible", "true")

    Invoke-CBOCCRun `
        -RunDir (Join-Path $SeedDir "sanitized_completed_predicted") `
        -Arguments @("1", "CBCCO", "$Seed", "1000", "--grouping-source", "explicit_files", "--po", $SanitizedPo, "--oo", $SanitizedOo, "--require-full-coverage", "true", "--require-hard-overlap-compatible", "true")
}

& $PythonProgram @PythonArgs `
    (Join-Path $RepoRoot "experiments\analyze_cbocco_multiseed.py") `
    --phase-dir $PhaseDir `
    --seeds 1 3 5 `
    --fe-tolerance 0 `
    --candidate-dir sanitized_completed_predicted `
    --candidate-name sanitized_completed_predicted `
    --candidate-prefix sanitized `
    --include-sanitizer-metadata `
    --comparison-output (Join-Path $PhaseDir "sanitized_multiseed_comparison.csv") `
    --summary-output (Join-Path $PhaseDir "sanitized_multiseed_summary.csv") `
    --report (Join-Path $PhaseDir "phase7g_report.md") `
    --report-title "Phase 7G Sanitized Multi-Seed CBOCC Smoke Comparison"
Check-Exit "analyze_cbocco_multiseed.py"

Get-Content (Join-Path $PhaseDir "sanitized_multiseed_comparison.csv")
Get-Content (Join-Path $PhaseDir "sanitized_multiseed_summary.csv")
