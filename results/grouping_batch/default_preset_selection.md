# Default Preset Selection

- Validation errors must be zero.
- Avoid over_shared_ratio close to 1.0.
- Avoid shared count always zero when truth exists.
- Prefer higher SharedVar F1 when truth exists.
- Prefer stable shared count across seeds.
- Prefer lower FE only when grouping metrics are comparable.

Recommended default preset: `capped`.

| Preset | Runs | Truth Runs | Shared Mean | Over-shared Mean | Shared F1 Mean | Jaccard Mean | Validation Mean | FE Mean |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced | 2 | 2 | 5.5 | 0.55 | 0.267857142857 | 0.691666666667 | 0 | 540 |
| capped | 2 | 2 | 5 | 0.5 | 0.285714285714 | 0.458002645503 | 0 | 540 |
| conservative | 2 | 2 | 0 | 0 | 0 | 0.888888888889 | 0 | 540 |

Rationale: the deterministic ranking first removes presets with validation failures or near-total over-sharing, then penalizes presets that always predict zero shared variables when truth metrics are available. Remaining ties prefer higher SharedVar F1, lower shared-count variance, higher group Jaccard, and finally the less cap-driven `balanced` preset.
