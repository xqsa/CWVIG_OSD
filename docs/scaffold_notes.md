# Scaffold Notes

Date: 2026-06-29  
Executor: Codex

## Decisions

- Keep upstream repositories intact under `benchmark/` during the first phase.
- Put new probabilistic grouping logic in pure Python with NumPy only.
- Keep C++ cooperative coevolution integration as a narrow adapter layer under
  `cc/` until the exact Flyki entry point is selected.
- Treat generated experiment outputs as reproducible artifacts under `results/`;
  source code and configuration stay in Git.

## Validation

- Python smoke tests cover a toy overlapping objective.
- `experiments/grouping_accuracy.py` runs the full path:
  black-box probing -> weighted graph -> soft affiliation -> shared-variable
  calibration.

## Open Integration Work

- Map Flyki's concrete CBCCO data structures into `cc/cbcco_adapter.cpp`.
- Add wrappers for selected CEC2013 LSGO functions once the desired subset is
  chosen.
- Replace the simple symmetric NMF affiliation learner with a stronger model if
  experiments show unstable grouping on high-dimensional functions.
