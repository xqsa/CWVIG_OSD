# CBOCC Grouping Loader

Date: 2026-06-29  
Executor: Codex  
Scope: Phase 2B narrow CBOCC grouping-loading seam.

## Purpose

Phase 2B extracts CBOCC's legacy po/oo grouping load into a testable loader.
It does not implement CWVIG-OSD and does not change optimization logic.

## Old Inline Behavior

Original `CBOCC.cpp` directly:

- opened `${func}po.txt` from the current working directory;
- read 20 primary groups into `groups`;
- opened `${func}oo.txt`;
- built `overiablesRedandunt` from raw overlap groups;
- built `overiables` from first-seen shared variables;
- built `sharedvar_group_pos` by scanning each overlap variable's position in
  its corresponding primary group;
- passed those structures to `CBOG_CBD`.

## New Loader API

Files:

```text
benchmark/flyki_overlap/grouping/CBOCCGroupingLoader.h
benchmark/flyki_overlap/grouping/CBOCCGroupingLoader.cpp
```

API:

```cpp
LegacyGroupingView loadLegacyGroupingForFunction(
    int func,
    const std::filesystem::path &benchmark_root);
```

It resolves:

```text
<benchmark_root>/<func>po.txt
<benchmark_root>/<func>oo.txt
```

Then it calls `GroupingIO` and `LegacyGroupingAdapter`, validates the result,
and returns the same `LegacyGroupingView` fields used by `CBOG_CBD`.

## Equivalence

`CBOCC.cpp` now replaces only the old inline grouping block with:

```cpp
const auto grouping = flyki::grouping::loadLegacyGroupingForFunction(func, ".");
```

The `CBOG_CBD` call still receives:

- `grouping.groups`
- `grouping.overiables`
- `grouping.overiablesRedandunt`
- `grouping.sharedvar_group_pos`

The original `DIM = 905` path is unchanged.

## Errors

The loader reports:

- invalid function ids outside `1..12`;
- missing po file;
- missing oo file;
- validation failures from `LegacyGroupingAdapter`.

## CLI

Target:

```text
cbocco_grouping_loader_cli
```

Usage:

```powershell
.\build\flyki\Release\cbocco_grouping_loader_cli.exe --func 1 --root benchmark\flyki_overlap --print-summary --dump-shared-map .codex\phase2b_shared_map.txt
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

The shared map dump is deterministic. Phase 2B observed:

```text
SHA256 A42A546503BF724CC4D2D33262B25E3F68F6989F30ACE649CD0C69318F021F55
```

## Build And Test

From repository root:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target cbocco_grouping_loader_cli --config Release
cmake --build build/flyki --target cbocco_grouping_loader_tests --config Release
.\build\flyki\Release\cbocco_grouping_loader_tests.exe
```

`cbocco` still requires the original external CMA-ES dependency. Without it,
the target should continue to fail with the documented missing files:

```text
cmaes_interface.h
boundary_transformation.h
CMA-ES implementation source files
```

That blocker is unchanged by Phase 2B.
