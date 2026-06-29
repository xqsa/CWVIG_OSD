"""Probabilistic differential grouping from black-box evaluations.

The implementation is intentionally small and deterministic. It estimates the
strength of pairwise interactions with second-order finite differences, then
maps robustly scaled scores to probabilities and uncertainty values.
"""

from __future__ import annotations

from collections.abc import Callable, Iterable
from dataclasses import dataclass
from itertools import combinations

import numpy as np
from numpy.typing import ArrayLike, NDArray


Objective = Callable[[NDArray[np.float64]], float]
Pair = tuple[int, int]


@dataclass(frozen=True)
class PairwiseProbeConfig:
    """Configuration for pairwise black-box probes."""

    probes: int = 5
    delta: float = 1e-3
    random_seed: int = 0
    threshold: float | None = None
    temperature: float | None = None
    max_pairs: int | None = None


@dataclass(frozen=True)
class InteractionEstimate:
    """Pairwise interaction probability and uncertainty matrices."""

    probability: NDArray[np.float64]
    uncertainty: NDArray[np.float64]
    raw_score: NDArray[np.float64]
    pairs_evaluated: tuple[Pair, ...]
    evaluations: int
    threshold: float
    temperature: float


def estimate_interactions(
    objective: Objective,
    bounds: ArrayLike,
    config: PairwiseProbeConfig | None = None,
    candidate_pairs: Iterable[Pair] | None = None,
) -> InteractionEstimate:
    """Estimate pairwise interaction probabilities for a bounded objective.

    Args:
        objective: Callable that maps a vector to a scalar objective value.
        bounds: Array-like of shape ``(dimension, 2)``.
        config: Probe configuration. Defaults are conservative smoke-test values.
        candidate_pairs: Optional subset of variable pairs to probe.

    Returns:
        InteractionEstimate with symmetric ``probability``, ``uncertainty`` and
        ``raw_score`` matrices.
    """

    cfg = config or PairwiseProbeConfig()
    bounds_arr = _validate_bounds(bounds)
    dimension = bounds_arr.shape[0]
    pairs = _select_pairs(dimension, cfg, candidate_pairs)

    probability = np.zeros((dimension, dimension), dtype=np.float64)
    uncertainty = np.ones((dimension, dimension), dtype=np.float64)
    raw_score = np.zeros((dimension, dimension), dtype=np.float64)

    if not pairs:
        return InteractionEstimate(
            probability=probability,
            uncertainty=uncertainty,
            raw_score=raw_score,
            pairs_evaluated=(),
            evaluations=0,
            threshold=0.0,
            temperature=1.0,
        )

    rng = np.random.default_rng(cfg.random_seed)
    steps = _steps_from_bounds(bounds_arr, cfg.delta)
    base_points = _sample_base_points(bounds_arr, steps, cfg.probes, rng)

    scores: list[float] = []
    score_by_pair: dict[Pair, float] = {}
    evaluations = 0

    for i, j in pairs:
        values = []
        for base in base_points:
            interaction, scale = _second_order_delta(objective, base, steps, i, j)
            evaluations += 4
            values.append(abs(interaction) / scale)
        score = float(np.mean(values))
        scores.append(score)
        score_by_pair[(i, j)] = score

    threshold, temperature = _robust_probability_params(np.asarray(scores), cfg)

    for i, j in pairs:
        prob = _sigmoid((score_by_pair[(i, j)] - threshold) / temperature)
        unc = 1.0 - abs(2.0 * prob - 1.0)
        probability[i, j] = probability[j, i] = prob
        uncertainty[i, j] = uncertainty[j, i] = unc
        raw_score[i, j] = raw_score[j, i] = score_by_pair[(i, j)]

    np.fill_diagonal(uncertainty, 0.0)
    return InteractionEstimate(
        probability=probability,
        uncertainty=uncertainty,
        raw_score=raw_score,
        pairs_evaluated=tuple(pairs),
        evaluations=evaluations,
        threshold=threshold,
        temperature=temperature,
    )


def _validate_bounds(bounds: ArrayLike) -> NDArray[np.float64]:
    bounds_arr = np.asarray(bounds, dtype=np.float64)
    if bounds_arr.ndim != 2 or bounds_arr.shape[1] != 2:
        msg = "bounds must have shape (dimension, 2)"
        raise ValueError(msg)
    if not np.all(np.isfinite(bounds_arr)):
        msg = "bounds must contain finite values"
        raise ValueError(msg)
    if np.any(bounds_arr[:, 1] <= bounds_arr[:, 0]):
        msg = "each bound must satisfy high > low"
        raise ValueError(msg)
    return bounds_arr


def _select_pairs(
    dimension: int,
    config: PairwiseProbeConfig,
    candidate_pairs: Iterable[Pair] | None,
) -> list[Pair]:
    pairs = list(candidate_pairs) if candidate_pairs is not None else list(combinations(range(dimension), 2))
    for i, j in pairs:
        if i == j or i < 0 or j < 0 or i >= dimension or j >= dimension:
            msg = f"invalid variable pair {(i, j)} for dimension {dimension}"
            raise ValueError(msg)

    normalized = [(min(i, j), max(i, j)) for i, j in pairs]
    unique_pairs = sorted(set(normalized))
    if config.max_pairs is None or len(unique_pairs) <= config.max_pairs:
        return unique_pairs

    rng = np.random.default_rng(config.random_seed)
    selected = rng.choice(len(unique_pairs), size=config.max_pairs, replace=False)
    return [unique_pairs[int(index)] for index in sorted(selected)]


def _steps_from_bounds(bounds: NDArray[np.float64], delta: float) -> NDArray[np.float64]:
    if delta <= 0:
        msg = "delta must be positive"
        raise ValueError(msg)
    span = bounds[:, 1] - bounds[:, 0]
    return np.maximum(span * delta, np.finfo(np.float64).eps)


def _sample_base_points(
    bounds: NDArray[np.float64],
    steps: NDArray[np.float64],
    probes: int,
    rng: np.random.Generator,
) -> NDArray[np.float64]:
    if probes <= 0:
        msg = "probes must be positive"
        raise ValueError(msg)
    lows = bounds[:, 0]
    highs = np.maximum(lows, bounds[:, 1] - steps)
    return rng.uniform(lows, highs, size=(probes, bounds.shape[0]))


def _second_order_delta(
    objective: Objective,
    base: NDArray[np.float64],
    steps: NDArray[np.float64],
    i: int,
    j: int,
) -> tuple[float, float]:
    xi = base.copy()
    xj = base.copy()
    xij = base.copy()
    xi[i] += steps[i]
    xj[j] += steps[j]
    xij[i] += steps[i]
    xij[j] += steps[j]

    f0 = _evaluate(objective, base)
    fi = _evaluate(objective, xi)
    fj = _evaluate(objective, xj)
    fij = _evaluate(objective, xij)
    interaction = fij - fi - fj + f0
    scale = max(abs(f0), abs(fi), abs(fj), abs(fij), np.finfo(np.float64).eps)
    return interaction, scale


def _evaluate(objective: Objective, x: NDArray[np.float64]) -> float:
    value = float(objective(x.copy()))
    if not np.isfinite(value):
        msg = "objective returned a non-finite value"
        raise ValueError(msg)
    return value


def _robust_probability_params(
    scores: NDArray[np.float64],
    config: PairwiseProbeConfig,
) -> tuple[float, float]:
    if scores.size == 0:
        return 0.0, 1.0
    median = float(np.median(scores))
    mad = float(np.median(np.abs(scores - median)))

    eps = np.finfo(np.float64).eps
    positive_scores = scores[scores > eps]
    if config.threshold is None:
        if 0 < positive_scores.size < scores.size:
            threshold = float(np.min(positive_scores) / 2.0)
        else:
            threshold = median
    else:
        threshold = float(config.threshold)

    if config.temperature is None:
        temperature = 1.4826 * mad
        if temperature <= eps and positive_scores.size:
            temperature = max(float(np.min(positive_scores)) / 6.0, eps)
    else:
        temperature = float(config.temperature)

    temperature = max(temperature, np.finfo(np.float64).eps)
    return threshold, temperature


def _sigmoid(value: float) -> float:
    if value >= 0:
        factor = float(np.exp(-value))
        return 1.0 / (1.0 + factor)
    factor = float(np.exp(value))
    return factor / (1.0 + factor)
