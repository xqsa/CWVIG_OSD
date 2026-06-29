#ifndef FLYKI_PROBE_CWVIG_EDGE_METRICS_H_
#define FLYKI_PROBE_CWVIG_EDGE_METRICS_H_

#include "probe/CWVIGEstimator.h"
#include "probe/InteractionGroundTruth.h"

#include <cstddef>
#include <vector>

namespace flyki {
namespace probe {

struct EdgeLabelCounts {
    std::size_t positive = 0;
    std::size_t negative = 0;
};

struct EdgeClassificationMetrics {
    std::size_t true_positive = 0;
    std::size_t false_positive = 0;
    std::size_t false_negative = 0;
    std::size_t true_negative = 0;
    double precision = 0.0;
    double recall = 0.0;
    double f1 = 0.0;
};

struct EdgeThresholdResult {
    double threshold = 0.0;
    double f1 = 0.0;
    EdgeClassificationMetrics metrics;
};

struct EdgeScoreAverages {
    std::size_t positive_count = 0;
    std::size_t negative_count = 0;
    double positive_mean = 0.0;
    double negative_mean = 0.0;
};

struct ScoreDistributionSummary {
    std::size_t count = 0;
    double min = 0.0;
    double max = 0.0;
    double mean = 0.0;
    double median = 0.0;
    double q25 = 0.0;
    double q75 = 0.0;
    double q90 = 0.0;
    double q95 = 0.0;
};

EdgeLabelCounts countEdgeLabels(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth);

EdgeClassificationMetrics calculateProbabilityMetrics(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    double threshold);

EdgeClassificationMetrics calculateScoreMetrics(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    double threshold);

double topKPrecisionByScore(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    std::size_t k);

EdgeScoreAverages averageScoresByLabel(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth);

EdgeThresholdResult bestF1ThresholdByScore(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth);

EdgeThresholdResult bestF1ThresholdByProbability(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth);

double rankingAccuracyByScore(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth);

ScoreDistributionSummary summarizeScores(const std::vector<CWVIGEdge> &edges);

}  // namespace probe
}  // namespace flyki

#endif  // FLYKI_PROBE_CWVIG_EDGE_METRICS_H_
