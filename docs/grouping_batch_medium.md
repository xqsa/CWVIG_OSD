# Medium Grouping Batch Stability Analysis

- Date: 2026-06-29
- Executor: Codex
- Scope: Phase 5C medium grouping batch analysis only; no optimizer change and no SharedVariablePolicy.

## Purpose

Phase 5B smoke mode used only two seeds at `dimension_limit=10`. Phase 5C runs the same CWVIG grouping pipeline across more dimensions, seeds, and context counts before choosing a default grouping preset.

## Medium Command

```powershell
C:\Windows\py.exe -3 experiments\run_grouping_batch.py `
  --mode medium `
  --output-root results\grouping_batch_medium `
  --clean
```

The medium default is:

- functions: `1`
- dimension limits: `10,20,50`
- seeds: `1,3,5,11`
- contexts: `3,5`
- presets: `conservative,balanced,capped`
- total runs: `72`

Dry-run guardrail:

```powershell
C:\Windows\py.exe -3 experiments\run_grouping_batch.py `
  --mode medium `
  --output-root results\grouping_batch_medium_dry `
  --dry-run
```

Partial-run guardrail:

```powershell
C:\Windows\py.exe -3 experiments\run_grouping_batch.py `
  --mode medium `
  --output-root results\grouping_batch_medium_partial `
  --max-runs 12 `
  --clean
```

## Output Files

Phase 5C generated:

- `results/grouping_batch_medium/summary.csv`
- `results/grouping_batch_medium/preset_summary.csv`
- `results/grouping_batch_medium/by_dimension_preset.csv`
- `results/grouping_batch_medium/by_context_preset.csv`
- `results/grouping_batch_medium/by_seed_preset.csv`
- `results/grouping_batch_medium/default_preset_selection.md`
- `results/grouping_batch_medium/batch_plan.txt`

Each run directory under `results/grouping_batch_medium/runs/` contains the pipeline outputs, command, stdout, and stderr.

## Stability Metrics

The analyzer reports, by preset and by grouped views:

- mean/std/median SharedVar-F1
- mean/std over-shared ratio
- mean/std shared-variable count
- validation error total
- number of runs with shared count `0`
- number of runs with over-shared ratio `>= 0.8`
- number of runs with over-shared ratio `<= 0.05`
- mean/std mean-best-group-Jaccard
- mean/std FE count

`summary.csv` includes `delta` and `exact_group_matches` for each run when truth files are available.

## Medium Result

All 72 runs used truth files from `benchmark/flyki_overlap/1po.txt` and `benchmark/flyki_overlap/1oo.txt` only for after-the-fact evaluation.

| Preset | Runs | Shared Mean | Over-shared Mean | SharedVar-F1 Mean | SharedVar-F1 Std | Jaccard Mean | Validation Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced | 24 | 6.70833333333 | 0.3225 | 0.11323051948 | 0.135540809157 | 0.549394011202 | 0 |
| capped | 24 | 12.0833333333 | 0.475 | 0.225159069326 | 0.0849008748789 | 0.539981741639 | 0 |
| conservative | 24 | 0 | 0 | 0 | 0 | 0.611591420064 | 0 |

Recommendation: `capped`.

## Smoke vs Medium

- Phase 5B smoke recommendation: `capped`
- Phase 5C medium recommendation: `capped`

The smoke recommendation still holds under the medium batch. `capped` has higher mean SharedVar-F1 and lower SharedVar-F1 std than `balanced` in this run. `balanced` remains less cap-driven and has lower over-shared ratio, but it did not catch up on truth-based shared-variable metrics. `conservative` is too strict for default use because it predicts zero shared variables in every truth-available medium run.

## Interpreting Conflicts

If a later run shows conflicting evidence:

- prefer zero validation errors first;
- reject presets that often produce `over_shared_ratio >= 0.8`;
- reject presets that always predict zero shared variables when true overlap exists;
- compare SharedVar-F1 mean before group Jaccard;
- if SharedVar-F1 means are close, prefer lower SharedVar-F1 std and lower shared-count std;
- if still close, prefer `balanced` over `capped` because it uses less hard-coded prior.

## Limitations

- Medium mode still uses only Flyki `func=1`.
- Dimension limit stops at `50`; full 905D is intentionally out of scope.
- `capped` currently wins, but it encodes a stronger shared-variable cap prior.
- No SharedVariablePolicy is implemented.
- No optimizer code path is modified.
