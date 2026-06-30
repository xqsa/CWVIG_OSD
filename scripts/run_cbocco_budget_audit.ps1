param(
    [string]$BuildDir = "",
    [string]$CMakeExe = "",
    [string]$PythonExe = ""
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build\flyki_phase7d"
}
$PhaseDir = Join-Path $RepoRoot "results\phase7d_budget_audit"
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
        "1.1.100.CBOG-CBD.result.txt", `
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
        if (Test-Path "1.1.100.CBOG-CBD.result.txt") {
            Move-Item -Force "1.1.100.CBOG-CBD.result.txt" (Join-Path $RunDir "1.1.100.CBOG-CBD.result.txt")
        }
        if (Test-Path "CBCCOtimefile.txt") {
            Move-Item -Force "CBCCOtimefile.txt" (Join-Path $RunDir "CBCCOtimefile.txt")
        }
        if (Test-Path "errcmaes.err") {
            Move-Item -Force "errcmaes.err" (Join-Path $RunDir "errcmaes.err")
        }
        Remove-CBOCCTemps
        if ($code -ne 0) {
            throw "cbocco failed with exit code $code. See $RunDir"
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
foreach ($target in @("cbocco", "cwvig_grouping_pipeline_cli", "grouping_coverage_cli", "grouping_source_cli")) {
    & $CMakeExe --build $BuildDir --target $target --config Release
    Check-Exit "cmake build $target"
}

$CboccoExe = Find-BuiltExe "cbocco"
$PipelineExe = Find-BuiltExe "cwvig_grouping_pipeline_cli"
$CoverageExe = Find-BuiltExe "grouping_coverage_cli"
$SourceExe = Find-BuiltExe "grouping_source_cli"

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $PhaseDir
New-Item -ItemType Directory -Force -Path $PhaseDir | Out-Null

$PredictedDir = Join-Path $PhaseDir "predicted_10d"
$CompletedPo = Join-Path $PhaseDir "completed_predicted_groups.txt"
$CompletedOo = Join-Path $PhaseDir "completed_predicted_overlap.txt"

& $PipelineExe `
    --config (Join-Path $RepoRoot "configs\cwvig_presets\capped_default.json") `
    --mode flyki `
    --func 1 `
    --dimension-limit 10 `
    --seed 1 `
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

& $SourceExe `
    --source explicit_files `
    --po $CompletedPo `
    --oo $CompletedOo `
    --print-summary > (Join-Path $PhaseDir "completed_loader_summary.txt") 2> (Join-Path $PhaseDir "completed_loader_stderr.txt")
Check-Exit "grouping_source_cli"

$AuditArgs = @()
foreach ($MaxFes in @(500, 1000, 2000)) {
    $LegacyDir = Join-Path $PhaseDir "legacy_maxfes$MaxFes"
    Invoke-CBOCCRun `
        -RunDir $LegacyDir `
        -Arguments @("1", "CBCCO", "1", "$MaxFes", "--grouping-source", "legacy_by_function", "--root", ".", "--require-full-coverage", "true")
    $AuditArgs += @("--run", "legacy_maxfes$MaxFes", (Join-Path $LegacyDir "1.1.100.CBOG-CBD.result.txt"), "$MaxFes", (Join-Path $LegacyDir "cbocco_stdout.txt"), (Join-Path $LegacyDir "exit_code.txt"))

    $CompletedDir = Join-Path $PhaseDir "completed_predicted_maxfes$MaxFes"
    Invoke-CBOCCRun `
        -RunDir $CompletedDir `
        -Arguments @("1", "CBCCO", "1", "$MaxFes", "--grouping-source", "explicit_files", "--po", $CompletedPo, "--oo", $CompletedOo, "--require-full-coverage", "true")
    $AuditArgs += @("--run", "completed_predicted_maxfes$MaxFes", (Join-Path $CompletedDir "1.1.100.CBOG-CBD.result.txt"), "$MaxFes", (Join-Path $CompletedDir "cbocco_stdout.txt"), (Join-Path $CompletedDir "exit_code.txt"))
}

& $PythonProgram @PythonArgs `
    (Join-Path $RepoRoot "experiments\audit_cbocco_budget.py") `
    @AuditArgs `
    --output (Join-Path $PhaseDir "budget_summary.csv") `
    --report (Join-Path $PhaseDir "budget_audit_report.md")
Check-Exit "audit_cbocco_budget.py"

Get-Content (Join-Path $PhaseDir "budget_summary.csv")
