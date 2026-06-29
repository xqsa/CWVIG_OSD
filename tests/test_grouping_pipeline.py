from __future__ import annotations

import numpy as np
from numpy.typing import NDArray

from grouping import (
    PairwiseProbeConfig,
    SoftOverlapConfig,
    build_weighted_vig,
    calibrate_shared_variables,
    estimate_interactions,
    fit_affiliation_model,
)


def toy_overlap_objective(x: NDArray[np.float64]) -> float:
    return float((x[0] + x[1]) ** 2 + (x[1] + x[2]) ** 2 + 0.1 * x[3] ** 2)


def test_probabilistic_dg_detects_known_interactions() -> None:
    estimate = estimate_interactions(
        toy_overlap_objective,
        np.array([[-1.0, 1.0]] * 4),
        PairwiseProbeConfig(probes=6, delta=1e-2, random_seed=7),
    )

    assert estimate.raw_score[0, 1] > estimate.raw_score[0, 3]
    assert estimate.raw_score[1, 2] > estimate.raw_score[0, 3]
    assert estimate.evaluations == 6 * 6 * 4


def test_soft_overlap_pipeline_returns_shared_candidates() -> None:
    estimate = estimate_interactions(
        toy_overlap_objective,
        np.array([[-1.0, 1.0]] * 4),
        PairwiseProbeConfig(probes=8, delta=1e-2, random_seed=11),
    )
    vig = build_weighted_vig(estimate, min_probability=0.5)
    overlap = fit_affiliation_model(vig, SoftOverlapConfig(n_groups=2, random_seed=11))
    bridge = calibrate_shared_variables(overlap, vig, min_shared_confidence=0.0)

    assert overlap.memberships.shape == (4, 2)
    assert np.allclose(overlap.memberships.sum(axis=1), 1.0)
    assert all(0 <= variable < 4 for variable in bridge.shared_variables)
