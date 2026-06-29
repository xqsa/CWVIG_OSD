# Soft Overlap Decomposition v0

- Date: 2026-06-29
- Executor: Codex
- Scope: Phase 4A graph-to-grouping conversion only; no optimizer change and no SharedVariablePolicy.
- Sources: `benchmark/flyki_overlap/probe/CWVIGEdgeData.*`, `benchmark/flyki_overlap/grouping/WeightedInteractionGraph.*`, `benchmark/flyki_overlap/grouping/SoftOverlapDecomposition.*`.

## Purpose

Phase 4A converts a CWVIG edge CSV into deterministic soft overlapping groups. It is a small bridge from edge evidence to po/oo-compatible predicted grouping files. It is not the final affiliation model and does not use ground-truth labels during decomposition.

## Weighted Graph

`WeightedInteractionGraph` reads Phase 3B/3C edge CSV files through the existing CWVIG edge parser. It stores undirected weighted edges and supports these score columns:

- `probability`
- `mean_abs_normalized`
- `mean_abs_raw`

The CLI can apply `log1p(score)` before graph construction. This is useful for Flyki normalized scores, which can be very large after division by `delta^2`.

The graph exposes:

- pairwise lookup `score(i,j)`;
- weighted degree;
- strong edges with `weight >= score_threshold`.

## Edge-Seeded Communities

The v0 algorithm is deliberately direct:

1. Select strong edges by score column and threshold.
2. Create one seed community `{i,j}` for each selected edge.
3. Expand a seed by adding variable `v` when its average affinity to the current community is at least `expand_threshold`.
4. Merge duplicate or near-duplicate communities when Jaccard similarity is at least `merge_jaccard_threshold`.
5. Add singleton communities for uncovered variables when `add_singletons_for_uncovered=true`.
6. Sort communities deterministically.

This is enough to validate the graph-to-grouping interface on synthetic overlap patterns before adding a heavier affiliation refinement method.

## Z Definition

`Z[v,k]` is the soft membership of variable `v` in group `k`.

- If `v` is inside community `k`, membership is `1`.
- Otherwise, membership is `avg_affinity(v,k) / (1 + avg_affinity(v,k))`.

This keeps memberships in `[0,1]` without saturating every large Flyki score to exactly `1`.

`Z.csv` schema:

```text
variable,group,membership
```

## Predicted Groups And Shared Variables

Predicted groups are derived from `Z` by including variables with `membership >= z_threshold`. Groups smaller than `min_group_size` are dropped.

A variable is marked shared if:

- it appears in more than one predicted group; or
- its second-largest membership divided by its largest membership is at least `shared_ratio_threshold`.

`predicted_groups.txt` is written with the existing po-compatible counted-group format. `predicted_overlap.txt` is written with one oo-compatible overlap row per predicted group; each row contains shared variables that are present in that group. This keeps the files readable by:

```powershell
.\build\flyki\Release\grouping_source_cli.exe --source explicit_files --po predicted_groups.txt --oo predicted_overlap.txt
```

Variable ids remain 0-based.

## CLI Usage

```powershell
.\build\flyki\Release\soft_overlap_cli.exe `
  --edges .codex\phase4a_synthetic_overlap_edges.csv `
  --score-column mean_abs_normalized `
  --score-threshold 1.0 `
  --use-log1p-score false `
  --expand-threshold 1.5 `
  --merge-jaccard-threshold 0.8 `
  --z-threshold 0.8 `
  --shared-ratio-threshold 0.7 `
  --dimension 3 `
  --output-z .codex\phase4a_synthetic_z.csv `
  --output-po .codex\phase4a_synthetic_predicted_groups.txt `
  --output-oo .codex\phase4a_synthetic_predicted_overlap.txt `
  --print-summary
```

Optional arguments:

- `--min-group-size`, default `1`;
- `--add-singletons-for-uncovered`, default `true`.

## Synthetic Expected Behavior

Path overlap graph `0-1-2`:

- predicted groups: `{0,1}` and `{1,2}`;
- shared variable: `1`;
- Z memberships are in `[0,1]`;
- predicted po/oo files are readable by the explicit grouping loader.

Clique graph `{0,1,2}`:

- duplicate edge seeds merge into one group `{0,1,2}`.

Two disconnected cliques:

- each connected clique becomes its own group.

One-interaction graph:

- strong pair `{0,1}` becomes one group;
- uncovered variables become singletons when enabled.

No strong edge:

- all variables become singleton groups when enabled.

## Flyki Smoke Workflow

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
  --output .codex\phase4a_flyki_f1_edges.csv

.\build\flyki\Release\soft_overlap_cli.exe `
  --edges .codex\phase4a_flyki_f1_edges.csv `
  --score-column mean_abs_normalized `
  --score-threshold 330634000000 `
  --use-log1p-score true `
  --expand-threshold 20 `
  --merge-jaccard-threshold 0.8 `
  --z-threshold 0.99 `
  --shared-ratio-threshold 0.7 `
  --dimension 10 `
  --output-z .codex\phase4a_flyki_f1_z.csv `
  --output-po .codex\phase4a_flyki_f1_predicted_groups.txt `
  --output-oo .codex\phase4a_flyki_f1_predicted_overlap.txt `
  --print-summary

.\build\flyki\Release\grouping_source_cli.exe `
  --source explicit_files `
  --po .codex\phase4a_flyki_f1_predicted_groups.txt `
  --oo .codex\phase4a_flyki_f1_predicted_overlap.txt `
  --print-summary
```

The Phase 4A Flyki smoke only checks deterministic output and loader compatibility. Accuracy is intentionally not required yet.

## Limitations

- v0 uses greedy edge seeding, not a learned affiliation model.
- Thresholds are manual; automatic calibration is left for a later phase.
- `Z` is a deterministic affinity matrix, not optimized by likelihood.
- Shared-variable confidence is not passed into cooperative coevolution yet.
- Full 905D decomposition is not attempted by default.
