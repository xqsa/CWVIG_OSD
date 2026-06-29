"""Weighted variable interaction graph construction."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

from grouping.prob_dg import InteractionEstimate


@dataclass(frozen=True)
class WeightedVIG:
    """Weighted interaction graph and uncertainty matrix."""

    weight: NDArray[np.float64]
    uncertainty: NDArray[np.float64]

    @property
    def dimension(self) -> int:
        return int(self.weight.shape[0])

    def edge_list(self, min_weight: float = 0.0) -> list[tuple[int, int, float, float]]:
        """Return upper-triangular edges as ``(i, j, weight, uncertainty)``."""

        edges: list[tuple[int, int, float, float]] = []
        for i in range(self.dimension):
            for j in range(i + 1, self.dimension):
                if self.weight[i, j] >= min_weight:
                    edges.append((i, j, float(self.weight[i, j]), float(self.uncertainty[i, j])))
        return edges


def build_weighted_vig(
    estimate: InteractionEstimate,
    min_probability: float = 0.0,
    normalize: bool = True,
) -> WeightedVIG:
    """Build a symmetric weighted VIG from interaction estimates."""

    probability = np.asarray(estimate.probability, dtype=np.float64)
    uncertainty = np.asarray(estimate.uncertainty, dtype=np.float64)
    _validate_square_pair(probability, uncertainty)

    weight = np.where(probability >= min_probability, probability, 0.0)
    weight = _symmetrize(weight)
    uncertainty = np.clip(_symmetrize(uncertainty), 0.0, 1.0)

    np.fill_diagonal(weight, 0.0)
    np.fill_diagonal(uncertainty, 0.0)

    if normalize:
        max_weight = float(np.max(weight))
        if max_weight > 0:
            weight = weight / max_weight

    return WeightedVIG(weight=weight, uncertainty=uncertainty)


def reliability_weighted_adjacency(vig: WeightedVIG, power: float = 1.0) -> NDArray[np.float64]:
    """Return ``W * (1 - U)^power`` for downstream community learning."""

    if power < 0:
        msg = "power must be non-negative"
        raise ValueError(msg)
    reliability = np.power(1.0 - np.clip(vig.uncertainty, 0.0, 1.0), power)
    weighted = vig.weight * reliability
    np.fill_diagonal(weighted, 0.0)
    return weighted


def _validate_square_pair(
    probability: NDArray[np.float64],
    uncertainty: NDArray[np.float64],
) -> None:
    if probability.ndim != 2 or probability.shape[0] != probability.shape[1]:
        msg = "probability must be a square matrix"
        raise ValueError(msg)
    if uncertainty.shape != probability.shape:
        msg = "uncertainty must have the same shape as probability"
        raise ValueError(msg)
    if not np.all(np.isfinite(probability)) or not np.all(np.isfinite(uncertainty)):
        msg = "probability and uncertainty must contain finite values"
        raise ValueError(msg)


def _symmetrize(matrix: NDArray[np.float64]) -> NDArray[np.float64]:
    return (matrix + matrix.T) / 2.0
