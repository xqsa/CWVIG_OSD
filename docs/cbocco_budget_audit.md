# CBOCC FE Budget Audit

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 7D diagnostic audit only; no optimizer behavior change.

## Code Path

`CBOCC.cpp` parses the fourth positional argument into `options.maxfes` and passes it unchanged to:

```text
CBOG_CBD(fp, DIM, seed, 100, groups, overiables, overiablesRedandunt, sharedvar_group_pos, maxfes)
```

Inside `CBOG_CBD`, that value is stored as `MAXFES`. It is checked only in `optimizationStage()`:

```text
while (usedFEs < MAXFES)
```

Before that check, `CBOCC.cpp` always calls:

```text
oneSolver.testStage()
oneSolver.optimizationStage()
```

`testStage()` runs `testRound=100` rounds. In each round it visits every optimizer/group and calls `CMAESO::calculateFitness()`. Each `calculateFitness()` increments `usedFEs` once per CMA-ES population member. Result rows are written as:

```text
usedFEs,gbestf
```

So the command-line `maxfes` is not a strict global total FE budget. It is an optimization-stage entry condition checked after the fixed test stage has already consumed evaluations.

## Why Grouping Changes Final FE

The test-stage FE per row depends on the active optimizers:

- legacy F1 grouping: `groups=20`, `shared_variables=95`, observed FE increment `1620`
- singleton-completed predicted grouping: `groups=904`, `shared_variables=5`, observed FE increment `1816`

Because `maxfes=500`, `1000`, and `2000` are all smaller than the test-stage FE already consumed in the first logged row, these sweep runs do not enter meaningful optimization-stage budget control. They produce 100 test-stage rows and final FE determined by the grouping-dependent test-stage cost.

## Phase 7D Sweep

Command:

```powershell
.\scripts\run_cbocco_budget_audit.ps1 -CMakeExe "$env:USERPROFILE\scoop\shims\cmake.exe" -PythonExe "C:\Users\83718\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
```

Outputs:

```text
results/phase7d_budget_audit/budget_summary.csv
results/phase7d_budget_audit/budget_audit_report.md
results/phase7d_budget_audit/legacy_maxfes500/
results/phase7d_budget_audit/completed_predicted_maxfes500/
results/phase7d_budget_audit/legacy_maxfes1000/
results/phase7d_budget_audit/completed_predicted_maxfes1000/
results/phase7d_budget_audit/legacy_maxfes2000/
results/phase7d_budget_audit/completed_predicted_maxfes2000/
```

Observed final FE:

```text
legacy_maxfes500: final_fe=162000
completed_predicted_maxfes500: final_fe=181600
legacy_maxfes1000: final_fe=162000
completed_predicted_maxfes1000: final_fe=181600
legacy_maxfes2000: final_fe=162000
completed_predicted_maxfes2000: final_fe=181600
```

## Fair-Comparison Policy

Use both views:

- Equal command `maxfes` is useful for reproducing the current CBCCO interface.
- Equal final FE is required before interpreting fitness differences as optimization performance evidence.

When final FE differs, report:

- `final_fe_difference`
- `relative_fe_difference`
- an explicit “not FE-matched” warning

Do not claim improvement from non-FE-matched runs. Phase 7C does not prove CWVIG optimization superiority because completed-predicted used `181600` final FE while legacy used `162000` final FE.

## Future Protocol

For multi-seed comparisons:

1. Keep legacy CBCCO as the unchanged baseline.
2. Record command `maxfes`, final FE, logged rows, groups, shared variables, and completion policy.
3. Compare at equal command `maxfes` as an interface smoke check.
4. Separately compare only FE-matched or FE-normalized results for performance claims.
5. Do not introduce `SharedVariablePolicy` conclusions until that policy exists and has its own matched-FE protocol.
