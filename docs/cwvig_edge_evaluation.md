# CWVIG Edge Evaluation And Calibration

- Date: 2026-06-29
- Executor: Codex
- Scope: Phase 3C edge-level evaluation only; no SoftOverlapDecomposition, no `Z` learning, no optimizer change.
- Sources: `benchmark/flyki_overlap/probe/CWVIGEstimator.*`, `benchmark/flyki_overlap/grouping/GroupingIO.*`, `benchmark/flyki_overlap/1po.txt`, `benchmark/flyki_overlap/1oo.txt`.

## Purpose

Phase 3B showed that Flyki finite-difference evidence can become very large after division by `delta^2`. A fixed threshold such as `0.5` is useful for tiny synthetic functions, but it is not calibrated for Flyki. Phase 3C adds a deterministic edge-level evaluator before any soft decomposition is attempted.

## True Edge Labels

Ground-truth interaction labels are derived from the true `po` grouping file:

1. Read each counted group from `po`.
2. Keep variables in the requested subset `[0, dimension_limit)`.
3. Mark every pair in the same group as a positive edge.
4. Mark evaluated pairs not found in that positive set as negative.

The `oo` file is still required by the CLI and parsed to validate the benchmark grouping input, but edge labels come from `po` co-membership. Shared variables are included naturally because they appear in multiple `po` groups.

Variable ids stay 0-based.

## Metrics

`cwvig_edge_eval_cli` reports:

- evaluated edge count;
- positive and negative evaluated edge count;
- precision, recall, and F1 at `--prob-threshold` using `probability`;
- precision, recall, and F1 at `--score-threshold` using `mean_abs_normalized`;
- top-k precision by `mean_abs_normalized`;
- average positive and negative `mean_abs_normalized`;
- best-F1 threshold scan over observed probability values;
- best-F1 threshold scan over observed normalized scores;
- pairwise positive-vs-negative ranking accuracy.

Ranking accuracy is the AUC-like metric used here: for every positive/negative edge pair, a higher positive score counts as `1`, a tie counts as `0.5`, and a lower positive score counts as `0`. If there is no positive or no negative edge, the value is `0`.

## Calibration Summary

The CLI also summarizes the normalized score distribution:

- min;
- max;
- mean;
- median;
- q25;
- q75;
- q90;
- q95.

Quantiles use deterministic linear interpolation over sorted observed edge scores. The best-F1 scans only observed scores/probabilities, which keeps the calibration reproducible and avoids inventing thresholds outside the data.

## CLI Usage

```powershell
.\build\flyki\Release\cwvig_edge_eval_cli.exe `
  --edges .codex\phase3c_flyki_f1_edges.csv `
  --po benchmark\flyki_overlap\1po.txt `
  --oo benchmark\flyki_overlap\1oo.txt `
  --dimension-limit 10 `
  --prob-threshold 0.5 `
  --score-threshold 0.5 `
  --top-k 10 `
  --print-summary `
  --output .codex\phase3c_flyki_f1_metrics.csv
```

The output CSV uses a simple `metric,value` schema.

## Synthetic Expected Results

One-interaction synthetic case:

- positive edge: `(0,1)`;
- expected probability F1 at `0.5`: `1`;
- expected score F1 at `0.5`: `1`;
- expected top-1 precision: `1`;
- expected ranking accuracy: `1`.

Overlap synthetic case for `(x0+x1)^2 + (x1+x2)^2`:

- positive edges: `(0,1)` and `(1,2)`;
- negative edge: `(0,2)`;
- expected probability F1 at `0.5`: `1`;
- expected score F1 at `0.5`: `1`;
- expected top-2 precision: `1`;
- expected ranking accuracy: `1`.

## Flyki Limited-Subset Smoke Workflow

```powershell
.\build\flyki\Release\cwvig_estimator_cli.exe `
  --mode flyki `
  --func 1 `
  --dimension-limit 10 `
  --contexts 3 `
  --seed 11 `
  --delta 0.0001 `
  --threshold-mode fixed `
  --fixed-threshold 0.5 `
  --output .codex\phase3c_flyki_f1_edges.csv

.\build\flyki\Release\cwvig_edge_eval_cli.exe `
  --edges .codex\phase3c_flyki_f1_edges.csv `
  --po benchmark\flyki_overlap\1po.txt `
  --oo benchmark\flyki_overlap\1oo.txt `
  --dimension-limit 10 `
  --prob-threshold 0.5 `
  --score-threshold 0.5 `
  --top-k 10 `
  --print-summary `
  --output .codex\phase3c_flyki_f1_metrics.csv
```

Observed Phase 3C smoke result for Flyki F1, `dimension_limit=10`, `contexts=3`, `seed=11`:

- evaluated edges: `45`;
- positives: `2`;
- negatives: `43`;
- probability F1 at `0.5`: `0.114285714286`;
- score F1 at `0.5`: `0.114285714286`;
- best score F1: `0.666666666667` at threshold `330634000000`;
- ranking accuracy: `0.738372093023`.

This low fixed-threshold F1 is acceptable for Phase 3C. The point of this layer is to expose calibration behavior before building soft overlapping groups.

## Validation

Commands run on 2026-06-29:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target cwvig_edge_eval_tests --config Release
.\build\flyki\Release\cwvig_edge_eval_tests.exe
cmake --build build/flyki --target cwvig_edge_eval_cli --config Release
```

Smoke outputs:

- `E:\CWVIG_OSD\.codex\phase3c_interaction_edges.csv`
- `E:\CWVIG_OSD\.codex\phase3c_interaction_metrics.csv`
- `E:\CWVIG_OSD\.codex\phase3c_overlap_edges.csv`
- `E:\CWVIG_OSD\.codex\phase3c_overlap_metrics.csv`
- `E:\CWVIG_OSD\.codex\phase3c_flyki_f1_edges.csv`
- `E:\CWVIG_OSD\.codex\phase3c_flyki_f1_metrics.csv`
