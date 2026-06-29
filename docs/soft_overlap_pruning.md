# Soft Overlap Pruning And Uncertainty-Aware Weighting

- Date: 2026-06-29
- Executor: Codex
- Scope: Phase 4C graph sparsification, uncertainty-aware edge weights, and shared-variable pruning only; no optimizer change and no SharedVariablePolicy.

## Purpose

Phase 4B made the over-sharing problem measurable: on the Flyki F1 `dimension=10` smoke case, raw/log1p unsupervised grouping marked all 10 variables as shared. Phase 4C adds deterministic graph and shared-variable pruning controls before any cooperative coevolution update is touched.

The data path remains:

```text
CWVIG edge CSV -> weighted/sparsified graph -> soft overlap decomposition -> po/oo/Z outputs
```

## Edge Weight Modes

`WeightedInteractionGraph` now supports:

- `raw`: use the selected score column directly.
- `log1p`: use `log1p(score)`, intended for very large normalized Flyki scores.
- `rank_normalized`: convert distinct edge scores to deterministic ranks in `[0,1]`.
- `uncertainty_penalized`: use `rank_normalized * (1 - uncertainty / log(2))`, clamped to `[0,1]`.

The uncertainty penalty downweights high-entropy edges, especially probability-near-0.5 interactions.

## Graph Sparsification Modes

`SparsificationConfig` supports:

- `score_threshold`: keep edges with weight at least the threshold.
- `top_k_per_node`: keep the union of each node's top-k incident edges.
- `mutual_top_k`: keep only edges that are top-k for both endpoints.
- `target_avg_degree`: keep the highest-weight edges needed for the requested average degree.

`max_avg_degree` is a final guard applied after the selected sparsification mode.

## Membership And Shared Rules

`SoftOverlapConfig` now records:

- `membership_transform`: `current_ratio`, `rank_normalized`, `minmax_normalized`, or `sigmoid_centered`.
- `membership_prune_threshold`: optional weak-membership suppression threshold.
- `shared_rule`: `hard_plus_ratio`, `hard_membership_only`, `hard_plus_second_membership`, or `capped_shared`.
- `max_shared_ratio`: cap for `capped_shared`.
- `shared_min_second_membership`: second-membership evidence required by stricter soft shared rules.

`hard_plus_ratio` preserves the Phase 4A/4B behavior. The stricter rules are intended for Phase 4C pruning experiments.

## Diagnostics

Calibration reports now include:

- `shared_confidence_min`
- `shared_confidence_mean`
- `shared_confidence_max`
- `shared_pruned_by_cap`
- `average_top1_membership`
- `average_top2_membership`
- `average_top2_top1_ratio`

The CLI summary also prints graph edge count, edge weight mode, sparsification mode, cap-pruned shared candidates, and shared-confidence mean.

## CLI Examples

Rank-normalized mutual-top-k:

```powershell
.\build\flyki\Release\soft_overlap_calibration_cli.exe `
  --edges .codex\phase4c_flyki_f1_edges.csv `
  --score-column mean_abs_normalized `
  --use-log1p-score false `
  --edge-weight-mode rank_normalized `
  --sparsification-mode mutual_top_k `
  --top-k-per-node 1 `
  --max-avg-degree 2 `
  --dimension 10 `
  --mode unsupervised `
  --unsupervised-method quantile_score `
  --score-quantile 0.75 `
  --membership-transform current_ratio `
  --shared-rule hard_plus_second_membership `
  --shared-min-second-membership 0.8 `
  --true-po benchmark\flyki_overlap\1po.txt `
  --true-oo benchmark\flyki_overlap\1oo.txt `
  --output-report .codex\phase4c_flyki_rank_mutual_report.csv `
  --output-best-po .codex\phase4c_flyki_rank_mutual_groups.txt `
  --output-best-oo .codex\phase4c_flyki_rank_mutual_overlap.txt `
  --output-best-z .codex\phase4c_flyki_rank_mutual_z.csv `
  --print-summary
```

Uncertainty-penalized target average degree:

```powershell
.\build\flyki\Release\soft_overlap_calibration_cli.exe `
  --edges .codex\phase4c_flyki_f1_edges.csv `
  --score-column mean_abs_normalized `
  --use-log1p-score false `
  --edge-weight-mode uncertainty_penalized `
  --sparsification-mode target_avg_degree `
  --target-avg-degree 1.0 `
  --max-avg-degree 2 `
  --dimension 10 `
  --mode unsupervised `
  --unsupervised-method target_avg_degree `
  --membership-transform current_ratio `
  --shared-rule hard_plus_second_membership `
  --shared-min-second-membership 0.8 `
  --true-po benchmark\flyki_overlap\1po.txt `
  --true-oo benchmark\flyki_overlap\1oo.txt `
  --output-report .codex\phase4c_flyki_uncertainty_targetdeg_report.csv `
  --output-best-po .codex\phase4c_flyki_uncertainty_targetdeg_groups.txt `
  --output-best-oo .codex\phase4c_flyki_uncertainty_targetdeg_overlap.txt `
  --output-best-z .codex\phase4c_flyki_uncertainty_targetdeg_z.csv `
  --print-summary
```

Log1p plus capped shared variables:

```powershell
.\build\flyki\Release\soft_overlap_calibration_cli.exe `
  --edges .codex\phase4c_flyki_f1_edges.csv `
  --score-column mean_abs_normalized `
  --use-log1p-score false `
  --edge-weight-mode log1p `
  --sparsification-mode score_threshold `
  --dimension 10 `
  --mode unsupervised `
  --unsupervised-method quantile_score `
  --score-quantile 0.9 `
  --membership-transform sigmoid_centered `
  --shared-rule capped_shared `
  --max-shared-ratio 0.5 `
  --shared-min-second-membership 0.8 `
  --true-po benchmark\flyki_overlap\1po.txt `
  --true-oo benchmark\flyki_overlap\1oo.txt `
  --output-report .codex\phase4c_flyki_log1p_capped_report.csv `
  --output-best-po .codex\phase4c_flyki_log1p_capped_groups.txt `
  --output-best-oo .codex\phase4c_flyki_log1p_capped_overlap.txt `
  --output-best-z .codex\phase4c_flyki_log1p_capped_z.csv `
  --print-summary
```

## Synthetic Regression

The pruning tests cover:

- path overlap `0-1-2` keeps shared variable `1`;
- clique `{0,1,2}` merges to one group and has zero shared variables under `hard_membership_only`;
- dense noisy graph is not allowed to mark every variable as shared under `capped_shared`;
- `mutual_top_k` is deterministic under ties;
- high-uncertainty edges are downweighted by `uncertainty_penalized`;
- generated `po/oo` files remain readable by the existing explicit grouping loader.

The CLI synthetic path smoke also produced:

- groups: `2`
- shared variables: `1`
- loader validation errors: `0`

## Flyki F1 Dimension-10 Smoke

All rows use `func=1`, `dimension-limit=10`, `contexts=3`, `seed=11`, and the same CWVIG edge file `.codex\phase4c_flyki_f1_edges.csv`.

| Method | Graph Edges | Groups | Shared Variables | Over Shared Ratio | Shared Pruned By Cap | SharedVar-F1 | Mean Best Group Jaccard |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Phase 4A raw-style baseline | 45 | 5 | 10 | 1.0 | 0 | 0.333333 | 0.231111 |
| Phase 4B log1p unsupervised baseline | 45 | 5 | 10 | 1.0 | 0 | 0.333333 | 0.231111 |
| Phase 4B log1p oracle diagnostic | 45 | 2 | 4 | 0.4 | 0 | 0.666667 | 0.236111 |
| 4C rank_normalized + mutual_top_k | 2 | 9 | 0 | 0.0 | 0 | 0.0 | 0.888889 |
| 4C uncertainty_penalized + target_avg_degree | 5 | 7 | 3 | 0.3 | 0 | 0.0 | 0.714286 |
| 4C log1p + capped_shared | 45 | 5 | 5 | 0.5 | 5 | 0.285714 | 0.231111 |

Interpretation:

- The raw/log1p baselines still over-share every variable.
- `mutual_top_k` is intentionally aggressive and removes all shared variables in this tiny smoke case.
- `uncertainty_penalized + target_avg_degree` cuts over-sharing to `0.3` while keeping more grouped structure than mutual-top-k.
- `capped_shared` directly limits the shared set to `0.5` of variables and records that 5 candidates were pruned by the cap.

These are smoke diagnostics, not final optimizer performance claims.

## Limitations

- No SharedVariablePolicy or shared-variable-aware CC update is implemented in this phase.
- The current membership transforms are deterministic scoring rules, not a learned affiliation likelihood model.
- The Flyki run is a 10D subset and should not be treated as full LSGO evidence.
- Full `cbocco` remains blocked until the external CMA-ES headers and sources are supplied.
