param(
    [string]$BuildDir = "",
    [string]$CMakeExe = ""
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build\flyki_phase7f"
}
$PhaseDir = Join-Path $RepoRoot "results\phase7f_hard_overlap_sanitizer"
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

function Invoke-CBOCCRun([string]$RunDir, [string[]]$Arguments, [bool]$ExpectFailure) {
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
        if ($ExpectFailure) {
            if ($code -eq 0) {
                throw "cbocco was expected to fail but exited 0. See $RunDir"
            }
            return
        }
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

$PredictedDir = Join-Path $PhaseDir "predicted_10d"
$CompletedPo = Join-Path $PhaseDir "completed_predicted_groups.txt"
$CompletedOo = Join-Path $PhaseDir "completed_predicted_overlap.txt"
$RepairedPo = Join-Path $PhaseDir "repaired_groups.txt"
$RepairedOo = Join-Path $PhaseDir "repaired_overlap.txt"

& $PipelineExe `
    --config (Join-Path $RepoRoot "configs\cwvig_presets\capped_default.json") `
    --mode flyki `
    --func 1 `
    --dimension-limit 10 `
    --seed 3 `
    --contexts 3 `
    --output-dir $PredictedDir > (Join-Path $PhaseDir "pipeline_stdout.txt") 2> (Join-Path $PhaseDir "pipeline_stderr.txt")
Check-Exit "cwvig_grouping_pipeline_cli"

& $CoverageExe `
    --po (Join-Path $PredictedDir "predicted_groups.txt") `
    --oo (Join-Path $PredictedDir "predicted_overlap.txt") `
    --expected-dimension 905 `
    --completion-policy singletons `
    --output-po $CompletedPo `
    --output-oo $CompletedOo `
    --print-summary > (Join-Path $PhaseDir "completion_summary.txt") 2> (Join-Path $PhaseDir "completion_stderr.txt")
Check-Exit "grouping_coverage_cli"

& $AuditExe `
    --po $CompletedPo `
    --oo $CompletedOo `
    --expected-dimension 905 `
    --print-summary `
    --output-report (Join-Path $PhaseDir "completed_hard_overlap_audit.csv") > (Join-Path $PhaseDir "completed_hard_overlap_audit.txt") 2> (Join-Path $PhaseDir "completed_hard_overlap_audit_stderr.txt")
Check-Exit "hard_overlap_audit_cli before repair"

Invoke-CBOCCRun `
    -RunDir (Join-Path $PhaseDir "strict_incompatible") `
    -Arguments @("1", "CBCCO", "3", "1000", "--grouping-source", "explicit_files", "--po", $CompletedPo, "--oo", $CompletedOo, "--require-full-coverage", "true", "--require-hard-overlap-compatible", "true") `
    -ExpectFailure $true

& $SanitizeExe `
    --po $CompletedPo `
    --oo $CompletedOo `
    --expected-dimension 905 `
    --repair-policy demote_min_shared `
    --output-po $RepairedPo `
    --output-oo $RepairedOo `
    --output-report (Join-Path $PhaseDir "repair_actions.csv") `
    --print-summary > (Join-Path $PhaseDir "sanitize_summary.txt") 2> (Join-Path $PhaseDir "sanitize_stderr.txt")
Check-Exit "hard_overlap_sanitize_cli"

& $AuditExe `
    --po $RepairedPo `
    --oo $RepairedOo `
    --expected-dimension 905 `
    --print-summary `
    --output-report (Join-Path $PhaseDir "repaired_hard_overlap_audit.csv") > (Join-Path $PhaseDir "repaired_hard_overlap_audit.txt") 2> (Join-Path $PhaseDir "repaired_hard_overlap_audit_stderr.txt")
Check-Exit "hard_overlap_audit_cli after repair"

& $SourceExe `
    --source explicit_files `
    --po $RepairedPo `
    --oo $RepairedOo `
    --print-summary > (Join-Path $PhaseDir "repaired_loader_summary.txt") 2> (Join-Path $PhaseDir "repaired_loader_stderr.txt")
Check-Exit "grouping_source_cli repaired"

Invoke-CBOCCRun `
    -RunDir (Join-Path $PhaseDir "repaired_cbocco") `
    -Arguments @("1", "CBCCO", "3", "1000", "--grouping-source", "explicit_files", "--po", $RepairedPo, "--oo", $RepairedOo, "--require-full-coverage", "true", "--require-hard-overlap-compatible", "true") `
    -ExpectFailure $false

Get-Content (Join-Path $PhaseDir "completed_hard_overlap_audit.txt")
Get-Content (Join-Path $PhaseDir "sanitize_summary.txt")
Get-Content (Join-Path $PhaseDir "repaired_loader_summary.txt")
Get-Content (Join-Path $PhaseDir "repaired_cbocco\exit_code.txt")
