# CWVIG Estimator

Date: 2026-06-29  
Executor: Codex  
Scope: Phase 3B limited-subset probabilistic CWVIG estimator.

## Purpose

Phase 3B adds a small, deterministic estimator for bounded variable subsets.
It does not implement `SoftOverlapDecomposition`, `Z` learning, a shared
variable policy, or optimizer changes.

## Finite-difference Evidence

For pair `(i, j)` and context vector `x`:

```text
raw = f(x + delta_i + delta_j) - f(x + delta_i) - f(x + delta_j) + f(x)
```

Raw evidence scales with `delta_i * delta_j`, so the estimator also stores:

```text
normalized = abs(raw) / (abs(delta * delta) + epsilon)
```

Each pair/context uses 4 black-box evaluations.

## Multi-context Probing

For `R = contexts`, the estimator samples `R` deterministic random context
vectors using the configured seed. For every pair inside `dimension_limit`, it
accumulates:

- `mean_abs_raw`
- `mean_abs_normalized`
- `std_normalized`

Only the first `dimension_limit` variables are probed. Full 905D pairwise
probing is intentionally skipped in this phase.

## Probability

For each pair:

```text
p_ij = sigmoid((mean_abs_normalized - threshold) / (std_normalized / sqrt(R) + epsilon))
```

Supported threshold modes:

- `fixed`: use `fixed_threshold`.
- `quantile`: deterministic median of `mean_abs_normalized` values in the
  probed subset.

## Uncertainty

Uncertainty is binary entropy:

```text
u_ij = -p_ij * log(p_ij + epsilon) - (1 - p_ij) * log(1 - p_ij + epsilon)
```

It is highest near `p = 0.5` and near zero for confident probabilities.

## CLI

Target:

```text
cwvig_estimator_cli
```

Synthetic examples:

```powershell
.\build\flyki\Release\cwvig_estimator_cli.exe --mode synthetic --synthetic-function sphere --dimension-limit 4 --contexts 3 --seed 11 --delta 0.0001 --threshold-mode fixed --fixed-threshold 0.5 --output .codex\phase3b_sphere_edges.csv
.\build\flyki\Release\cwvig_estimator_cli.exe --mode synthetic --synthetic-function interaction --dimension-limit 4 --contexts 3 --seed 11 --delta 0.0001 --threshold-mode fixed --fixed-threshold 0.5 --output .codex\phase3b_interaction_edges.csv
.\build\flyki\Release\cwvig_estimator_cli.exe --mode synthetic --synthetic-function overlap --dimension-limit 3 --contexts 3 --seed 11 --delta 0.0001 --threshold-mode fixed --fixed-threshold 0.5 --output .codex\phase3b_overlap_edges.csv
```

Flyki small-subset example:

```powershell
.\build\flyki\Release\cwvig_estimator_cli.exe --mode flyki --func 1 --dimension-limit 10 --contexts 3 --seed 11 --delta 0.0001 --threshold-mode fixed --fixed-threshold 0.5 --output .codex\phase3b_flyki_f1_edges.csv
```

CSV columns:

```text
i,j,mean_abs_raw,mean_abs_normalized,std_normalized,threshold,probability,uncertainty,contexts
```

## Expected Synthetic Behavior

- `sphere`: all pair probabilities are low.
- `interaction`: pair `(0,1)` has the highest `mean_abs_normalized`.
- `overlap`: variable `1` appears in both interacting components and has a
  higher probability-weighted degree than variables `0` and `2`.

## Build And Test

From repository root:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target cwvig_estimator_cli --config Release
cmake --build build/flyki --target cwvig_estimator_tests --config Release
.\build\flyki\Release\cwvig_estimator_tests.exe
```

`cbocco` still depends on missing external CMA-ES files and is not required for
this phase.
