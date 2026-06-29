# CWVIG-OSD

Initial scaffold date: 2026-06-29  
Executor: Codex

CWVIG-OSD is a research scaffold for estimating uncertain variable interaction
graphs from black-box function evaluations, recovering soft overlapping groups
with an affiliation model, and feeding shared-variable confidence into
cooperative coevolution updates.

## Core Idea

The project is organized around three artifacts:

- `W`: weighted variable interaction graph estimated from black-box probes.
- `U`: uncertainty matrix for interaction confidence and downstream weighting.
- `Z`: soft variable-to-group affiliation matrix used to identify overlapping
  components and shared variables.

The intended integration path is:

1. Use `grouping/prob_dg.py` to estimate pairwise interaction probabilities.
2. Use `grouping/weighted_vig.py` to construct and normalize `W` and `U`.
3. Use `grouping/soft_overlap.py` to learn overlapping affiliations `Z`.
4. Use `grouping/bridge_validation.py` to calibrate shared-variable confidence.
5. Pass shared-variable confidence into `cc/soft_cc_update.cpp` for
   shared-variable-aware cooperative coevolution.

## Layout

```text
CWVIG-OSD/
├── benchmark/
│   ├── flyki_overlap/
│   └── cec2013lsgo/
├── grouping/
│   ├── prob_dg.py
│   ├── weighted_vig.py
│   ├── soft_overlap.py
│   └── bridge_validation.py
├── cc/
│   ├── cbcco_adapter.cpp
│   └── soft_cc_update.cpp
├── experiments/
│   ├── grouping_accuracy.py
│   ├── optimization_test.cpp
│   └── statistical_test.py
├── results/
├── tests/
└── docs/
```

## Upstream Baselines

- Main optimization base:
  <https://github.com/Flyki/Large-Scale-Overlapping-Optimization>
- Fast Python validation benchmark:
  <https://github.com/dmolina/cec2013lsgo>

The first scaffold keeps these sources under `benchmark/` so the integration
layer can evolve without rewriting the upstream code immediately.

## Quick Start

Use Python 3.10+.

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -e ".[dev]"
pytest
python experiments/grouping_accuracy.py
```

If Python is managed with `uv`, the equivalent workflow is:

```powershell
uv sync --extra dev
uv run pytest
uv run python experiments/grouping_accuracy.py
```

## Current Status

This is an executable research scaffold, not a finished algorithm release.
The Python grouping path has deterministic toy tests. The C++ files expose
integration boundaries for Flyki/CBCCO and should be wired to concrete upstream
types after inspecting the selected optimization entry point.
