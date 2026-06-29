# Grouping IO And Metrics

Date: 2026-06-29  
Executor: Codex  
Scope: Phase 1 grouping data model, po/oo IO, shared-variable metrics, and CLI.

## Purpose

This module provides a reusable C++17 grouping layer for Flyki overlap
experiments before implementing CWVIG-OSD. It does not modify CBOCC, CBOG_CBD,
CMAESO, or the F1-F12 benchmark functions.

## Files

Implementation files live under:

```text
benchmark/flyki_overlap/grouping/
```

Main targets:

- `grouping_core`: C++17 static library with grouping data, IO, and metrics.
- `grouping_metrics_cli`: command-line smoke and comparison utility.
- `grouping_metrics_tests`: small local regression test executable.

## po/oo File Format

The current Flyki `po` and `oo` files use repeated counted groups:

```text
<group_size>
<var_id_0> <var_id_1> ... <var_id_n>
<group_size>
<var_id_0> <var_id_1> ... <var_id_n>
...
```

Parser assumptions:

- Variable ids are preserved as 0-based ids.
- Group count is inferred by reading until EOF; it is not hard-coded to 20.
- Newlines are not semantically important beyond normal whitespace parsing.
- Group sizes and variable ids must be non-negative integers.
- The parser rejects malformed files with missing ids or invalid negative values.

Writers emit the same counted-group structure.

## Shared Variable Definition

Two shared-variable extractors are provided:

- `extractSharedVariablesFromGroups(groups)`: variables with occurrence count
  greater than 1 across primary groups.
- `extractSharedVariablesFromOo(overlap_groups)`: the union of all variables
  listed in the `oo` overlap groups.

The CLI uses `oo` files for shared-variable precision, recall, and F1 because
`oo` is the explicit overlap-variable annotation in the Flyki data.

## Metrics

For true shared-variable set `T` and predicted shared-variable set `P`:

```text
TP = |T intersect P|
FP = |P - T|
FN = |T - P|
Precision = TP / (TP + FP)
Recall = TP / (TP + FN)
F1 = 2 * Precision * Recall / (Precision + Recall)
```

If both sets are empty, precision and recall are defined as `1.0`.

Group summaries include:

- number of groups
- total variable occurrences
- number of unique variables
- min group size
- max group size
- mean group size
- number of shared variables inferred from repeated group occurrences

## Build Commands

From repository root:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target grouping_metrics_cli --config Release
cmake --build build/flyki --target grouping_metrics_tests --config Release
```

If the current shell does not already see Scoop-installed tools, prepend:

```powershell
$env:PATH = "$env:USERPROFILE\scoop\shims;$env:USERPROFILE\scoop\apps\gcc\current\bin;$env:USERPROFILE\scoop\apps\llvm\current\bin;$env:PATH"
```

## Smoke Test

Use `1po.txt` and `1oo.txt` as both ground truth and prediction by omitting the
prediction arguments:

```powershell
.\build\flyki\Release\grouping_metrics_cli.exe --true-po benchmark\flyki_overlap\1po.txt --true-oo benchmark\flyki_overlap\1oo.txt
```

Expected key output:

```text
SharedVar Precision: 1.000000
SharedVar Recall: 1.000000
SharedVar F1: 1.000000
True Groups: 20
True Total Variable Occurrences: 1000
True Unique Variables: 905
True Group Size Min: 25
True Group Size Max: 100
True Group Size Mean: 50.000000
True Shared Variables From Groups: 95
```

The exact output also includes predicted summaries and `oo` summaries.

## CLI Arguments

```text
grouping_metrics_cli --true-po PATH --true-oo PATH [--pred-po PATH] [--pred-oo PATH]
```

If `--pred-po` or `--pred-oo` is omitted, the corresponding true file is reused
as the prediction input. This makes smoke tests deterministic and simple.

## Phase Boundary

This phase intentionally does not implement:

- `CWVIGEstimator`
- `SoftOverlapDecomposition`
- `GroupingProvider`
- `SharedVariablePolicy`
- any CBOCC, CBOG_CBD, CMAESO, or benchmark-function behavior changes
