"""Shared-variable calibration for overlapping decompositions."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

from grouping.soft_overlap import SoftOverlapResult
from grouping.weighted_vig import WeightedVIG


@dataclass(frozen=True)
class BridgeValidationResult:
    """Calibrated shared-variable scores."""

    shared_variables: tuple[int, ...]
    confidence: NDArray[np.float64]
    bridge_score: NDArray[np.float64]
    threshold: float


def calibrate_shared_variables(
    overlap: SoftOverlapResult,
    vig: WeightedVIG,
    min_shared_confidence: float = 0.25,
    bridge_quantile: float = 0.65,
) -> BridgeValidationResult:
    """Select variables likely to be reliable bridges between overlapping groups."""

    memberships = overlap.memberships
    if memberships.shape[0] != vig.dimension:
        msg = "membership rows must match VIG dimension"
        raise ValueError(msg)
    if not 0 <= min_shared_confidence <= 1:
        msg = "min_shared_confidence must be in [0, 1]"
        raise ValueError(msg)
    if not 0 <= bridge_quantile <= 1:
        msg = "bridge_quantile must be in [0, 1]"
        raise ValueError(msg)

    bridge_score = _bridge_score(memberships, vig.weight)
    threshold = float(np.quantile(bridge_score, bridge_quantile)) if bridge_score.size else 0.0
    confidence = np.clip((overlap.shared_confidence + bridge_score) / 2.0, 0.0, 1.0)
    shared_variables = tuple(
        int(index)
        for index in np.flatnonzero(
            (overlap.shared_confidence >= min_shared_confidence) & (bridge_score >= threshold)
        )
    )
    return BridgeValidationResult(
        shared_variables=shared_variables,
        confidence=confidence,
        bridge_score=bridge_score,
        threshold=threshold,
    )


def shared_variable_table(result: BridgeValidationResult) -> list[dict[str, float | int]]:
    """Return a lightweight serializable table for reports and logs."""

    return [
        {
            "variable": variable,
            "confidence": float(result.confidence[variable]),
            "bridge_score": float(result.bridge_score[variable]),
        }
        for variable in result.shared_variables
    ]


def _bridge_score(
    memberships: NDArray[np.float64],
    weight: NDArray[np.float64],
) -> NDArray[np.float64]:
    if memberships.shape[1] == 1:
        return np.zeros(memberships.shape[0], dtype=np.float64)

    cross_group_tension = np.zeros(memberships.shape[0], dtype=np.float64)
    primary = np.argmax(memberships, axis=1)
    for variable in range(memberships.shape[0]):
        neighbors = np.flatnonzero(weight[variable] > 0)
        if neighbors.size == 0:
            continue
        cross_edges = neighbors[primary[neighbors] != primary[variable]]
        cross_group_tension[variable] = float(np.sum(weight[variable, cross_edges]))

    max_score = float(np.max(cross_group_tension))
    if max_score <= 0:
        return cross_group_tension
    return cross_group_tension / max_score
