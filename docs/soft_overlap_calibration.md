# Soft Overlap Threshold Calibration

- Date: 2026-06-29
- Executor: Codex
- Scope: Phase 4B calibration and grouping evaluation only; no optimizer change and no SharedVariablePolicy.

## Why Calibration Is Needed

Phase 4A proved the graph-to-grouping path works:

```text
CWVIG edge CSV -> weighted graph -> Z -> predicted_groups.txt / predicted_overlap.txt
```

On Flyki F1 `dimension=10`, the raw Phase 4A threshold choice produced `Shared Variables: 10`, so every variable was treated as shared. Phase 4B adds a calibration/evaluation layer to measure that behavior and compare threshold choices before optimizer integration.

## Modes

`unsupervised` mode chooses thresholds from edge scores only. It does not read true `po/oo` while selecting thresholds. If `--true-po` and `--true-oo` are supplied, they are used only after decomposition for evaluation metrics.

`oracle` mode is diagnostic only. It requires true `po/oo`, sweeps threshold combinations, and reports the best row by SharedVar-F1, then mean best group Jaccard, then lower over-shared ratio.

## Threshold Strategies

Unsupervised methods:

- `quantile_score`: choose `score_threshold` from an edge-score quantile.
- `target_avg_degree`: choose the observed score threshold whose selected graph average degree is closest to the target.
- `top_edge_fraction`: keep the top fraction of edges.
- `elbow_score`: use a simple deterministic elbow on descending edge scores.

Oracle grids:

- `--score-threshold-grid`
- `--expand-threshold-grid`
- `--z-threshold-grid`
- `--shared-ratio-threshold-grid`

Each grid accepts comma-separated values or `auto`. Auto grids are derived deterministically from observed edge-score quantiles plus small fixed Z/shared-ratio grids.

## Metrics

The calibration report CSV contains:

- threshold configuration;
- group count;
- unique variable count;
- shared variable count;
- `over_shared_ratio = predicted_shared / dimension`;
- singleton count;
- mean and max group size;
- SharedVar precision, recall, F1;
- mean best group Jaccard;
- exact group match count.

True shared variables are taken from `oo` and restricted to `[0, dimension)`. Group Jaccard also restricts true groups to the same subset.

## CLI Usage

Unsupervised example:

```powershell
.\build\flyki\Release\soft_overlap_calibration_cli.exe `
  --edges .codex\phase4b_flyki_f1_edges.csv `
  --score-column mean_abs_normalized `
  --use-log1p-score true `
  --dimension 10 `
  --mode unsupervised `
  --unsupervised-method quantile_score `
  --score-quantile 0.90 `
  --true-po benchmark\flyki_overlap\1po.txt `
  --true-oo benchmark\flyki_overlap\1oo.txt `
  --output-report .codex\phase4b_flyki_quantile_report.csv `
  --output-best-po .codex\phase4b_flyki_quantile_groups.txt `
  --output-best-oo .codex\phase4b_flyki_quantile_overlap.txt `
  --output-best-z .codex\phase4b_flyki_quantile_z.csv `
  --print-summary
```

Oracle example:

```powershell
.\build\flyki\Release\soft_overlap_calibration_cli.exe `
  --edges .codex\phase4b_flyki_f1_edges.csv `
  --score-column mean_abs_normalized `
  --use-log1p-score true `
  --dimension 10 `
  --mode oracle `
  --true-po benchmark\flyki_overlap\1po.txt `
  --true-oo benchmark\flyki_overlap\1oo.txt `
  --score-threshold-grid auto `
  --expand-threshold-grid auto `
  --z-threshold-grid auto `
  --shared-ratio-threshold-grid auto `
  --output-report .codex\phase4b_flyki_oracle_report.csv `
  --output-best-po .codex\phase4b_flyki_oracle_groups.txt `
  --output-best-oo .codex\phase4b_flyki_oracle_overlap.txt `
  --output-best-z .codex\phase4b_flyki_oracle_z.csv `
  --print-summary
```

## Synthetic Expected Behavior

For the synthetic overlap path `0-1-2`, oracle diagnostics with the expected threshold row reports:

- groups: `2`;
- shared variables: `1`;
- SharedVar-F1: `1`;
- mean best group Jaccard: `1`.

For a no-edge graph, unsupervised mode produces singleton groups and zero shared variables.

For a clique `{0,1,2}`, oracle diagnostics can choose the one-group solution and avoid marking every variable as shared.

## Flyki Small-Subset Result

On Flyki F1 with `dimension=10`, `contexts=3`, `seed=11`:

| Method | Groups | Shared Variables | Over Shared Ratio | SharedVar-F1 |
| --- | ---: | ---: | ---: | ---: |
| Phase 4A raw threshold diagnostic | 10 | 10 | 1.0 | 0.333333 |
| Unsupervised `quantile_score` | 5 | 10 | 1.0 | 0.333333 |
| Unsupervised `target_avg_degree` | 9 | 10 | 1.0 | 0.333333 |
| Oracle auto-grid diagnostic | 2 | 4 | 0.4 | 0.666667 |

This shows that the over-sharing issue is measurable and that supervised diagnostics can identify a less over-shared threshold region. It does not claim final optimization performance.

## Limitations

- Oracle mode is diagnostic only and must not be used inside the default algorithm path.
- Unsupervised thresholds still over-share on the current Flyki F1 small subset.
- No SharedVariablePolicy is implemented.
- No optimizer integration is done.
- Full 905D calibration is intentionally not attempted by default.
