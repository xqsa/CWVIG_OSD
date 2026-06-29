# Default Preset Selection

- Validation errors must be zero.
- Avoid over_shared_ratio close to 1.0.
- Avoid shared count always zero when truth exists.
- Prefer higher SharedVar F1 when truth exists.
- Prefer lower SharedVar-F1 std when means are close.
- Prefer stable shared count across seeds.
- Prefer lower FE only when grouping metrics are comparable.

Recommended default preset: `capped`.

| Preset | Runs | Truth Runs | Shared Mean | Shared Std | Over-shared Mean | Over >= 0.8 | Shared F1 Mean | Shared F1 Std | Shared F1 Median | Jaccard Mean | Validation Total | FE Mean |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced | 24 | 24 | 6.70833333333 | 1.96806094644 | 0.3225 | 0 | 0.11323051948 | 0.135540809157 | 0.142857142857 | 0.549394011202 | 0 | 7786.66666667 |
| capped | 24 | 24 | 12.0833333333 | 6.80634916008 | 0.475 | 0 | 0.225159069326 | 0.0849008748789 | 0.230769230769 | 0.539981741639 | 0 | 7786.66666667 |
| conservative | 24 | 24 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.611591420064 | 0 | 7786.66666667 |

## Smoke vs Medium

- Phase 5B smoke recommendation: `capped`.
- Phase 5C medium recommendation: `capped`.
- The smoke recommendation still holds under the medium batch.
- `conservative` is treated as too strict when it predicts zero shared variables in every truth-available run.
- `balanced` is preferred over `capped` only when their metrics are close because it has less hard-coded shared-cap prior.

Rationale: the deterministic ranking first removes presets with validation failures or frequent near-total over-sharing, then penalizes presets that always predict zero shared variables when truth metrics are available. Remaining order prefers higher mean SharedVar-F1, lower SharedVar-F1 std, lower shared-count variance, and finally the less cap-driven preset.
