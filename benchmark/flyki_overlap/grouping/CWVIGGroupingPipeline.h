#ifndef FLYKI_GROUPING_CWVIG_GROUPING_PIPELINE_H_
#define FLYKI_GROUPING_CWVIG_GROUPING_PIPELINE_H_

#include "grouping/SoftOverlapCalibration.h"

#include <cstddef>
#include <filesystem>
#include <set>
#include <string>

namespace flyki {
namespace grouping {

enum class PipelineMode {
    Synthetic,
    Flyki,
};

enum class PipelinePreset {
    Conservative,
    Balanced,
    Capped,
};

struct CWVIGGroupingPipelineConfig {
    PipelineMode mode = PipelineMode::Synthetic;
    std::string synthetic_function = "overlap";
    int func = 1;
    std::size_t dimension_limit = 10;
    std::size_t contexts = 3;
    unsigned seed = 11;
    double delta = 1e-4;
    std::string threshold_mode = "fixed";
    double fixed_threshold = 0.5;
    PipelinePreset preset = PipelinePreset::Balanced;
    InteractionScoreColumn edge_score_column = InteractionScoreColumn::MeanAbsNormalized;
    EdgeWeightMode edge_weight_mode = EdgeWeightMode::UncertaintyPenalized;
    SparsificationMode sparsification_mode = SparsificationMode::TargetAvgDegree;
    double graph_score_threshold = 0.0;
    double target_avg_degree = 1.5;
    std::size_t top_k_per_node = 1;
    double max_avg_degree = 2.0;
    MembershipTransform membership_transform = MembershipTransform::CurrentRatio;
    SharedVariableRule shared_rule = SharedVariableRule::HardPlusSecondMembership;
    double max_shared_ratio = 0.5;
    double shared_min_second_membership = 0.8;
    std::filesystem::path true_po_path;
    std::filesystem::path true_oo_path;
    std::filesystem::path output_dir;
};

struct CWVIGGroupingPipelineOutputs {
    std::filesystem::path edges_csv;
    std::filesystem::path edge_metrics_csv;
    std::filesystem::path z_csv;
    std::filesystem::path predicted_groups;
    std::filesystem::path predicted_overlap;
    std::filesystem::path grouping_metrics_csv;
    std::filesystem::path pipeline_config;
    std::filesystem::path pipeline_summary;
};

struct CWVIGGroupingPipelineSummary {
    std::size_t function_evaluations = 0;
    std::size_t graph_edges_before_pruning = 0;
    std::size_t graph_edges_after_pruning = 0;
    std::size_t groups = 0;
    std::size_t shared_variables = 0;
    double over_shared_ratio = 0.0;
    std::size_t singleton_count = 0;
    double mean_group_size = 0.0;
    std::size_t max_group_size = 0;
    std::size_t validation_errors = 0;
    SetMetrics shared_metrics;
    double mean_best_group_jaccard = 0.0;
    std::set<int> shared_set;
};

struct CWVIGGroupingPipelineResult {
    CWVIGGroupingPipelineOutputs outputs;
    CWVIGGroupingPipelineSummary summary;
};

PipelinePreset parsePipelinePreset(const std::string &name);
std::string pipelinePresetName(PipelinePreset preset);
CWVIGGroupingPipelineConfig pipelineConfigForPreset(PipelinePreset preset);
CWVIGGroupingPipelineConfig loadPipelineConfigFile(const std::filesystem::path &path);

CWVIGGroupingPipelineResult runCWVIGGroupingPipeline(
    const CWVIGGroupingPipelineConfig &config);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_CWVIG_GROUPING_PIPELINE_H_
