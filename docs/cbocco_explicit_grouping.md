# CBOCC Explicit Grouping Mode

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7A grouping-source selection plus Phase 7B coverage guardrails; no optimizer behavior change.

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

## Coverage Guardrails

Phase 7B adds explicit coverage policy flags:

```text
--require-full-coverage true|false
--allow-partial-grouping true|false
--completion-policy none|singletons|tail_group
```

Default behavior is conservative:

- legacy full coverage runs normally.
- partial grouping is rejected unless `--allow-partial-grouping true` is explicitly set.
- `--require-full-coverage true` rejects partial grouping even when the source is explicit.
- `--completion-policy singletons` or `--completion-policy tail_group` completes missing variables before the coverage guard.
- out-of-range variables are always rejected.

Phase 7F adds hard-overlap compatibility guardrails for full-coverage files whose groups may become zero-dimensional after original shared-variable removal:

```text
--require-hard-overlap-compatible true|false
--allow-hard-overlap-incompatible true|false
```

Use `hard_overlap_audit_cli` before optimization to inspect `Zero Effective Group Count`. Use `hard_overlap_sanitize_cli` explicitly when a predicted grouping needs repair; `cbocco` does not silently sanitize inputs.

Strict legacy smoke:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source legacy_by_function --root . --require-full-coverage true
```

Strict predicted 10D smoke fails before optimization:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source explicit_files --po results/phase7a_predicted_f1_10d/predicted_groups.txt --oo results/phase7a_predicted_f1_10d/predicted_overlap.txt --require-full-coverage true
```

Smoke-only partial predicted run:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source explicit_files --po results/phase7a_predicted_f1_10d/predicted_groups.txt --oo results/phase7a_predicted_f1_10d/predicted_overlap.txt --allow-partial-grouping true
```

Completed predicted run:

```powershell
cbocco 1 CBCCO 1 1000 --grouping-source explicit_files --po results/phase7a_predicted_f1_10d/predicted_groups.txt --oo results/phase7a_predicted_f1_10d/predicted_overlap.txt --completion-policy singletons --require-full-coverage true
```

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
- expected benchmark dimension
- original inferred grouping dimension
- original covered unique variables
- original missing variable count
- completion policy
- whether partial grouping is allowed
- whether full coverage is required
- whether hard-overlap compatibility is required
- whether hard-overlap incompatibility is explicitly allowed
- number of groups
- applied grouping dimension
- covered unique variables
- missing variable count
- duplicate variable occurrences
- out-of-range variables
- coverage ratio
- full coverage true/false
- shared variables
- zero effective group count
- hard-overlap compatible true/false
- validation errors

## What Changed

`CBOCC.cpp` now selects the grouping source, audits coverage, optionally completes missing variables, and then passes the resulting `LegacyGroupingView` fields into `CBOG_CBD`:

- `groups`
- `overiables`
- `overiablesRedandunt`
- `sharedvar_group_pos`

Coverage audit and completion run before solver construction. They do not alter CMA-ES sampling, optimizer state updates, or the hard-overlap logic inside `CBOG_CBD`.
Hard-overlap compatibility audit also runs before solver construction. It only reports or rejects incompatible grouping when strict flags request it.

## What Did Not Change

- No `SharedVariablePolicy` was implemented.
- `CBOG_CBD` still uses its original hard-overlap behavior.
- `CMAESO` sampling and distribution updates were not changed.
- Benchmark function logic was not changed.
- Explicit predicted mode does not silently fall back to true `po/oo`.
- Missing-variable completion is separate from the optimizer entry path and preserves existing groups.

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

## Phase 7B Smoke Evidence

Local verification artifacts:

```text
results/phase7b_coverage_audit/
results/phase7b_cbocco_legacy_full/
results/phase7b_cbocco_predicted_partial/
results/phase7b_cbocco_predicted_completed/
```

Legacy strict startup coverage:

```text
Expected Benchmark Dimension: 905
Original Grouping Inferred Dimension: 905
Covered Unique Variables: 905
Missing Variable Count: 0
Coverage Ratio: 1
Full Coverage: true
Validation Errors: 0
```

Predicted 10D strict failure:

```text
cbocco: Grouping coverage is partial: missing_variable_count=895, out_of_range_variables=0, coverage_ratio=0.0110497.
```

Predicted 10D partial allow warning:

```text
WARNING: Grouping coverage is partial: missing_variable_count=895, out_of_range_variables=0, coverage_ratio=0.0110497. The run is allowed only because --allow-partial-grouping true was set.
```

Predicted 10D singleton completion startup coverage:

```text
Completion Policy: singletons
Number Of Groups: 904
Grouping Inferred Dimension: 905
Covered Unique Variables: 905
Missing Variable Count: 0
Coverage Ratio: 1
Full Coverage: true
Validation Errors: 0
```
