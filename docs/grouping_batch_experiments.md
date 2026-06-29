# Grouping Batch Experiments

- Date: 2026-06-29
- Executor: Codex
- Scope: Phase 5B batch grouping experiments only; no optimizer change and no SharedVariablePolicy.

## Why This Exists

Phase 5A made one CWVIG grouping run reproducible. Phase 5B repeats that pipeline across presets, seeds, dimensions, contexts, and functions so the default grouping preset is selected from stable batch evidence before any optimizer integration.

The batch scripts call `cwvig_grouping_pipeline_cli`; they do not call `CBOG_CBD`, `CMAESO`, or `CBOCC`.

## Scripts

- `experiments/run_grouping_batch.py`: runs the pipeline CLI repeatedly, then invokes the analyzer.
- `experiments/analyze_grouping_batch.py`: reads completed run directories and writes summary reports.

Both scripts use only the Python standard library.

## Smoke Mode

Smoke mode is the default quick check:

- `func=1`
- `dimension_limit=10`
- `seeds=1,3`
- `contexts=3`
- presets: `conservative,balanced,capped`

Command used in Phase 5B:

```powershell
C:\Windows\py.exe -3 experiments\run_grouping_batch.py `
  --mode smoke `
  --output-root results\grouping_batch `
  --clean
```

Analyze an existing batch without rerunning the pipeline:

```powershell
C:\Windows\py.exe -3 experiments\analyze_grouping_batch.py `
  --output-root results\grouping_batch
```

## Medium Mode

Medium mode is still small-scale and only uses Flyki `func=1`:

- `dimension_limit=10,20,50`
- `seeds=1,3,5,11`
- `contexts=3,5`
- presets: `conservative,balanced,capped`

```powershell
C:\Windows\py.exe -3 experiments\run_grouping_batch.py `
  --mode medium `
  --output-root results\grouping_batch_medium `
  --clean
```

## Custom Mode

```powershell
C:\Windows\py.exe -3 experiments\run_grouping_batch.py `
  --mode custom `
  --functions 1,2 `
  --dimension-limits 10,20 `
  --seeds 1,3 `
  --contexts 3 `
  --presets balanced,capped `
  --output-root results\grouping_batch_custom `
  --clean
```

The runner checks whether `<func>po.txt` and `<func>oo.txt` exist under `benchmark/flyki_overlap`. If they exist, truth files are passed to the pipeline for after-the-fact metrics. If they are missing, the run still executes and truth-dependent metrics are left blank in the consolidated CSV.

## Output Structure

```text
results/grouping_batch/
├── summary.csv
├── preset_summary.csv
├── default_preset_selection.md
└── runs/
    └── f1_d10_s1_c3_balanced/
        ├── command.txt
        ├── stdout.txt
        ├── stderr.txt
        ├── edges.csv
        ├── edge_metrics.csv
        ├── z.csv
        ├── predicted_groups.txt
        ├── predicted_overlap.txt
        ├── grouping_metrics.csv
        ├── pipeline_config.txt
        └── pipeline_summary.txt
```

## Summary Columns

`summary.csv` includes:

- `case`
- `func`
- `dimension_limit`
- `seed`
- `contexts`
- `preset`
- `truth_available`
- `function_evaluations`
- `graph_edges_before_pruning`
- `graph_edges_after_pruning`
- `groups`
- `shared_variables`
- `over_shared_ratio`
- `singleton_count`
- `mean_group_size`
- `max_group_size`
- `validation_errors`
- `shared_precision`
- `shared_recall`
- `shared_f1`
- `mean_best_group_jaccard`
- `run_dir`

`preset_summary.csv` aggregates mean and population standard deviation by preset for shared variables, over-shared ratio, SharedVar-F1, group Jaccard, validation errors, and FE count.

## Selection Rules

`default_preset_selection.md` uses deterministic rules:

1. Validation errors must be zero.
2. Avoid `over_shared_ratio` close to `1.0`.
3. Avoid presets that always predict zero shared variables when truth metrics are available.
4. Prefer higher SharedVar-F1 when truth exists.
5. Prefer stable shared count across seeds.
6. Prefer lower FE only when grouping metrics are comparable.

If metrics are effectively tied, the tie-break prefers `balanced`, then `capped`, then `conservative`, because `balanced` is less directly cap-driven than `capped` and less likely to under-share than `conservative`.

## Phase 5B Smoke Result

The smoke batch generated:

- `results/grouping_batch/summary.csv`
- `results/grouping_batch/preset_summary.csv`
- `results/grouping_batch/default_preset_selection.md`

All six generated `predicted_groups.txt` and `predicted_overlap.txt` files passed `grouping_source_cli --source explicit_files` with `Validation Errors: 0`.

Current smoke recommendation:

- `capped`, because it has the highest mean SharedVar-F1 in this tiny batch.

Interpretation:

- `conservative` avoids over-sharing but predicts zero shared variables in this truth-available smoke batch.
- `balanced` keeps more group structure and remains the less cap-driven candidate, but its mean SharedVar-F1 is slightly below `capped` in the current 2-seed smoke.
- `capped` is stable and slightly better on SharedVar-F1 here, but its prior is stronger because it enforces a shared cap.

## Limitations

- Smoke mode is intentionally tiny and should not be used as final optimizer evidence.
- Medium mode still only defaults to `func=1`; use custom arguments to broaden functions.
- Full 905D runs are intentionally not attempted by default.
- No SharedVariablePolicy is implemented.
- No optimizer code path is modified.
