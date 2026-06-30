# Grouping Coverage Audit

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7B coverage audit, completion utility, and `cbocco` startup guardrails.

## Purpose

`grouping_coverage_cli` audits whether a `po/oo` grouping covers the expected Flyki overlap benchmark dimension before the grouping is passed into `CBOG_CBD`.

It reports:

- expected benchmark dimension
- inferred grouping dimension
- covered unique variables
- missing variable count
- duplicate variable occurrences
- out-of-range variables
- coverage ratio
- full coverage true/false
- loader validation error count

## Expected Dimension

For Flyki overlap F1-F12, the expected dimension is `905`.

The audit helper validates that the function id is in `1..12` and then uses the same `905` dimension assumed by the Flyki overlap benchmark classes and `probe/BenchmarkEvaluator.cpp`. It does not construct a benchmark object only to obtain dimension, because the original Flyki destructors free lazily initialized fields and can fail on early-exit paths before any function evaluation.

## CLI Usage

Audit only:

```powershell
.\build\flyki_phase7b\grouping_coverage_cli.exe --po benchmark\flyki_overlap\1po.txt --oo benchmark\flyki_overlap\1oo.txt --expected-dimension 905 --print-summary
```

Audit and complete missing variables as singleton groups:

```powershell
.\build\flyki_phase7b\grouping_coverage_cli.exe --po results\phase7a_predicted_f1_10d\predicted_groups.txt --oo results\phase7a_predicted_f1_10d\predicted_overlap.txt --expected-dimension 905 --completion-policy singletons --output-po results\phase7b_coverage_audit\predicted_10d_completed_singletons_po.txt --output-oo results\phase7b_coverage_audit\predicted_10d_completed_singletons_oo.txt --print-summary
```

Supported completion policies:

- `none`: audit only, no missing-variable completion.
- `singletons`: preserve existing groups and append one singleton group per missing variable.
- `tail_group`: preserve existing groups and append one final group containing all missing variables.

Completion regenerates compatible legacy overlap structures from the resulting `po/oo` files. Variable ids remain 0-based.

## Phase 7B Evidence

Legacy F1 grouping:

```text
Expected Dimension: 905
Grouping Inferred Dimension: 905
Covered Unique Variables: 905
Missing Variable Count: 0
Duplicate Variable Occurrences: 95
Out Of Range Variables: 0
Coverage Ratio: 1
Full Coverage: true
Validation Errors: 0
```

Phase 7A 10D predicted grouping:

```text
Expected Dimension: 905
Grouping Inferred Dimension: 10
Covered Unique Variables: 10
Missing Variable Count: 895
Duplicate Variable Occurrences: 18
Out Of Range Variables: 0
Coverage Ratio: 0.0110497237569
Full Coverage: false
Validation Errors: 0
```

After singleton completion:

```text
Expected Dimension: 905
Grouping Inferred Dimension: 905
Covered Unique Variables: 905
Missing Variable Count: 0
Duplicate Variable Occurrences: 18
Out Of Range Variables: 0
Coverage Ratio: 1
Full Coverage: true
Validation Errors: 0
```

Local result files:

```text
results/phase7b_coverage_audit/legacy_f1_summary.txt
results/phase7b_coverage_audit/predicted_10d_summary.txt
results/phase7b_coverage_audit/predicted_10d_completed_singletons_summary.txt
results/phase7b_coverage_audit/predicted_10d_completed_singletons_po.txt
results/phase7b_coverage_audit/predicted_10d_completed_singletons_oo.txt
results/phase7b_coverage_audit/predicted_10d_completed_singletons_loader_summary.txt
```
