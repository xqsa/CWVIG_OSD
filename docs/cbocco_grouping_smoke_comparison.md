# CBOCC Grouping Smoke Comparison

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7C legacy-vs-completed-predicted grouping-source smoke comparison.

## Purpose

This smoke comparison checks that `cbocco` can run the original legacy grouping and an explicit full-coverage completed-predicted grouping through the same original hard-overlap `CBOG_CBD` path.

It is an integration check only. The completed-predicted grouping starts from a capped `dimension_limit=10` CWVIG grouping and is completed to 905D with singleton groups. That is not a fair full CWVIG optimization benchmark.

## Command

```powershell
.\scripts\run_cbocco_grouping_smoke_compare.ps1
```

On this machine, the verified command used explicit tool paths:

```powershell
.\scripts\run_cbocco_grouping_smoke_compare.ps1 -CMakeExe "$env:USERPROFILE\scoop\shims\cmake.exe" -PythonExe "C:\Users\83718\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
```

The script builds:

- `cbocco`
- `cwvig_grouping_pipeline_cli`
- `grouping_coverage_cli`
- `grouping_source_cli`

## Flow

1. Generate capped CWVIG grouping for `func=1`, `dimension_limit=10`, `seed=1`, `contexts=3`.
2. Complete missing variables to expected dimension `905` with singleton groups.
3. Validate the completed `po/oo` through the explicit grouping loader.
4. Run legacy `cbocco` with `--require-full-coverage true`.
5. Run explicit completed-predicted `cbocco` with `--require-full-coverage true`.
6. Parse both result files into `comparison.csv`.
7. Write a short caveated smoke report.

## Output Files

```text
results/phase7c/predicted_10d/
results/phase7c/completed_predicted_groups.txt
results/phase7c/completed_predicted_overlap.txt
results/phase7c/completion_summary.txt
results/phase7c/completed_loader_summary.txt
results/phase7c/legacy/
results/phase7c/completed_predicted/
results/phase7c/comparison.csv
results/phase7c/phase7c_smoke_comparison.md
```

## Parser Behavior

`experiments/parse_cbocco_results.py` parses each `*.result.txt` file as no-header `FE,fitness` rows and extracts:

- file exists / non-empty status
- first recorded FE and fitness
- final recorded FE and fitness
- logged row count

For Phase 7C it also reads `cbocco_stdout.txt` startup logs to fill grouping metadata in `comparison.csv`.

## Observed Result

```text
legacy: final_fe=162000, final_fitness=9.70409e+10, logged_rows=100, exit_code=0
completed_predicted: final_fe=181600, final_fitness=1.81944e+10, logged_rows=100, exit_code=0
```

Both runs used command-line `maxfes=1000`, but final recorded FE was much larger. This is consistent with the existing `CBOG_CBD` flow where `testStage()` consumes evaluations before `optimizationStage()` checks the evaluation limit. Phase 7C records this behavior and does not change it.

## Interpretation Limits

- Both runs use original hard-overlap `CBOG_CBD`.
- No `SharedVariablePolicy` is implemented in this phase.
- The completed-predicted run uses singleton completion of a capped 10D grouping.
- The completed-predicted run is not a fair full CWVIG optimization result.
- Any fitness difference here is not an optimization performance claim.
