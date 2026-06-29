$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$PythonExe = "python"
$PythonArgs = @()
if (Get-Command py -ErrorAction SilentlyContinue) {
    $PythonExe = "py"
    $PythonArgs = @("-3")
}

cmake -S "$RepoRoot\benchmark\flyki_overlap" -B "$RepoRoot\build\flyki" -DCMAKE_BUILD_TYPE=Release
cmake --build "$RepoRoot\build\flyki" --target cwvig_grouping_pipeline_cli --config Release
cmake --build "$RepoRoot\build\flyki" --target grouping_source_cli --config Release

& $PythonExe @PythonArgs "$RepoRoot\experiments\run_grouping_batch.py" `
    --mode medium `
    --output-root results/repro_medium `
    --preset-config-dir configs/cwvig_presets `
    --clean

& $PythonExe @PythonArgs "$RepoRoot\experiments\analyze_grouping_batch.py" `
    --output-root results/repro_medium
