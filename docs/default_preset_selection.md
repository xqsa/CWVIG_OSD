# Default Preset Selection

- Date: 2026-06-29
- Executor: Codex
- Current default candidate: `capped`
- Config file: `configs/cwvig_presets/capped_default.json`

## Meaning

`capped` is frozen as the current CWVIG grouping default candidate for the next optimizer-integration phase. It is not a final full-benchmark default.

The current evidence covers only Flyki `func=1`, `dimension_limit=10,20,50`, seeds `1,3,5,11`, and contexts `3,5`.

## Evidence

Phase 5C medium batch produced:

| Preset | Runs | Shared Mean | Over-shared Mean | SharedVar-F1 Mean | SharedVar-F1 Std | Jaccard Mean | Validation Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `balanced` | 24 | 6.70833333333 | 0.3225 | 0.11323051948 | 0.135540809157 | 0.549394011202 | 0 |
| `capped` | 24 | 12.0833333333 | 0.475 | 0.225159069326 | 0.0849008748789 | 0.539981741639 | 0 |
| `conservative` | 24 | 0 | 0 | 0 | 0 | 0.611591420064 | 0 |

## Why Not Conservative

`conservative` has the highest mean best-group Jaccard, but it predicts zero shared variables in every truth-available medium run. That makes it too strict for an overlapping decomposition default.

## Why Capped Over Balanced

`balanced` has a lower over-shared ratio, but `capped` recovers shared variables better under current truth diagnostics:

- higher mean SharedVar-F1: `0.225159069326` vs `0.11323051948`
- lower SharedVar-F1 std: `0.0849008748789` vs `0.135540809157`
- zero validation errors, same as `balanced`

The cost is that `capped` encodes a stronger shared-variable cap prior. This is acceptable for a current candidate, but it must be rechecked before claiming a final default.

## Limitations

- Only Flyki `func=1` has medium-batch evidence.
- Dimension limit is at most `50`, not full `905D`.
- No optimizer performance result exists yet.
- No SharedVariablePolicy is implemented.
- `CBOG_CBD`, `CMAESO`, and `CBOCC` optimization behavior are unchanged.

## Needed Before Final Default

Before claiming a final default, collect:

- grouping evidence on more Flyki functions;
- at least one larger dimension setting beyond `50`;
- optimizer-level comparison against the original CBCCO path;
- sensitivity checks for `max_shared_ratio`, contexts, and delta;
- confirmation that truth `po/oo` files remain evaluation-only.
