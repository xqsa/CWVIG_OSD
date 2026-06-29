#include "grouping/SoftOverlapCalibration.h"

#include "grouping/GroupingIO.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

std::vector<double> sortedScores(const WeightedInteractionGraph &graph)
{
    std::vector<double> scores;
    for (const auto &edge : graph.edges()) {
        scores.push_back(edge.weight);
    }
    std::sort(scores.begin(), scores.end());
    return scores;
}

double quantile(std::vector<double> values, const double q)
{
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double clamped = std::max(0.0, std::min(1.0, q));
    const double pos = clamped * static_cast<double>(values.size() - 1);
    const auto lower = static_cast<std::size_t>(pos);
    const auto upper = std::min(lower + 1, values.size() - 1);
    const double weight = pos - static_cast<double>(lower);
    return values[lower] * (1.0 - weight) + values[upper] * weight;
}

std::vector<double> uniqueSorted(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

double thresholdForTargetAvgDegree(const WeightedInteractionGraph &graph, const double target_avg_degree)
{
    auto thresholds = sortedScores(graph);
    thresholds = uniqueSorted(std::move(thresholds));
    if (thresholds.empty()) {
        return 0.0;
    }

    double best_threshold = thresholds.front();
    double best_distance = std::numeric_limits<double>::max();
    for (const double threshold : thresholds) {
        const double avg_degree = graph.dimension() == 0
            ? 0.0
            : 2.0 * static_cast<double>(graph.strongEdges(threshold).size()) / static_cast<double>(graph.dimension());
        const double distance = std::fabs(avg_degree - target_avg_degree);
        if (distance < best_distance || (distance == best_distance && threshold > best_threshold)) {
            best_distance = distance;
            best_threshold = threshold;
        }
    }
    return best_threshold;
}

double thresholdForTopEdgeFraction(const WeightedInteractionGraph &graph, const double fraction)
{
    auto scores = sortedScores(graph);
    if (scores.empty()) {
        return 0.0;
    }
    std::sort(scores.begin(), scores.end(), std::greater<double>());
    const double clamped = std::max(0.0, std::min(1.0, fraction));
    const std::size_t keep = std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(clamped * scores.size())));
    return scores[std::min(keep - 1, scores.size() - 1)];
}

double thresholdForElbow(const WeightedInteractionGraph &graph)
{
    auto scores = sortedScores(graph);
    if (scores.size() < 3) {
        return scores.empty() ? 0.0 : scores.back();
    }
    std::sort(scores.begin(), scores.end(), std::greater<double>());
    const double x1 = 0.0;
    const double y1 = scores.front();
    const double x2 = static_cast<double>(scores.size() - 1);
    const double y2 = scores.back();
    double best_distance = -1.0;
    std::size_t best_index = 0;
    for (std::size_t i = 1; i + 1 < scores.size(); ++i) {
        const double x0 = static_cast<double>(i);
        const double y0 = scores[i];
        const double numerator = std::fabs((y2 - y1) * x0 - (x2 - x1) * y0 + x2 * y1 - y2 * x1);
        const double denominator = std::sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
        const double distance = denominator == 0.0 ? 0.0 : numerator / denominator;
        if (distance > best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    return scores[best_index];
}

std::set<int> restrictSharedVariables(
    const std::vector<std::vector<int>> &overlap_groups,
    const std::size_t dimension)
{
    std::set<int> shared;
    for (const auto &group : overlap_groups) {
        for (const int variable : group) {
            if (variable >= 0 && static_cast<std::size_t>(variable) < dimension) {
                shared.insert(variable);
            }
        }
    }
    return shared;
}

std::vector<std::set<int>> restrictGroups(
    const std::vector<std::vector<int>> &groups,
    const std::size_t dimension)
{
    std::vector<std::set<int>> result;
    for (const auto &group : groups) {
        std::set<int> restricted;
        for (const int variable : group) {
            if (variable >= 0 && static_cast<std::size_t>(variable) < dimension) {
                restricted.insert(variable);
            }
        }
        if (!restricted.empty()) {
            result.push_back(restricted);
        }
    }
    return result;
}

double jaccard(const std::set<int> &left, const std::set<int> &right)
{
    if (left.empty() && right.empty()) {
        return 1.0;
    }
    std::vector<int> intersection;
    std::set_intersection(
        left.begin(),
        left.end(),
        right.begin(),
        right.end(),
        std::back_inserter(intersection));
    const std::size_t union_size = left.size() + right.size() - intersection.size();
    return union_size == 0 ? 0.0 : static_cast<double>(intersection.size()) / static_cast<double>(union_size);
}

std::size_t countSingletons(const std::vector<std::vector<int>> &groups)
{
    return static_cast<std::size_t>(std::count_if(groups.begin(), groups.end(), [](const auto &group) {
        return group.size() == 1;
    }));
}

SoftOverlapConfig configFromRow(SoftOverlapConfig config, const SoftOverlapCalibrationRow &row)
{
    config.score_threshold = row.score_threshold;
    config.expand_threshold = row.expand_threshold;
    config.z_threshold = row.z_threshold;
    config.shared_ratio_threshold = row.shared_ratio_threshold;
    return config;
}

bool betterOracleRow(const SoftOverlapCalibrationRow &candidate, const SoftOverlapCalibrationRow &best)
{
    if (candidate.shared_metrics.f1 != best.shared_metrics.f1) {
        return candidate.shared_metrics.f1 > best.shared_metrics.f1;
    }
    if (candidate.mean_best_group_jaccard != best.mean_best_group_jaccard) {
        return candidate.mean_best_group_jaccard > best.mean_best_group_jaccard;
    }
    if (candidate.over_shared_ratio != best.over_shared_ratio) {
        return candidate.over_shared_ratio < best.over_shared_ratio;
    }
    return candidate.groups < best.groups;
}

}  // namespace

UnsupervisedMethod parseUnsupervisedMethod(const std::string &name)
{
    if (name == "quantile_score") {
        return UnsupervisedMethod::QuantileScore;
    }
    if (name == "target_avg_degree") {
        return UnsupervisedMethod::TargetAvgDegree;
    }
    if (name == "top_edge_fraction") {
        return UnsupervisedMethod::TopEdgeFraction;
    }
    if (name == "elbow_score") {
        return UnsupervisedMethod::ElbowScore;
    }
    throw std::invalid_argument("Invalid unsupervised calibration method: " + name);
}

std::string unsupervisedMethodName(const UnsupervisedMethod method)
{
    switch (method) {
    case UnsupervisedMethod::QuantileScore:
        return "quantile_score";
    case UnsupervisedMethod::TargetAvgDegree:
        return "target_avg_degree";
    case UnsupervisedMethod::TopEdgeFraction:
        return "top_edge_fraction";
    case UnsupervisedMethod::ElbowScore:
        return "elbow_score";
    }
    return "unknown";
}

ThresholdGrid autoThresholdGrid(const WeightedInteractionGraph &graph)
{
    const auto scores = sortedScores(graph);
    ThresholdGrid grid;
    if (scores.empty()) {
        grid.score_thresholds = {0.0};
        grid.expand_thresholds = {0.5};
    } else {
        grid.score_thresholds = uniqueSorted({
            quantile(scores, 0.50),
            quantile(scores, 0.75),
            quantile(scores, 0.90),
            quantile(scores, 0.95),
        });
        const double median = quantile(scores, 0.50);
        grid.expand_thresholds = uniqueSorted({median * 0.5, median, quantile(scores, 0.75)});
    }
    grid.z_thresholds = {0.5, 0.8, 0.95};
    grid.shared_ratio_thresholds = {0.7, 0.9, 1.1};
    return grid;
}

SoftOverlapCalibrationRow selectUnsupervisedThresholds(
    const WeightedInteractionGraph &graph,
    const SoftOverlapConfig &base_config,
    const UnsupervisedMethod method,
    const double score_quantile,
    const double target_avg_degree,
    const double top_edge_fraction)
{
    SoftOverlapCalibrationRow row;
    row.score_column = base_config.score_column;
    row.use_log1p_score = base_config.use_log1p_score;
    const auto scores = sortedScores(graph);
    switch (method) {
    case UnsupervisedMethod::QuantileScore:
        row.score_threshold = quantile(scores, score_quantile);
        break;
    case UnsupervisedMethod::TargetAvgDegree:
        row.score_threshold = thresholdForTargetAvgDegree(graph, target_avg_degree);
        break;
    case UnsupervisedMethod::TopEdgeFraction:
        row.score_threshold = thresholdForTopEdgeFraction(graph, top_edge_fraction);
        break;
    case UnsupervisedMethod::ElbowScore:
        row.score_threshold = thresholdForElbow(graph);
        break;
    }
    row.expand_threshold = row.score_threshold;
    row.z_threshold = base_config.z_threshold;
    row.shared_ratio_threshold = base_config.shared_ratio_threshold;
    return row;
}

SoftOverlapCalibrationRow evaluateSoftOverlapGrouping(
    const SoftOverlapResult &result,
    const std::vector<std::vector<int>> &true_groups,
    const std::vector<std::vector<int>> &true_overlap_groups,
    const std::size_t dimension)
{
    SoftOverlapCalibrationRow row;
    const auto summary = summarizeGroups(result.groups);
    row.groups = summary.number_groups;
    row.unique_variables = summary.number_unique_variables;
    row.shared_variables = result.shared_variables.size();
    row.over_shared_ratio = dimension == 0 ? 0.0 : static_cast<double>(row.shared_variables) / static_cast<double>(dimension);
    row.singleton_count = countSingletons(result.groups);
    row.mean_group_size = summary.mean_group_size;
    row.max_group_size = summary.max_group_size;
    row.shared_metrics = calculateSetMetrics(
        restrictSharedVariables(true_overlap_groups, dimension),
        result.shared_variables);

    const auto pred_sets = restrictGroups(result.groups, dimension);
    const auto true_sets = restrictGroups(true_groups, dimension);
    if (pred_sets.empty()) {
        row.mean_best_group_jaccard = true_sets.empty() ? 1.0 : 0.0;
    } else {
        double sum = 0.0;
        for (const auto &pred : pred_sets) {
            double best = 0.0;
            for (const auto &truth : true_sets) {
                best = std::max(best, jaccard(pred, truth));
            }
            sum += best;
            if (std::find(true_sets.begin(), true_sets.end(), pred) != true_sets.end()) {
                ++row.exact_group_matches;
            }
        }
        row.mean_best_group_jaccard = sum / static_cast<double>(pred_sets.size());
    }
    return row;
}

OracleCalibrationResult runOracleCalibration(
    const WeightedInteractionGraph &graph,
    const SoftOverlapConfig &base_config,
    const OracleTruth &truth,
    const ThresholdGrid &grid)
{
    if (!truth.provided) {
        throw std::invalid_argument("Oracle calibration requires true po/oo files.");
    }
    OracleCalibrationResult result;
    bool have_best = false;
    for (const double score_threshold : grid.score_thresholds) {
        for (const double expand_threshold : grid.expand_thresholds) {
            for (const double z_threshold : grid.z_thresholds) {
                for (const double shared_ratio_threshold : grid.shared_ratio_thresholds) {
                    SoftOverlapConfig config = base_config;
                    config.score_threshold = score_threshold;
                    config.expand_threshold = expand_threshold;
                    config.z_threshold = z_threshold;
                    config.shared_ratio_threshold = shared_ratio_threshold;
                    const auto decomposition = decomposeSoftOverlap(graph, config);
                    auto row = evaluateSoftOverlapGrouping(
                        decomposition,
                        truth.true_groups,
                        truth.true_overlap_groups,
                        truth.dimension == 0 ? graph.dimension() : truth.dimension);
                    row.score_column = base_config.score_column;
                    row.use_log1p_score = base_config.use_log1p_score;
                    row.score_threshold = score_threshold;
                    row.expand_threshold = expand_threshold;
                    row.z_threshold = z_threshold;
                    row.shared_ratio_threshold = shared_ratio_threshold;
                    result.rows.push_back(row);
                    if (!have_best || betterOracleRow(row, result.best)) {
                        result.best = row;
                        have_best = true;
                    }
                }
            }
        }
    }
    return result;
}

void writeCalibrationReportCsv(
    const std::string &path,
    const std::vector<SoftOverlapCalibrationRow> &rows)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write calibration report: " + path);
    }
    output << "score_column,use_log1p_score,score_threshold,expand_threshold,z_threshold,"
              "shared_ratio_threshold,groups,unique_variables,shared_variables,over_shared_ratio,"
              "singleton_count,mean_group_size,max_group_size,shared_precision,shared_recall,"
              "shared_f1,mean_best_group_jaccard,exact_group_matches\n";
    for (const auto &row : rows) {
        output << interactionScoreColumnName(row.score_column) << ','
               << (row.use_log1p_score ? "true" : "false") << ','
               << row.score_threshold << ','
               << row.expand_threshold << ','
               << row.z_threshold << ','
               << row.shared_ratio_threshold << ','
               << row.groups << ','
               << row.unique_variables << ','
               << row.shared_variables << ','
               << row.over_shared_ratio << ','
               << row.singleton_count << ','
               << row.mean_group_size << ','
               << row.max_group_size << ','
               << row.shared_metrics.precision << ','
               << row.shared_metrics.recall << ','
               << row.shared_metrics.f1 << ','
               << row.mean_best_group_jaccard << ','
               << row.exact_group_matches << '\n';
    }
}

void writeBestSoftOverlapOutputs(
    const std::string &po_path,
    const std::string &oo_path,
    const std::string &z_path,
    const WeightedInteractionGraph &graph,
    SoftOverlapConfig base_config,
    const SoftOverlapCalibrationRow &row)
{
    const auto result = decomposeSoftOverlap(graph, configFromRow(base_config, row));
    writeSoftOverlapOutputs(po_path, oo_path, z_path, result);
}

}  // namespace grouping
}  // namespace flyki
