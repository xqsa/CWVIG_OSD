"""Probabilistic grouping utilities for CWVIG-OSD."""

from grouping.bridge_validation import BridgeValidationResult, calibrate_shared_variables
from grouping.prob_dg import InteractionEstimate, PairwiseProbeConfig, estimate_interactions
from grouping.soft_overlap import SoftOverlapConfig, SoftOverlapResult, fit_affiliation_model
from grouping.weighted_vig import WeightedVIG, build_weighted_vig

__all__ = [
    "BridgeValidationResult",
    "InteractionEstimate",
    "PairwiseProbeConfig",
    "SoftOverlapConfig",
    "SoftOverlapResult",
    "WeightedVIG",
    "build_weighted_vig",
    "calibrate_shared_variables",
    "estimate_interactions",
    "fit_affiliation_model",
]
