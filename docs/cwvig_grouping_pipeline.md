# CWVIG Grouping Pipeline CLI

- Date: 2026-06-29
- Executor: Codex
- Scope: Phase 5A integrated grouping pipeline only; no optimizer change and no SharedVariablePolicy.

## Purpose

Phase 5A wraps the existing CWVIG grouping steps into one reproducible CLI:

```text
CWVIG estimation -> optional edge metrics -> weighted/pruned graph -> soft overlap decomposition
-> predicted po/oo/Z -> explicit loader validation -> pipeline report
```

The pipeline does not call `CBOG_CBD`, `CMAESO`, or `CBOCC` optimization logic.

## Architecture

New files:

- `benchmark/flyki_overlap/grouping/CWVIGGroupingPipeline.h`
- `benchmark/flyki_overlap/grouping/CWVIGGroupingPipeline.cpp`
- `benchmark/flyki_overlap/grouping/cwvig_grouping_pipeline_cli.cpp`
- `benchmark/flyki_overlap/grouping/CWVIGGroupingPipelineTests.cpp`

The implementation reuses existing modules:

- `BenchmarkEvaluator`, `SyntheticFunctions`, `CWVIGEstimator` for edge estimation.
- `CWVIGEdgeMetrics` for optional edge metrics.
- `WeightedInteractionGraph`, `SoftOverlapCalibration`, `SoftOverlapDecomposition` for grouping.
- `GroupingIO`, `CBOCCGroupingLoader` for `po/oo` writing and validation.

## Presets

| Preset | Edge Weight | Sparsification | Shared Rule | Intended Use |
| --- | --- | --- | --- | --- |
| `conservative` | `rank_normalized` | `mutual_top_k` | `hard_membership_only` | Very sparse diagnostic; avoids over-sharing aggressively. |
| `balanced` | `uncertainty_penalized` | `target_avg_degree` | `hard_plus_second_membership` | Default candidate; keeps more structure while reducing over-sharing. |
| `capped` | `log1p` | `score_threshold` | `capped_shared` | Direct cap on shared variables. |

Default preset: `balanced`.

The pipeline treats all-zero/no-positive CWVIG evidence as an empty graph before decomposition. This keeps separable synthetic cases from turning numerical noise into rank-normalized edges.

## CLI Usage

Synthetic overlap with after-the-fact truth evaluation:

```powershell
.\build\flyki\Release\cwvig_grouping_pipeline_cli.exe `
  --mode synthetic `
  --synthetic-function overlap `
  --dimension-limit 3 `
  --contexts 3 `
  --seed 11 `
  --delta 0.0001 `
  --preset balanced `
  --true-po .codex\phase4b_synthetic_true_po.txt `
  --true-oo .codex\phase4b_synthetic_true_oo.txt `
  --output-dir .codex\phase5a\synthetic_overlap_balanced `
  --print-summary
```

Flyki F1 10D balanced smoke:

```powershell
.\build\flyki\Release\cwvig_grouping_pipeline_cli.exe `
  --mode flyki `
  --func 1 `
  --dimension-limit 10 `
  --contexts 3 `
  --seed 11 `
  --delta 0.0001 `
  --preset balanced `
  --true-po benchmark\flyki_overlap\1po.txt `
  --true-oo benchmark\flyki_overlap\1oo.txt `
  --output-dir .codex\phase5a\flyki_f1_balanced `
  --print-summary
```

Truth files are optional. When `--true-po` and `--true-oo` are omitted, the pipeline still writes grouping outputs and skips metric files that require labels.

## Output Files

Each run writes:

- `edges.csv`
- `z.csv`
- `predicted_groups.txt`
- `predicted_overlap.txt`
- `pipeline_config.txt`
- `pipeline_summary.txt`

When truth files are provided, it also writes:

- `edge_metrics.csv`
- `grouping_metrics.csv`

`pipeline_summary.txt` records case, dimension, contexts, delta, FE count, preset, graph edge counts before/after pruning, group count, shared-variable count, over-shared ratio, singleton count, group-size summary, validation errors, SharedVar metrics, and mean best group Jaccard.

## Reproducibility Rules

- The seed is explicit.
- `dimension-limit`, `contexts`, and `delta` are recorded in `pipeline_config.txt` and `pipeline_summary.txt`.
- Output filenames are fixed.
- `pipeline_summary.txt`, `predicted_groups.txt`, and `predicted_overlap.txt` are deterministic for fixed inputs.
- True `po/oo` files are used only after decomposition for metrics.

## Verified Smoke Results

Synthetic:

| Case | Preset | Groups | Shared | Over-shared | SharedVar-F1 | Validation Errors |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| overlap | balanced | 2 | 1 | 0.333333 | 1 | 0 |
| sphere/no-edge | balanced | 3 | 0 | 0 | 1 | 0 |
| clique | conservative | 2 | 0 | 0 | 1 | 0 |

Flyki F1 10D:

| Phase | Method | Shared | Over-shared | SharedVar-F1 | Mean Best Group Jaccard | FE |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 4A | raw baseline | 10 | 1.0 | 0.333333 | 0.231111 | 540 |
| 4B | log1p unsupervised baseline | 10 | 1.0 | 0.333333 | 0.231111 | 540 |
| 4C | rank + mutual-top-k | 0 | 0.0 | 0 | 0.888889 | 540 |
| 4C | uncertainty + target-degree | 3 | 0.3 | 0 | 0.714286 | 540 |
| 4C | log1p + capped-shared | 5 | 0.5 | 0.285714 | 0.231111 | 540 |
| 5A | conservative preset | 0 | 0.0 | 0 | 0.888889 | 540 |
| 5A | balanced preset | 5 | 0.5 | 0 | 0.636364 | 540 |
| 5A | capped preset | 5 | 0.5 | 0 | 0.551852 | 540 |

The detailed comparison table is in `.codex/phase5a/comparison_summary.csv`.

## Optimizer Integration Path

This phase produces deterministic `predicted_groups.txt`, `predicted_overlap.txt`, and `z.csv` from a single command. Later optimizer integration can consume these files through the existing explicit grouping loader without changing the original CBCCO baseline path.

## Limitations

- No SharedVariablePolicy is implemented.
- No `CBOG_CBD`, `CMAESO`, or optimizer behavior is changed.
- The presets are small-scale defaults, not final algorithm claims.
- Flyki smoke uses only `func=1`, `dimension-limit=10`.
- Full 905D LSGO runs remain intentionally out of scope for this phase.
