#include "probe/CWVIGEdgeMetrics.h"

#include <algorithm>
#include <numeric>

namespace flyki {
namespace probe {
namespace {

using ScoreFn = double (*)(const CWVIGEdge &);

bool isPositive(const CWVIGEdge &edge, const InteractionEdgeSet &truth)
{
    return truth.count(canonicalEdge(edge.i, edge.j)) != 0;
}

double probabilityScore(const CWVIGEdge &edge)
{
    return edge.probability;
}

double normalizedScore(const CWVIGEdge &edge)
{
    return edge.mean_abs_normalized;
}

EdgeClassificationMetrics calculateMetrics(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    const double threshold,
    const ScoreFn score)
{
    EdgeClassificationMetrics metrics;
    for (const auto &edge : edges) {
        const bool actual = isPositive(edge, truth);
        const bool predicted = score(edge) >= threshold;
        if (actual && predicted) {
            ++metrics.true_positive;
        } else if (!actual && predicted) {
            ++metrics.false_positive;
        } else if (actual) {
            ++metrics.false_negative;
        } else {
            ++metrics.true_negative;
        }
    }

    const double predicted_positive = static_cast<double>(metrics.true_positive + metrics.false_positive);
    const double actual_positive = static_cast<double>(metrics.true_positive + metrics.false_negative);
    metrics.precision = predicted_positive == 0.0 ? 0.0 : static_cast<double>(metrics.true_positive) / predicted_positive;
    metrics.recall = actual_positive == 0.0 ? 0.0 : static_cast<double>(metrics.true_positive) / actual_positive;
    metrics.f1 = (metrics.precision + metrics.recall) == 0.0
        ? 0.0
        : 2.0 * metrics.precision * metrics.recall / (metrics.precision + metrics.recall);
    return metrics;
}

EdgeThresholdResult bestF1Threshold(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    const ScoreFn score)
{
    EdgeThresholdResult best;
    if (edges.empty()) {
        return best;
    }

    std::vector<double> thresholds;
    thresholds.reserve(edges.size());
    for (const auto &edge : edges) {
        thresholds.push_back(score(edge));
    }
    std::sort(thresholds.begin(), thresholds.end());
    thresholds.erase(std::unique(thresholds.begin(), thresholds.end()), thresholds.end());

    bool have_best = false;
    for (const double threshold : thresholds) {
        const auto metrics = calculateMetrics(edges, truth, threshold, score);
        if (!have_best || metrics.f1 > best.f1 || (metrics.f1 == best.f1 && threshold > best.threshold)) {
            best.threshold = threshold;
            best.f1 = metrics.f1;
            best.metrics = metrics;
            have_best = true;
        }
    }
    return best;
}

double quantile(const std::vector<double> &sorted_values, const double q)
{
    if (sorted_values.empty()) {
        return 0.0;
    }
    const double pos = q * static_cast<double>(sorted_values.size() - 1);
    const auto lower = static_cast<std::size_t>(pos);
    const auto upper = std::min(lower + 1, sorted_values.size() - 1);
    const double weight = pos - static_cast<double>(lower);
    return sorted_values[lower] * (1.0 - weight) + sorted_values[upper] * weight;
}

}  // namespace

EdgeLabelCounts countEdgeLabels(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth)
{
    EdgeLabelCounts counts;
    for (const auto &edge : edges) {
        if (isPositive(edge, truth)) {
            ++counts.positive;
        } else {
            ++counts.negative;
        }
    }
    return counts;
}

EdgeClassificationMetrics calculateProbabilityMetrics(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    const double threshold)
{
    return calculateMetrics(edges, truth, threshold, probabilityScore);
}

EdgeClassificationMetrics calculateScoreMetrics(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    const double threshold)
{
    return calculateMetrics(edges, truth, threshold, normalizedScore);
}

double topKPrecisionByScore(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth,
    const std::size_t k)
{
    if (k == 0 || edges.empty()) {
        return 0.0;
    }

    std::vector<CWVIGEdge> ranked = edges;
    std::sort(ranked.begin(), ranked.end(), [](const auto &left, const auto &right) {
        if (left.mean_abs_normalized != right.mean_abs_normalized) {
            return left.mean_abs_normalized > right.mean_abs_normalized;
        }
        return canonicalEdge(left.i, left.j) < canonicalEdge(right.i, right.j);
    });

    const std::size_t take = std::min(k, ranked.size());
    std::size_t positives = 0;
    for (std::size_t idx = 0; idx < take; ++idx) {
        if (isPositive(ranked[idx], truth)) {
            ++positives;
        }
    }
    return static_cast<double>(positives) / static_cast<double>(take);
}

EdgeScoreAverages averageScoresByLabel(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth)
{
    EdgeScoreAverages averages;
    double positive_sum = 0.0;
    double negative_sum = 0.0;
    for (const auto &edge : edges) {
        if (isPositive(edge, truth)) {
            positive_sum += edge.mean_abs_normalized;
            ++averages.positive_count;
        } else {
            negative_sum += edge.mean_abs_normalized;
            ++averages.negative_count;
        }
    }
    averages.positive_mean = averages.positive_count == 0 ? 0.0 : positive_sum / static_cast<double>(averages.positive_count);
    averages.negative_mean = averages.negative_count == 0 ? 0.0 : negative_sum / static_cast<double>(averages.negative_count);
    return averages;
}

EdgeThresholdResult bestF1ThresholdByScore(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth)
{
    return bestF1Threshold(edges, truth, normalizedScore);
}

EdgeThresholdResult bestF1ThresholdByProbability(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth)
{
    return bestF1Threshold(edges, truth, probabilityScore);
}

double rankingAccuracyByScore(
    const std::vector<CWVIGEdge> &edges,
    const InteractionEdgeSet &truth)
{
    std::vector<double> positives;
    std::vector<double> negatives;
    for (const auto &edge : edges) {
        if (isPositive(edge, truth)) {
            positives.push_back(edge.mean_abs_normalized);
        } else {
            negatives.push_back(edge.mean_abs_normalized);
        }
    }
    if (positives.empty() || negatives.empty()) {
        return 0.0;
    }

    double wins = 0.0;
    for (const double positive : positives) {
        for (const double negative : negatives) {
            if (positive > negative) {
                wins += 1.0;
            } else if (positive == negative) {
                wins += 0.5;
            }
        }
    }
    return wins / static_cast<double>(positives.size() * negatives.size());
}

ScoreDistributionSummary summarizeScores(const std::vector<CWVIGEdge> &edges)
{
    ScoreDistributionSummary summary;
    if (edges.empty()) {
        return summary;
    }

    std::vector<double> scores;
    scores.reserve(edges.size());
    for (const auto &edge : edges) {
        scores.push_back(edge.mean_abs_normalized);
    }
    std::sort(scores.begin(), scores.end());

    summary.count = scores.size();
    summary.min = scores.front();
    summary.max = scores.back();
    summary.mean = std::accumulate(scores.begin(), scores.end(), 0.0) / static_cast<double>(scores.size());
    summary.median = quantile(scores, 0.5);
    summary.q25 = quantile(scores, 0.25);
    summary.q75 = quantile(scores, 0.75);
    summary.q90 = quantile(scores, 0.90);
    summary.q95 = quantile(scores, 0.95);
    return summary;
}

}  // namespace probe
}  // namespace flyki
