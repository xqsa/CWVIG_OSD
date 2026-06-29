"""Run a minimal grouping pipeline on a toy overlapping objective."""

from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from numpy.typing import NDArray

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from grouping import (
    PairwiseProbeConfig,
    SoftOverlapConfig,
    build_weighted_vig,
    calibrate_shared_variables,
    estimate_interactions,
    fit_affiliation_model,
)
from grouping.bridge_validation import shared_variable_table


RESULT_PATH = ROOT / "results" / "toy_grouping_summary.json"


def toy_overlap_objective(x: NDArray[np.float64]) -> float:
    """Toy function with variable 1 shared by two interacting components."""

    return float((x[0] + x[1]) ** 2 + (x[1] + x[2]) ** 2 + 0.1 * x[3] ** 2)


def main() -> None:
    bounds = np.array([[-1.0, 1.0]] * 4)
    estimate = estimate_interactions(
        toy_overlap_objective,
        bounds,
        PairwiseProbeConfig(probes=8, delta=1e-2, random_seed=42),
    )
    vig = build_weighted_vig(estimate, min_probability=0.5)
    overlap = fit_affiliation_model(
        vig,
        SoftOverlapConfig(n_groups=2, random_seed=42, membership_threshold=0.2),
    )
    bridge = calibrate_shared_variables(overlap, vig, min_shared_confidence=0.6)

    summary = {
        "evaluations": estimate.evaluations,
        "threshold": estimate.threshold,
        "temperature": estimate.temperature,
        "groups": [list(group) for group in overlap.groups],
        "shared_variables": list(bridge.shared_variables),
        "shared_table": shared_variable_table(bridge),
        "reconstruction_error": overlap.reconstruction_error,
    }
    RESULT_PATH.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
