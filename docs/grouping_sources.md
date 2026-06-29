# Grouping Sources

Date: 2026-06-29  
Executor: Codex  
Scope: Phase 2C predicted grouping source support.

## Source Modes

Both modes return the same `LegacyGroupingView` used by the CBOCC seam:

- `legacy_by_function`: resolves `<root>/<func>po.txt` and `<root>/<func>oo.txt`.
- `explicit_files`: reads caller-provided `po` and `oo` paths, intended for
  predicted grouping outputs.

Both modes go through:

```text
GroupingIO -> LegacyGroupingAdapter -> validateLegacyGroupingView
```

No optimizer logic is changed.

## File Format

Files use the existing counted-group format:

```text
<group_size>
<var_id_0> <var_id_1> ...
<group_size>
<var_id_0> <var_id_1> ...
```

Variable ids remain 0-based. Future CWVIG-OSD outputs should write:

```text
predicted_groups.txt
predicted_overlap.txt
```

in this same format.

## CLI

Target:

```text
grouping_source_cli
```

Legacy mode:

```powershell
.\build\flyki\Release\grouping_source_cli.exe --source legacy_by_function --func 1 --root benchmark\flyki_overlap --print-summary --dump-shared-map .codex\phase2c_legacy_map.txt
```

Explicit file mode:

```powershell
.\build\flyki\Release\grouping_source_cli.exe --source explicit_files --po benchmark\flyki_overlap\1po.txt --oo benchmark\flyki_overlap\1oo.txt --print-summary --dump-shared-map .codex\phase2c_explicit_map.txt
```

Expected output for both:

```text
Number Of Groups: 20
Dimension: 905
Unique Variables: 905
Shared Variables: 95
Sharedvar Group Pos Entries: 95
Validation Errors: 0
```

Equivalence check:

```powershell
Get-FileHash .codex\phase2c_legacy_map.txt -Algorithm SHA256
Get-FileHash .codex\phase2c_explicit_map.txt -Algorithm SHA256
```

Expected hash:

```text
A42A546503BF724CC4D2D33262B25E3F68F6989F30ACE649CD0C69318F021F55
```

## Sample Predicted Files

Sample files live in:

```text
examples/predicted_grouping/predicted_groups.txt
examples/predicted_grouping/predicted_overlap.txt
```

They currently mirror `benchmark/flyki_overlap/1po.txt` and `1oo.txt` to keep
the predicted-file path behavior-equivalent while CWVIG-OSD generation is still
absent.

## Errors

The loader and CLI report:

- `Invalid grouping source mode`
- `Missing explicit po file`
- `Missing explicit oo file`
- validation failures such as overlap variables missing from their primary group

## Build And Test

From repository root:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target grouping_source_cli --config Release
cmake --build build/flyki --target grouping_source_tests --config Release
.\build\flyki\Release\grouping_source_tests.exe
```

`cbocco` still depends on the missing external CMA-ES files and is not required
for this phase.
