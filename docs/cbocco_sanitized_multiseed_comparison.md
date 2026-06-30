# CBOCC Sanitized Multi-Seed Smoke Comparison

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7G integration smoke only; no optimizer behavior change.

## Why This Phase Exists

Phase 7E showed that completed-predicted grouping can run for seeds `1` and `5`, but seed `3` failed before producing a result file because some groups became zero-dimensional after original hard-overlap shared-variable removal.

Phase 7F added a pre-optimizer compatibility audit and explicit sanitizer. Phase 7G reruns the same multi-seed comparison with sanitized completed-predicted grouping so every explicit grouping is full coverage and hard-overlap compatible before `CBOG_CBD` is constructed.

This is still a smoke/integration comparison, not a final optimization benchmark.

## Command

```powershell
$env:PATH="$env:USERPROFILE\scoop\apps\gcc\15.2.0\bin;$env:USERPROFILE\scoop\apps\ninja\current;$env:USERPROFILE\scoop\shims;$env:PATH"
.\scripts\run_cbocco_multiseed_sanitized_compare.ps1 -CMakeExe "$env:USERPROFILE\scoop\shims\cmake.exe" -PythonExe "C:\Users\83718\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
```

The script runs:

```text
func=1
seeds=1,3,5
maxfes argument=1000
config=configs/cwvig_presets/capped_default.json
dimension_limit=10
contexts=3
repair_policy=demote_min_shared
```

## Pipeline

For each seed:

1. Generate capped 10D CWVIG grouping.
2. Complete to 905D with singleton groups.
3. Audit hard-overlap compatibility before sanitizer.
4. Run `hard_overlap_sanitize_cli --repair-policy demote_min_shared`.
5. Validate sanitized `po/oo` with `grouping_source_cli`.
6. Audit sanitized compatibility.
7. Run legacy `cbocco` with strict full-coverage and hard-overlap compatibility flags.
8. Run sanitized completed-predicted `cbocco` with the same strict flags.
9. Analyze final FE, common FE, result-file presence, hard-overlap compatibility, and sanitizer metadata.

## Outputs

```text
results/phase7g/sanitized_multiseed_comparison.csv
results/phase7g/sanitized_multiseed_summary.csv
results/phase7g/phase7g_report.md
results/phase7g/seed_1/
results/phase7g/seed_3/
results/phase7g/seed_5/
```

Each seed directory contains:

```text
grouping/predicted_10d/
grouping/completed_groups.txt
grouping/completed_overlap.txt
grouping/sanitized_groups.txt
grouping/sanitized_overlap.txt
grouping/sanitized_loader_summary.txt
audit/hard_overlap_audit_before.csv
audit/hard_overlap_audit_before.txt
audit/sanitize_summary.txt
audit/sanitizer_report.csv
audit/hard_overlap_audit_after.csv
audit/hard_overlap_audit_after.txt
legacy/
sanitized_completed_predicted/
```

`results/*` is ignored by git, so these are local verification artifacts.

## Sanitizer Result

Phase 7G sanitizer summary:

| seed | zero before | zero after | shared before | shared after | repair actions | loader errors |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0 | 0 | 5 | 5 | 0 | 0 |
| 3 | 3 | 0 | 5 | 3 | 2 | 0 |
| 5 | 0 | 0 | 5 | 5 | 0 | 0 |

Seed `3` repairs:

```text
demote_min_shared group=2 variable=0 removed variable from 5 overlap rows
demote_min_shared group=4 variable=1 removed variable from 3 overlap rows
```

All sanitized groupings had:

```text
zero_effective_groups_after=0
hard_overlap_compatible_after=true
loader_validation_errors=0
both_hard_overlap_compatible=true
```

## FE Results

Per-seed comparison:

| seed | legacy final FE | sanitized final FE | common FE | final FE diff | winner at common FE |
| ---: | ---: | ---: | ---: | ---: | --- |
| 1 | 162000 | 181600 | 162000 | 19600 | sanitized_completed_predicted |
| 3 | 162000 | 182200 | 162000 | 20200 | sanitized_completed_predicted |
| 5 | 162000 | 181800 | 162000 | 19800 | sanitized_completed_predicted |

Summary:

```text
legacy mean_final_fitness=111101733333.33333
legacy mean_common_fe_fitness=111101733333.33333
legacy mean_final_fe=162000.0
sanitized_completed_predicted mean_final_fitness=16879933333.333334
sanitized_completed_predicted mean_common_fe_fitness=17400366666.666668
sanitized_completed_predicted mean_final_fe=181866.66666666666
```

The final-FE comparison is not FE-matched. Common-FE comparison uses the last logged row with `FE <= common_fe`, so it is more conservative than comparing each run at its own final FE.

## Interpretation Limits

- Sanitized completed-predicted uses 10D CWVIG plus singleton completion plus `demote_min_shared` repair.
- The sanitizer changes predicted overlap membership only to avoid zero-dimensional CMA-ES groups.
- Original `CBOG_CBD` hard-overlap behavior is unchanged.
- `CMAESO` and benchmark functions are unchanged.
- Legacy CBCCO behavior is unchanged.
- No `SharedVariablePolicy` is implemented.
- These results do not prove final optimization superiority.

## Next Step

Use the sanitized comparison as an integration baseline, then add matched-FE or FE-normalized comparison scripts before interpreting optimization performance.
