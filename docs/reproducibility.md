# Reproducibility

- Date: 2026-06-29
- Executor: Codex
- Frozen default candidate config: `configs/cwvig_presets/capped_default.json`

## Build

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target cwvig_grouping_pipeline_cli --config Release
cmake --build build/flyki --target grouping_source_cli --config Release
```

The full `cbocco` target is still blocked by missing external CMA-ES files. CWVIG grouping reproduction does not require CMA-ES.

## Smoke Batch

```powershell
scripts\run_grouping_smoke.ps1
```

Outputs:

- `results/repro_smoke/summary.csv`
- `results/repro_smoke/preset_summary.csv`
- `results/repro_smoke/by_dimension_preset.csv`
- `results/repro_smoke/by_context_preset.csv`
- `results/repro_smoke/by_seed_preset.csv`
- `results/repro_smoke/default_preset_selection.md`

## Medium Batch

```powershell
scripts\run_grouping_medium.ps1
```

Outputs:

- `results/repro_medium/summary.csv`
- `results/repro_medium/preset_summary.csv`
- `results/repro_medium/by_dimension_preset.csv`
- `results/repro_medium/by_context_preset.csv`
- `results/repro_medium/by_seed_preset.csv`
- `results/repro_medium/default_preset_selection.md`

Medium mode runs Flyki `func=1`, dimensions `10,20,50`, seeds `1,3,5,11`, contexts `3,5`, and all three frozen preset configs.

## Regenerate Analysis

```powershell
C:\Windows\py.exe -3 experiments\analyze_grouping_batch.py --output-root results/repro_smoke
C:\Windows\py.exe -3 experiments\analyze_grouping_batch.py --output-root results/repro_medium
```

These commands regenerate `summary.csv`, `preset_summary.csv`, grouped preset tables, and `default_preset_selection.md` from run directories.

## Single Config Run

```powershell
.\build\flyki\Release\cwvig_grouping_pipeline_cli.exe `
  --config configs\cwvig_presets\capped_default.json `
  --mode flyki `
  --func 1 `
  --dimension-limit 10 `
  --seed 1 `
  --contexts 3 `
  --output-dir results\repro_checks\manual_capped `
  --print-summary
```

Do not pass true `po/oo` files for the unsupervised path. Add `--true-po benchmark\flyki_overlap\1po.txt --true-oo benchmark\flyki_overlap\1oo.txt` only for after-the-fact evaluation.

## Validate Predicted Grouping Files

```powershell
.\build\flyki\Release\grouping_source_cli.exe `
  --source explicit_files `
  --po results\repro_checks\manual_capped\predicted_groups.txt `
  --oo results\repro_checks\manual_capped\predicted_overlap.txt `
  --print-summary
```

Expected validation line:

```text
Validation Errors: 0
```

## Regression Check

```powershell
C:\Windows\py.exe -3 experiments\check_reproducibility.py --clean
```

This check confirms:

- `capped_default.json` runs on Flyki `func=1`, dimension `10`, seed `1`, contexts `3`;
- `predicted_groups.txt` and `predicted_overlap.txt` are deterministic for fixed inputs;
- predicted `po/oo` pass explicit loader validation;
- truth-only files are not produced when no true `po/oo` paths are passed.

## Manifest

```powershell
C:\Windows\py.exe -3 scripts\write_reproducibility_manifest.py
```

Output:

- `results/reproducibility_manifest.md`
