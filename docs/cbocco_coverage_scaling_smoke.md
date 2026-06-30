# CBOCC Coverage-Scaling Smoke

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7H coverage-scaling smoke only; no optimizer behavior change.

## Why This Phase Exists

Phase 7G showed that sanitized completed-predicted grouping can run across seeds `1,3,5` under the original hard-overlap `CBOG_CBD` path. The remaining limitation is coverage: those completed-predicted groupings were based on `dimension_limit=10`, then singleton-completed to the 905D benchmark dimension.

Phase 7H tests whether larger CWVIG coverage remains stable before any shared-variable-aware update policy is implemented.

## Command

Default smoke:

```powershell
$env:PATH="$env:USERPROFILE\scoop\apps\gcc\15.2.0\bin;$env:USERPROFILE\scoop\apps\ninja\current;$env:USERPROFILE\scoop\shims;$env:PATH"
.\scripts\run_cbocco_coverage_scaling_smoke.ps1 -CMakeExe "$env:USERPROFILE\scoop\shims\cmake.exe" -PythonExe "C:\Users\83718\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
```

Defaults:

```text
func=1
seeds=1,3
dimension_limits=10,20,50
contexts=3
maxfes=1000
config=configs/cwvig_presets/capped_default.json
output_root=results/phase7h_coverage_scaling
```

`dimension_limit=100` is supported by passing `-DimensionLimits @(10,20,50,100)`, but it is not in the default smoke because the default `10,20,50` run already took about 17 minutes on this machine.

## Pipeline

For each seed and dimension limit:

1. Generate CWVIG grouping with the capped default preset.
2. Complete to 905D with singleton completion.
3. Audit hard-overlap compatibility before sanitizer.
4. Sanitize with `demote_min_shared`.
5. Validate sanitized `po/oo` with `grouping_source_cli`.
6. Audit hard-overlap compatibility after sanitizer.
7. Run strict sanitized completed-predicted `cbocco`.
8. Reuse one strict legacy `cbocco` result per seed as the common reference.

## Outputs

```text
results/phase7h_coverage_scaling/coverage_scaling_summary.csv
results/phase7h_coverage_scaling/by_dimension_summary.csv
results/phase7h_coverage_scaling/by_seed_summary.csv
results/phase7h_coverage_scaling/phase7h_report.md
results/phase7h_coverage_scaling/seed_1/
results/phase7h_coverage_scaling/seed_3/
```

Each `seed_<seed>/dim_<limit>/` directory contains:

```text
predicted/
completed_groups.txt
completed_overlap.txt
hard_overlap_before.txt
hard_overlap_before.csv
sanitize_summary.txt
sanitizer_report.csv
sanitized_groups.txt
sanitized_overlap.txt
sanitized_loader_summary.txt
hard_overlap_after.txt
hard_overlap_after.csv
sanitized_cbocco/
```

`results/*` is ignored by git, so these are local verification artifacts.

## Coverage And Completion

`cwvig_coverage_ratio` is:

```text
dimension_limit / 905
```

`completion_added_ratio` is:

```text
(905 - predicted_covered_unique_variables) / 905
```

Observed default smoke:

| dim | coverage ratio | mean completion added ratio | successful runs |
| ---: | ---: | ---: | ---: |
| 10 | 0.011049723756906077 | 0.988950276243094 | 2 |
| 20 | 0.022099447513812154 | 0.9779005524861878 | 2 |
| 50 | 0.055248618784530384 | 0.9447513812154696 | 2 |

Larger `dimension_limit` reduces singleton completion, but it increases CWVIG probing and grouping cost.

## Sanitizer Observations

All successful sanitized groupings had:

```text
zero_effective_groups_after=0
loader_validation_errors=0
hard_overlap_compatible_after=true
result_file_exists=true
```

Mean zero-effective groups before repair increased with coverage:

| dim | mean zero-effective groups before | mean repairs |
| ---: | ---: | ---: |
| 10 | 1.5 | 1.0 |
| 20 | 4.5 | 3.0 |
| 50 | 17.0 | 5.0 |

This is expected: larger CWVIG coverage creates more overlap structure, so more hard-overlap compatibility repairs may be needed before the unchanged optimizer can run.

## FE Observations

Final FE still differs from legacy:

| seed | dim | sanitized final FE | relative FE difference vs legacy |
| ---: | ---: | ---: | ---: |
| 1 | 10 | 181600 | 0.12098765432098765 |
| 1 | 20 | 183600 | 0.13333333333333333 |
| 1 | 50 | 181000 | 0.11728395061728394 |
| 3 | 10 | 182200 | 0.12469135802469136 |
| 3 | 20 | 182800 | 0.12839506172839507 |
| 3 | 50 | 180600 | 0.11481481481481481 |

Common-FE values are recorded in `coverage_scaling_summary.csv` using the last logged row with `FE <= common_fe`.

## Interpretation Limits

- This is a smoke/integration scaling study, not a final benchmark.
- Original hard-overlap `CBOG_CBD` behavior is unchanged.
- `CMAESO` and benchmark function logic are unchanged.
- Legacy CBCCO behavior is unchanged.
- Sanitization is explicit and logged.
- No partial grouping is used in comparison mode.
- No `SharedVariablePolicy` is implemented.
- No final optimization superiority claim is made.

## Relation To Full-Dimension CWVIG

Full 905D CWVIG probing is intentionally avoided by default because it would be much more expensive. Phase 7H provides an intermediate check: as the probed dimension increases from `10` to `20` to `50`, completed and sanitized groupings still pass loader validation, hard-overlap compatibility, and strict `cbocco` smoke.

The next step is a matched-FE or FE-normalized protocol before using these runs for performance interpretation.
