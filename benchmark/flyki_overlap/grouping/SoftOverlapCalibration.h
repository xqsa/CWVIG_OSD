#ifndef FLYKI_GROUPING_SOFT_OVERLAP_CALIBRATION_H_
#define FLYKI_GROUPING_SOFT_OVERLAP_CALIBRATION_H_

#include "grouping/GroupingMetrics.h"
#include "grouping/SoftOverlapDecomposition.h"

#include <cstddef>
#include <string>
#include <vector>

namespace flyki {
namespace grouping {

enum class UnsupervisedMethod {
    QuantileScore,
    TargetAvgDegree,
    TopEdgeFraction,
    ElbowScore,
};

struct ThresholdGrid {
    std::vector<double> score_thresholds;
    std::vector<double> expand_thresholds;
    std::vector<double> z_thresholds;
    std::vector<double> shared_ratio_thresholds;
};

struct OracleTruth {
    bool provided = false;
    std::vector<std::vector<int>> true_groups;
    std::vector<std::vector<int>> true_overlap_groups;
    std::size_t dimension = 0;
};

struct SoftOverlapCalibrationRow {
    InteractionScoreColumn score_column = InteractionScoreColumn::MeanAbsNormalized;
    bool use_log1p_score = false;
    double score_threshold = 0.0;
    double expand_threshold = 0.0;
    double z_threshold = 0.0;
    double shared_ratio_threshold = 0.0;
    std::size_t groups = 0;
    std::size_t unique_variables = 0;
    std::size_t shared_variables = 0;
    double over_shared_ratio = 0.0;
    std::size_t singleton_count = 0;
    double mean_group_size = 0.0;
    std::size_t max_group_size = 0;
    SetMetrics shared_metrics;
    double mean_best_group_jaccard = 0.0;
    std::size_t exact_group_matches = 0;
    SharedVariableDiagnostics shared_diagnostics;
};

struct OracleCalibrationResult {
    std::vector<SoftOverlapCalibrationRow> rows;
    SoftOverlapCalibrationRow best;
};

UnsupervisedMethod parseUnsupervisedMethod(const std::string &name);
std::string unsupervisedMethodName(UnsupervisedMethod method);

ThresholdGrid autoThresholdGrid(const WeightedInteractionGraph &graph);

SoftOverlapCalibrationRow selectUnsupervisedThresholds(
    const WeightedInteractionGraph &graph,
    const SoftOverlapConfig &base_config,
    UnsupervisedMethod method,
    double score_quantile,
    double target_avg_degree,
    double top_edge_fraction);

SoftOverlapCalibrationRow evaluateSoftOverlapGrouping(
    const SoftOverlapResult &result,
    const std::vector<std::vector<int>> &true_groups,
    const std::vector<std::vector<int>> &true_overlap_groups,
    std::size_t dimension);

OracleCalibrationResult runOracleCalibration(
    const WeightedInteractionGraph &graph,
    const SoftOverlapConfig &base_config,
    const OracleTruth &truth,
    const ThresholdGrid &grid);

void writeCalibrationReportCsv(
    const std::string &path,
    const std::vector<SoftOverlapCalibrationRow> &rows);

void writeBestSoftOverlapOutputs(
    const std::string &po_path,
    const std::string &oo_path,
    const std::string &z_path,
    const WeightedInteractionGraph &graph,
    SoftOverlapConfig base_config,
    const SoftOverlapCalibrationRow &row);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_SOFT_OVERLAP_CALIBRATION_H_
