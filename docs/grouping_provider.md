# Legacy Grouping Provider

Date: 2026-06-29  
Executor: Codex  
Scope: Phase 2A legacy grouping adapter equivalence.

## Purpose

Phase 2A adds a reusable adapter that reproduces the grouping structures built
inside the original `CBOCC.cpp`, without modifying `CBOCC.cpp`, `CBOG_CBD.*`,
`CMAESO.*`, or benchmark functions.

The adapter is independent of CMA-ES and only uses C++17 standard library code.

## Original CBOCC Construction

The original `CBOCC.cpp` reads `<func>po.txt` and `<func>oo.txt`, then builds:

- `groups`: primary groups from `po`.
- `overiablesRedandunt`: raw overlap groups from `oo`, including duplicates.
- `overiables`: first-seen shared variables. Each `oo` group contributes only
  variables not seen in earlier `oo` groups; empty contributions are skipped.
- `sharedvar_group_pos`: map from overlap variable id to every
  `(group_id, position_in_group)` pair where that variable appears in its
  corresponding primary group.

Variable ids stay 0-based. Duplicate overlap entries are preserved in
`overiablesRedandunt`, and they produce duplicate position pairs in
`sharedvar_group_pos`, matching the nested loops in `CBOCC.cpp`.

## Adapter Data Structures

Implementation:

```text
benchmark/flyki_overlap/grouping/LegacyGroupingAdapter.h
benchmark/flyki_overlap/grouping/LegacyGroupingAdapter.cpp
```

`LegacyGroupingView` contains:

- `groups`
- `overlap_groups`
- `overiablesRedandunt`
- `overiables`
- `sharedvar_group_pos`
- `dimension`
- `number_of_groups`

The main entry point is:

```cpp
LegacyGroupingView makeLegacyGroupingView(const GroupingData &data);
```

Use `readPoFile`, `readOoFile`, and `makeGroupingData` from Phase 1 to create
the input `GroupingData`.

## Validation Rules

`validateLegacyGroupingView(view)` returns a list of validation errors. An
empty list means the view is compatible with the legacy grouping construction.

Checks:

- `number_of_groups == groups.size()`
- `groups.size() == overiablesRedandunt.size()`
- `overlap_groups == overiablesRedandunt`
- `overiables` matches legacy first-seen construction
- no negative variable ids
- every raw overlap variable appears in the corresponding primary group
- every `sharedvar_group_pos` pair is in range and points to the mapped variable
- inferred dimension equals `max_variable_id + 1`

If an explicit dimension is used by a caller, pass `true` as the second
argument; validation then checks only that the dimension is large enough.

## CLI

Target:

```text
grouping_provider_cli
```

Arguments:

```text
grouping_provider_cli --po PATH --oo PATH [--print-summary] [--dump-shared-map PATH]
```

Summary output includes:

- number of groups
- dimension
- number of unique variables
- number of shared variables
- number of `sharedvar_group_pos` entries
- validation error count

The shared map dump is deterministic because it iterates the ordered
`std::map<int, ...>` and preserves the legacy position insertion order.

## Build And Test Commands

From repository root:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target grouping_provider_cli --config Release
cmake --build build/flyki --target grouping_provider_tests --config Release
.\build\flyki\Release\grouping_provider_tests.exe
```

Smoke command:

```powershell
.\build\flyki\Release\grouping_provider_cli.exe --po benchmark\flyki_overlap\1po.txt --oo benchmark\flyki_overlap\1oo.txt --print-summary --dump-shared-map .codex\phase2a_shared_map.txt
```

Expected key output:

```text
Number Of Groups: 20
Dimension: 905
Unique Variables: 905
Shared Variables: 95
Sharedvar Group Pos Entries: 95
Validation Errors: 0
```

Observed deterministic dump hash during Phase 2A:

```text
SHA256 A42A546503BF724CC4D2D33262B25E3F68F6989F30ACE649CD0C69318F021F55
```

## Phase Boundary

This phase does not implement CWVIG-OSD, `CWVIGEstimator`,
`SoftOverlapDecomposition`, or `SharedVariablePolicy`, and it does not connect
the adapter into `CBOCC.cpp` yet.
