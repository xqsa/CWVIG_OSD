# CBOCC Explicit Grouping Mode

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7A grouping-source selection only; no optimizer behavior change.

## Legacy Mode

The original four-argument command remains valid:

```powershell
cbocco 1 CBCCO 1 1000
```

It defaults to:

```text
grouping_source=legacy_by_function
root=.
po=./1po.txt
oo=./1oo.txt
```

The equivalent explicit legacy form is:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source legacy_by_function
```

Use `--root PATH` only when the legacy `Npo.txt` and `Noo.txt` files are not in the current working directory:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source legacy_by_function --root benchmark/flyki_overlap
```

## Explicit Predicted Mode

Predicted grouping files can be passed directly:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source explicit_files --po results/phase7a_predicted_f1_10d/predicted_groups.txt --oo results/phase7a_predicted_f1_10d/predicted_overlap.txt
```

`--po` is read with the same parser used for legacy `po.txt`, and `--oo` is read with the same parser used for legacy `oo.txt`.

## CWVIG To CBOCC Flow

Generate predicted grouping:

```powershell
cwvig_grouping_pipeline_cli --config configs/cwvig_presets/capped_default.json --mode flyki --func 1 --dimension-limit 10 --seed 1 --contexts 3 --output-dir results/phase7a_predicted_f1_10d
```

Validate predicted grouping:

```powershell
grouping_source_cli --source explicit_files --po results/phase7a_predicted_f1_10d/predicted_groups.txt --oo results/phase7a_predicted_f1_10d/predicted_overlap.txt --print-summary
```

Run `cbocco` with the predicted grouping files:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source explicit_files --po results/phase7a_predicted_f1_10d/predicted_groups.txt --oo results/phase7a_predicted_f1_10d/predicted_overlap.txt
```

## Startup Log

`cbocco` prints these fields before constructing the solver:

- function id
- method
- seed
- maxfes argument
- grouping source
- po path
- oo path
- number of groups
- dimension inferred from grouping
- shared variables
- validation errors

## What Changed

Only the grouping source at the `CBOCC.cpp` entry point changed. Both source modes produce the same `LegacyGroupingView` fields passed into `CBOG_CBD`:

- `groups`
- `overiables`
- `overiablesRedandunt`
- `sharedvar_group_pos`

## What Did Not Change

- No `SharedVariablePolicy` was implemented.
- `CBOG_CBD` still uses its original hard-overlap behavior.
- `CMAESO` sampling and distribution updates were not changed.
- Benchmark function logic was not changed.
- Explicit predicted mode does not silently fall back to true `po/oo`.

## Phase 7A Smoke Evidence

Predicted grouping generation used `dimension_limit=10`, `seed=1`, and `contexts=3`.

Validation summary:

```text
Number Of Groups: 9
Dimension: 10
Unique Variables: 10
Shared Variables: 5
Sharedvar Group Pos Entries: 5
Validation Errors: 0
```

Legacy smoke output was archived under:

```text
results/phase7a_cbocco_legacy/
```

Explicit predicted smoke output was archived under:

```text
results/phase7a_cbocco_predicted/
```

## FE Count Note

The `maxfes` argument is not a strict upper bound on the final logged FE count in the original `CBOG_CBD` flow, because `testStage()` runs before `optimizationStage()` checks `usedFEs < MAXFES`.

Observed smoke runs with `maxfes=1000`:

- Phase 6 legacy smoke reached `162000` FEs.
- Phase 7A legacy smoke reached `162000` FEs.
- Phase 7A explicit predicted 10D smoke reached `2600` FEs.
