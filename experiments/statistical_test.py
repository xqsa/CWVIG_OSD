"""Small statistical utilities for experiment summaries."""

from __future__ import annotations

from dataclasses import dataclass
from math import erf, sqrt

import numpy as np
from numpy.typing import ArrayLike


@dataclass(frozen=True)
class PairedSignResult:
    wins: int
    losses: int
    ties: int
    z_score: float
    two_sided_p: float


def paired_sign_test(baseline: ArrayLike, candidate: ArrayLike) -> PairedSignResult:
    """Approximate two-sided paired sign test for minimization scores."""

    baseline_arr = np.asarray(baseline, dtype=np.float64)
    candidate_arr = np.asarray(candidate, dtype=np.float64)
    if baseline_arr.shape != candidate_arr.shape:
        msg = "baseline and candidate must have the same shape"
        raise ValueError(msg)

    diff = baseline_arr - candidate_arr
    wins = int(np.sum(diff > 0))
    losses = int(np.sum(diff < 0))
    ties = int(np.sum(diff == 0))
    non_ties = wins + losses
    if non_ties == 0:
        return PairedSignResult(wins=wins, losses=losses, ties=ties, z_score=0.0, two_sided_p=1.0)

    z_score = (wins - non_ties / 2.0) / sqrt(non_ties / 4.0)
    cdf = 0.5 * (1.0 + erf(abs(z_score) / sqrt(2.0)))
    p_value = 2.0 * (1.0 - cdf)
    return PairedSignResult(
        wins=wins,
        losses=losses,
        ties=ties,
        z_score=float(z_score),
        two_sided_p=float(max(0.0, min(1.0, p_value))),
    )


if __name__ == "__main__":
    result = paired_sign_test([10.0, 8.0, 7.5], [9.0, 8.5, 7.0])
    print(result)
