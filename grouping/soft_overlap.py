"""Soft overlapping community recovery for weighted VIGs."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

from grouping.weighted_vig import WeightedVIG, reliability_weighted_adjacency


@dataclass(frozen=True)
class SoftOverlapConfig:
    """Configuration for the affiliation model."""

    n_groups: int
    max_iter: int = 300
    tol: float = 1e-5
    random_seed: int = 0
    uncertainty_power: float = 1.0
    membership_threshold: float = 0.25
    epsilon: float = 1e-12


@dataclass(frozen=True)
class SoftOverlapResult:
    """Learned soft affiliations and derived shared-variable confidence."""

    memberships: NDArray[np.float64]
    reconstruction_error: float
    iterations: int
    shared_confidence: NDArray[np.float64]
    groups: tuple[tuple[int, ...], ...]


def fit_affiliation_model(vig: WeightedVIG, config: SoftOverlapConfig) -> SoftOverlapResult:
    """Fit a non-negative affiliation matrix ``Z`` such that ``W ~= Z Z^T``."""

    _validate_config(vig, config)
    adjacency = reliability_weighted_adjacency(vig, power=config.uncertainty_power)
    rng = np.random.default_rng(config.random_seed)
    z = rng.random((vig.dimension, config.n_groups), dtype=np.float64) + config.epsilon

    previous_error = np.inf
    iterations = 0
    for iterations in range(1, config.max_iter + 1):
        numerator = adjacency @ z
        denominator = (z @ z.T @ z) + config.epsilon
        z *= numerator / denominator
        z = np.maximum(z, config.epsilon)

        error = _relative_reconstruction_error(adjacency, z)
        if abs(previous_error - error) <= config.tol * max(1.0, previous_error):
            previous_error = error
            break
        previous_error = error

    memberships = _row_normalize(z, config.epsilon)
    memberships = _assign_isolated_variables(memberships, adjacency, config.epsilon)
    shared_confidence = _shared_confidence(memberships, adjacency)
    groups = _groups_from_memberships(memberships, config.membership_threshold)
    return SoftOverlapResult(
        memberships=memberships,
        reconstruction_error=float(previous_error),
        iterations=iterations,
        shared_confidence=shared_confidence,
        groups=groups,
    )


def _validate_config(vig: WeightedVIG, config: SoftOverlapConfig) -> None:
    if config.n_groups <= 0:
        msg = "n_groups must be positive"
        raise ValueError(msg)
    if config.n_groups > vig.dimension:
        msg = "n_groups cannot exceed the number of variables"
        raise ValueError(msg)
    if config.max_iter <= 0:
        msg = "max_iter must be positive"
        raise ValueError(msg)
    if not 0 <= config.membership_threshold <= 1:
        msg = "membership_threshold must be in [0, 1]"
        raise ValueError(msg)


def _relative_reconstruction_error(adjacency: NDArray[np.float64], z: NDArray[np.float64]) -> float:
    residual = adjacency - z @ z.T
    denominator = max(float(np.linalg.norm(adjacency)), np.finfo(np.float64).eps)
    return float(np.linalg.norm(residual) / denominator)


def _row_normalize(matrix: NDArray[np.float64], epsilon: float) -> NDArray[np.float64]:
    totals = np.sum(matrix, axis=1, keepdims=True)
    return matrix / np.maximum(totals, epsilon)


def _assign_isolated_variables(
    memberships: NDArray[np.float64],
    adjacency: NDArray[np.float64],
    epsilon: float,
) -> NDArray[np.float64]:
    adjusted = memberships.copy()
    isolated = np.sum(adjacency, axis=1) <= epsilon
    for variable in np.flatnonzero(isolated):
        adjusted[variable, :] = 0.0
        adjusted[variable, int(variable % memberships.shape[1])] = 1.0
    return adjusted


def _shared_confidence(
    memberships: NDArray[np.float64],
    adjacency: NDArray[np.float64],
) -> NDArray[np.float64]:
    if memberships.shape[1] == 1:
        return np.zeros(memberships.shape[0], dtype=np.float64)

    sorted_memberships = np.sort(memberships, axis=1)
    top = sorted_memberships[:, -1]
    second = sorted_memberships[:, -2]
    ambiguity = np.clip(1.0 - (top - second), 0.0, 1.0)

    degree = np.sum(adjacency, axis=1)
    max_degree = float(np.max(degree))
    evidence = degree / max_degree if max_degree > 0 else np.zeros_like(degree)
    return np.clip(ambiguity * evidence, 0.0, 1.0)


def _groups_from_memberships(
    memberships: NDArray[np.float64],
    threshold: float,
) -> tuple[tuple[int, ...], ...]:
    groups: list[tuple[int, ...]] = []
    winners = np.argmax(memberships, axis=1)
    for group_index in range(memberships.shape[1]):
        members = set(np.flatnonzero(memberships[:, group_index] >= threshold).tolist())
        members.update(np.flatnonzero(winners == group_index).tolist())
        groups.append(tuple(sorted(members)))
    return tuple(groups)
