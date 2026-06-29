# Probe Harness

Date: 2026-06-29  
Executor: Codex  
Scope: Phase 3A black-box evaluation harness and finite-difference smoke probe.

## Purpose

Phase 3A adds a small black-box probing harness for future CWVIG interaction
estimation. It does not implement the full CWVIG estimator, probability matrix
`P`, uncertainty matrix `U`, soft decomposition, or optimizer changes.

## BenchmarkEvaluator

Files:

```text
benchmark/flyki_overlap/probe/BenchmarkEvaluator.h
benchmark/flyki_overlap/probe/BenchmarkEvaluator.cpp
```

API:

```cpp
BenchmarkEvaluator evaluator(func_id, benchmark_root);
double y = evaluator.evaluate(x);
std::size_t d = evaluator.dimension();
std::size_t fe = evaluator.evaluations();
evaluator.resetEvaluations();
```

The evaluator wraps the existing Flyki `generateFuncObj` and benchmark
`compute(double*)` path. The Flyki overlap dimension is fixed at 905 in this
phase because the benchmark classes do not expose a dimension getter.

## Synthetic Functions

Files:

```text
benchmark/flyki_overlap/probe/SyntheticFunctions.h
benchmark/flyki_overlap/probe/SyntheticFunctions.cpp
```

Included functions:

- `separableSphere(x)`: separable sum of squares.
- `twoVariableInteraction(x)`: `x0 * x1`.
- `tinyOverlappingFunction(x)`: `(x0 + x1)^2 + (x1 + x2)^2`, where variable 1
  appears in two components.

## Finite Difference Formula

File:

```text
benchmark/flyki_overlap/probe/FiniteDifferenceProbe.h
benchmark/flyki_overlap/probe/FiniteDifferenceProbe.cpp
```

For pair `(i, j)`:

```text
evidence = f(x + delta_i + delta_j) - f(x + delta_i) - f(x + delta_j) + f(x)
```

Each pair probe uses exactly 4 black-box evaluations.

## CLI

Target:

```text
probe_smoke_cli
```

Synthetic smoke:

```powershell
.\build\flyki\Release\probe_smoke_cli.exe --mode synthetic --dimension-limit 3 --seed 7 --delta 0.0001 --output .codex\phase3a_synthetic_probe.csv
```

Observed output:

```text
Synthetic Interaction Evidence: 1e-08
Synthetic Separable Max Abs Evidence: 0
FE Count: 12
```

Flyki small-subset smoke:

```powershell
.\build\flyki\Release\probe_smoke_cli.exe --mode flyki --func 1 --dimension-limit 10 --seed 7 --delta 0.0001 --output .codex\phase3a_flyki_f1_probe.csv
```

Observed output:

```text
Flyki Function: 1
Dimension Limit: 10
FE Count: 180
```

CSV columns:

```text
i,j,evidence
```

## Why Not Full 905D Yet

Full pairwise probing over 905 variables would require `905 * 904 / 2 = 409060`
pairs and `1,636,240` black-box evaluations with the current 4-evaluation
probe. Phase 3A intentionally limits Flyki probing to a small subset, such as
the first 10 variables, to verify plumbing before adding budgeted CWVIG logic.

## Build And Test

From repository root:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target probe_smoke_cli --config Release
cmake --build build/flyki --target probe_harness_tests --config Release
.\build\flyki\Release\probe_harness_tests.exe
```

`cbocco` still depends on missing external CMA-ES files and is not required for
this phase.
