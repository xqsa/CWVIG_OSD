# Hard-Overlap Compatibility

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7F pre-optimizer compatibility layer only; no `SharedVariablePolicy`.

## Why Phase 7E Seed 3 Failed

Phase 7E completed the seed 3 predicted grouping to full 905D coverage, and the explicit loader reported `Validation Errors: 0`.
The later `cbocco` run still failed before producing a result file:

```text
cmaes_readpara_ReadFromFile(): No valid dimension N
*** CMA-ES ABORTED ***
```

The cause is the original hard-overlap behavior in `CBOG_CBD::initializeOptimizers()`: any variable present in `sharedvar_group_pos` is removed from every subproblem optimizer dimension. If a group contains only shared variables, its effective optimizer dimension becomes zero, and CMA-ES cannot initialize.

## Effective Group Dimension

For Phase 7F, a group's hard-overlap effective dimension is:

```text
effective_group = primary_group - hard_overlap_shared_variables
```

The hard-overlap shared-variable set is the key set of the legacy `sharedvar_group_pos` map produced from `oo` files. A grouping is hard-overlap compatible when every group that will instantiate a CMA-ES optimizer has `effective_group_size >= 1`.

## Audit Metrics

`hard_overlap_audit_cli` reports:

- group count
- shared variable count
- original group sizes
- effective group sizes after shared-variable removal
- zero-effective group count
- zero-effective group indices
- minimum, maximum, and mean effective group size
- hard-overlap compatible true/false

Command:

```powershell
hard_overlap_audit_cli --po PATH --oo PATH --expected-dimension 905 --print-summary --output-report audit.csv
```

The CSV report is per group:

```text
group_index,original_size,effective_size,zero_effective
```

## Repair Policies

`hard_overlap_sanitize_cli` supports:

- `none`: write the grouping unchanged and report compatibility.
- `drop_empty_groups`: remove zero-effective groups only when this does not drop uniquely covered variables.
- `demote_min_shared`: for the first current zero-effective group, select its smallest shared variable and remove that variable from all `oo` rows. This matches the current global `sharedvar_group_pos` behavior in `CBOG_CBD`.
- `merge_empty_to_nearest`: merge each zero-effective group into the deterministic non-empty group with the largest primary-variable intersection; ties use the lowest group index.

Command:

```powershell
hard_overlap_sanitize_cli --po PATH --oo PATH --expected-dimension 905 --repair-policy demote_min_shared --output-po repaired_po.txt --output-oo repaired_oo.txt --output-report repair_actions.csv --print-summary
```

Every repair action is logged. The sanitizer does not run automatically inside `cbocco`.

## CBOCC Guardrail

`cbocco` now audits hard-overlap compatibility before constructing `CBOG_CBD` and prints:

```text
Zero Effective Group Count: N
Hard-Overlap Compatible: true|false
```

New flags:

```text
--require-hard-overlap-compatible true|false
--allow-hard-overlap-incompatible true|false
```

Strict explicit mode fails before optimizer construction when the grouping is incompatible:

```powershell
cbocco 1 CBCCO 3 1000 --grouping-source explicit_files --po completed_po.txt --oo completed_oo.txt --require-full-coverage true --require-hard-overlap-compatible true
```

Debug mode can intentionally allow an incompatible run:

```powershell
cbocco 1 CBCCO 3 1000 --grouping-source explicit_files --po completed_po.txt --oo completed_oo.txt --allow-hard-overlap-incompatible true
```

This remains a pre-optimizer guardrail. It does not change `CBOG_CBD` optimizer logic, CMA-ES sampling, or benchmark functions.

## Phase 7F Smoke

Command:

```powershell
.\scripts\run_hard_overlap_sanitizer_smoke.ps1 -CMakeExe "$env:USERPROFILE\scoop\shims\cmake.exe"
```

Outputs:

```text
results/phase7f_hard_overlap_sanitizer/completed_hard_overlap_audit.txt
results/phase7f_hard_overlap_sanitizer/completed_hard_overlap_audit.csv
results/phase7f_hard_overlap_sanitizer/strict_incompatible/
results/phase7f_hard_overlap_sanitizer/sanitize_summary.txt
results/phase7f_hard_overlap_sanitizer/repair_actions.csv
results/phase7f_hard_overlap_sanitizer/repaired_groups.txt
results/phase7f_hard_overlap_sanitizer/repaired_overlap.txt
results/phase7f_hard_overlap_sanitizer/repaired_loader_summary.txt
results/phase7f_hard_overlap_sanitizer/repaired_cbocco/1.3.100.CBOG-CBD.result.txt
```

Observed before repair:

```text
Hard-Overlap Group Count: 904
Hard-Overlap Shared Variables: 5
Zero Effective Group Count: 3
Zero Effective Group Indices: 2 4 5
Hard-Overlap Compatible: false
```

Strict incompatible `cbocco` failed before optimizer construction:

```text
Grouping is hard-overlap incompatible: zero_effective_group_count=3.
```

Observed after `demote_min_shared`:

```text
Hard-Overlap Shared Variables: 3
Zero Effective Group Count: 0
Min Effective Group Size: 1
Hard-Overlap Compatible: true
```

Repair actions:

```text
demote_min_shared group=2 variable=0 removed variable from 5 overlap rows
demote_min_shared group=4 variable=1 removed variable from 3 overlap rows
```

The repaired grouping passed explicit loader validation:

```text
Validation Errors: 0
```

The repaired strict `cbocco` run exited `0` and produced:

```text
results/phase7f_hard_overlap_sanitizer/repaired_cbocco/1.3.100.CBOG-CBD.result.txt
```

Final logged row:

```text
182200,1.60771e+10
```

## Interpretation Limits

Phase 7F proves that predicted full-coverage `po/oo` files can be audited and made compatible with the original hard-overlap optimizer entry path. It does not prove optimization superiority.

The sanitizer changes the grouping input before optimization only when explicitly invoked. It is not a shared-variable-aware update policy, and it does not replace the original CBCCO baseline.
