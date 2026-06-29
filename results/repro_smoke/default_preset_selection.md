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
| balanced | 2 | 2 | 5.5 | 0.5 | 0.55 | 0 | 0.267857142857 | 0.017857142857 | 0.267857142857 | 0.691666666667 | 0 | 540 |
| capped | 2 | 2 | 5 | 0 | 0.5 | 0 | 0.285714285714 | 0 | 0.285714285714 | 0.458002645503 | 0 | 540 |
| conservative | 2 | 2 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.888888888889 | 0 | 540 |

## Smoke vs Medium

- Phase 5B smoke recommendation: `capped`.
- Phase 5C medium recommendation: `capped`.
- The smoke recommendation still holds under the medium batch.
- `conservative` is treated as too strict when it predicts zero shared variables in every truth-available run.
- `balanced` is preferred over `capped` only when their metrics are close because it has less hard-coded shared-cap prior.

Rationale: the deterministic ranking first removes presets with validation failures or frequent near-total over-sharing, then penalizes presets that always predict zero shared variables when truth metrics are available. Remaining order prefers higher mean SharedVar-F1, lower SharedVar-F1 std, lower shared-count variance, and finally the less cap-driven preset.
