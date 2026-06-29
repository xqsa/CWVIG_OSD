#include "grouping/CWVIGGroupingPipeline.h"

#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingIO.h"
#include "probe/BenchmarkEvaluator.h"
#include "probe/CWVIGEdgeMetrics.h"
#include "probe/InteractionGroundTruth.h"
#include "probe/SyntheticFunctions.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <regex>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

constexpr double kDefaultZThreshold = 0.8;
constexpr double kDefaultSharedRatioThreshold = 0.7;
constexpr double kDefaultMinExpandThreshold = 0.8;

CWVIGGroupingPipelineOutputs outputPaths(const std::filesystem::path &output_dir, const bool has_truth)
{
    CWVIGGroupingPipelineOutputs outputs;
    outputs.edges_csv = output_dir / "edges.csv";
    if (has_truth) {
        outputs.edge_metrics_csv = output_dir / "edge_metrics.csv";
        outputs.grouping_metrics_csv = output_dir / "grouping_metrics.csv";
    }
    outputs.z_csv = output_dir / "z.csv";
    outputs.predicted_groups = output_dir / "predicted_groups.txt";
    outputs.predicted_overlap = output_dir / "predicted_overlap.txt";
    outputs.pipeline_config = output_dir / "pipeline_config.txt";
    outputs.pipeline_summary = output_dir / "pipeline_summary.txt";
    return outputs;
}

bool hasTruth(const CWVIGGroupingPipelineConfig &config)
{
    if (config.true_po_path.empty() && config.true_oo_path.empty()) {
        return false;
    }
    if (config.true_po_path.empty() || config.true_oo_path.empty()) {
        throw std::invalid_argument("Both true_po_path and true_oo_path are required for pipeline evaluation.");
    }
    return true;
}

std::function<double(const std::vector<double> &)> syntheticFunction(const std::string &name)
{
    if (name == "sphere") {
        return flyki::probe::separableSphere;
    }
    if (name == "interaction") {
        return flyki::probe::twoVariableInteraction;
    }
    if (name == "overlap") {
        return flyki::probe::tinyOverlappingFunction;
    }
    if (name == "clique") {
        return flyki::probe::tinyCliqueFunction;
    }
    throw std::invalid_argument("Invalid synthetic function: " + name);
}

PipelineMode parsePipelineMode(const std::string &name)
{
    if (name == "synthetic") {
        return PipelineMode::Synthetic;
    }
    if (name == "flyki") {
        return PipelineMode::Flyki;
    }
    throw std::invalid_argument("Invalid pipeline mode: " + name);
}

std::map<std::string, std::string> readFlatJsonConfig(const std::filesystem::path &path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Failed to read pipeline config: " + path.string());
    }
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const std::regex entry(R"json("([A-Za-z0-9_]+)"\s*:\s*("([^"]*)"|[-+0-9.eE]+|true|false))json");
    std::map<std::string, std::string> values;
    std::sregex_iterator iter(text.begin(), text.end(), entry);
    const std::sregex_iterator end;
    for (; iter != end; ++iter) {
        values[(*iter)[1].str()] = (*iter)[3].matched ? (*iter)[3].str() : (*iter)[2].str();
    }
    return values;
}

bool hasValue(const std::map<std::string, std::string> &values, const std::string &key)
{
    return values.find(key) != values.end();
}

std::string valueOr(
    const std::map<std::string, std::string> &values,
    const std::string &key,
    const std::string &fallback)
{
    const auto iter = values.find(key);
    return iter == values.end() ? fallback : iter->second;
}

flyki::probe::CWVIGEstimateResult estimateEdges(const CWVIGGroupingPipelineConfig &config)
{
    flyki::probe::CWVIGEstimatorConfig estimator_config;
    estimator_config.dimension_limit = config.dimension_limit;
    estimator_config.contexts = config.contexts;
    estimator_config.seed = config.seed;
    estimator_config.delta = config.delta;
    estimator_config.threshold_mode = config.threshold_mode;
    estimator_config.fixed_threshold = config.fixed_threshold;

    if (config.mode == PipelineMode::Synthetic) {
        return flyki::probe::estimateCWVIG(
            syntheticFunction(config.synthetic_function),
            config.dimension_limit,
            estimator_config);
    }
    flyki::probe::BenchmarkEvaluator evaluator(config.func);
    return flyki::probe::estimateCWVIG([&evaluator](const std::vector<double> &candidate) {
        return evaluator.evaluate(candidate);
    }, evaluator.dimension(), estimator_config);
}

std::vector<flyki::probe::CWVIGEdge> graphEvidenceEdges(
    const std::vector<flyki::probe::CWVIGEdge> &edges)
{
    const bool has_positive_probability = std::any_of(edges.begin(), edges.end(), [](const auto &edge) {
        return edge.probability > 0.5;
    });
    return has_positive_probability ? edges : std::vector<flyki::probe::CWVIGEdge>{};
}

SparsificationConfig sparsificationConfig(const CWVIGGroupingPipelineConfig &config)
{
    SparsificationConfig sparsification;
    sparsification.mode = config.sparsification_mode;
    sparsification.score_threshold = config.graph_score_threshold;
    sparsification.top_k_per_node = config.top_k_per_node;
    sparsification.target_avg_degree = config.target_avg_degree;
    sparsification.max_avg_degree = config.max_avg_degree;
    return sparsification;
}

SoftOverlapConfig softConfig(const CWVIGGroupingPipelineConfig &config)
{
    SoftOverlapConfig soft;
    soft.score_column = config.edge_score_column;
    soft.edge_weight_mode = config.edge_weight_mode;
    soft.sparsification_mode = config.sparsification_mode;
    soft.score_threshold = 0.0;
    soft.expand_threshold = 0.0;
    soft.z_threshold = kDefaultZThreshold;
    soft.shared_ratio_threshold = kDefaultSharedRatioThreshold;
    soft.top_k_per_node = config.top_k_per_node;
    soft.target_avg_degree = config.target_avg_degree;
    soft.max_avg_degree = config.max_avg_degree;
    soft.membership_transform = config.membership_transform;
    soft.shared_rule = config.shared_rule;
    soft.max_shared_ratio = config.max_shared_ratio;
    soft.shared_min_second_membership = config.shared_min_second_membership;
    soft.dimension = config.dimension_limit;
    return soft;
}

SoftOverlapCalibrationRow calibratedRow(
    const WeightedInteractionGraph &graph,
    const CWVIGGroupingPipelineConfig &config,
    const SoftOverlapConfig &base_config)
{
    const auto method = config.sparsification_mode == SparsificationMode::TargetAvgDegree
        ? UnsupervisedMethod::TargetAvgDegree
        : UnsupervisedMethod::QuantileScore;
    return selectUnsupervisedThresholds(
        graph,
        base_config,
        method,
        config.shared_rule == SharedVariableRule::CappedShared ? 0.90 : 0.75,
        config.target_avg_degree,
        0.2);
}

void writeEdgeMetricsCsv(
    const std::filesystem::path &path,
    const std::vector<flyki::probe::CWVIGEdge> &edges,
    const std::vector<std::vector<int>> &true_groups,
    const std::size_t dimension_limit)
{
    const auto truth = flyki::probe::trueInteractionEdgesFromGroups(true_groups, dimension_limit);
    const auto counts = flyki::probe::countEdgeLabels(edges, truth);
    const auto prob_metrics = flyki::probe::calculateProbabilityMetrics(edges, truth, 0.5);
    const auto score_metrics = flyki::probe::calculateScoreMetrics(edges, truth, 0.5);
    const auto best_prob = flyki::probe::bestF1ThresholdByProbability(edges, truth);
    const auto best_score = flyki::probe::bestF1ThresholdByScore(edges, truth);
    const auto top_k = counts.positive == 0 ? 1 : counts.positive;
    const auto averages = flyki::probe::averageScoresByLabel(edges, truth);

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write edge metrics: " + path.string());
    }
    output << std::setprecision(12);
    output << "metric,value\n"
           << "evaluated_edges," << edges.size() << '\n'
           << "positive_edges," << counts.positive << '\n'
           << "negative_edges," << counts.negative << '\n'
           << "prob_f1," << prob_metrics.f1 << '\n'
           << "score_f1," << score_metrics.f1 << '\n'
           << "best_probability_f1," << best_prob.f1 << '\n'
           << "best_score_f1," << best_score.f1 << '\n'
           << "top_k_precision," << flyki::probe::topKPrecisionByScore(edges, truth, top_k) << '\n'
           << "ranking_accuracy," << flyki::probe::rankingAccuracyByScore(edges, truth) << '\n'
           << "positive_mean_score," << averages.positive_mean << '\n'
           << "negative_mean_score," << averages.negative_mean << '\n';
}

void writeConfig(
    const std::filesystem::path &path,
    const CWVIGGroupingPipelineConfig &config)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write pipeline config: " + path.string());
    }
    output << "mode=" << (config.mode == PipelineMode::Synthetic ? "synthetic" : "flyki") << '\n'
           << "synthetic_function=" << config.synthetic_function << '\n'
           << "func=" << config.func << '\n'
           << "dimension_limit=" << config.dimension_limit << '\n'
           << "contexts=" << config.contexts << '\n'
           << "seed=" << config.seed << '\n'
           << "delta=" << config.delta << '\n'
           << "threshold_mode=" << config.threshold_mode << '\n'
           << "fixed_threshold=" << config.fixed_threshold << '\n'
           << "preset=" << pipelinePresetName(config.preset) << '\n'
           << "edge_score_column=" << interactionScoreColumnName(config.edge_score_column) << '\n'
           << "edge_weight_mode=" << edgeWeightModeName(config.edge_weight_mode) << '\n'
           << "sparsification_mode=" << sparsificationModeName(config.sparsification_mode) << '\n'
           << "target_avg_degree=" << config.target_avg_degree << '\n'
           << "top_k_per_node=" << config.top_k_per_node << '\n'
           << "max_avg_degree=" << config.max_avg_degree << '\n'
           << "membership_transform=" << membershipTransformName(config.membership_transform) << '\n'
           << "shared_rule=" << sharedVariableRuleName(config.shared_rule) << '\n'
           << "max_shared_ratio=" << config.max_shared_ratio << '\n'
           << "shared_min_second_membership=" << config.shared_min_second_membership << '\n';
}

void writeSummary(
    const std::filesystem::path &path,
    const CWVIGGroupingPipelineConfig &config,
    const CWVIGGroupingPipelineSummary &summary)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write pipeline summary: " + path.string());
    }
    output << std::setprecision(12);
    output << "case=";
    if (config.mode == PipelineMode::Synthetic) {
        output << "synthetic:" << config.synthetic_function << '\n';
    } else {
        output << "flyki:" << config.func << '\n';
    }
    output << "dimension_limit=" << config.dimension_limit << '\n'
           << "contexts=" << config.contexts << '\n'
           << "delta=" << config.delta << '\n'
           << "function_evaluations=" << summary.function_evaluations << '\n'
           << "preset=" << pipelinePresetName(config.preset) << '\n'
           << "edge_weight_mode=" << edgeWeightModeName(config.edge_weight_mode) << '\n'
           << "sparsification_mode=" << sparsificationModeName(config.sparsification_mode) << '\n'
           << "graph_edges_before_pruning=" << summary.graph_edges_before_pruning << '\n'
           << "graph_edges_after_pruning=" << summary.graph_edges_after_pruning << '\n'
           << "groups=" << summary.groups << '\n'
           << "shared_variables=" << summary.shared_variables << '\n'
           << "over_shared_ratio=" << summary.over_shared_ratio << '\n'
           << "singleton_count=" << summary.singleton_count << '\n'
           << "mean_group_size=" << summary.mean_group_size << '\n'
           << "max_group_size=" << summary.max_group_size << '\n'
           << "validation_errors=" << summary.validation_errors << '\n'
           << "shared_precision=" << summary.shared_metrics.precision << '\n'
           << "shared_recall=" << summary.shared_metrics.recall << '\n'
           << "shared_f1=" << summary.shared_metrics.f1 << '\n'
           << "mean_best_group_jaccard=" << summary.mean_best_group_jaccard << '\n';
}

}  // namespace

PipelinePreset parsePipelinePreset(const std::string &name)
{
    if (name == "conservative") {
        return PipelinePreset::Conservative;
    }
    if (name == "balanced") {
        return PipelinePreset::Balanced;
    }
    if (name == "capped") {
        return PipelinePreset::Capped;
    }
    throw std::invalid_argument("Invalid pipeline preset: " + name);
}

std::string pipelinePresetName(const PipelinePreset preset)
{
    switch (preset) {
    case PipelinePreset::Conservative:
        return "conservative";
    case PipelinePreset::Balanced:
        return "balanced";
    case PipelinePreset::Capped:
        return "capped";
    }
    return "unknown";
}

CWVIGGroupingPipelineConfig pipelineConfigForPreset(const PipelinePreset preset)
{
    CWVIGGroupingPipelineConfig config;
    config.preset = preset;
    if (preset == PipelinePreset::Conservative) {
        config.edge_weight_mode = EdgeWeightMode::RankNormalized;
        config.sparsification_mode = SparsificationMode::MutualTopK;
        config.top_k_per_node = 1;
        config.max_avg_degree = 2.0;
        config.membership_transform = MembershipTransform::CurrentRatio;
        config.shared_rule = SharedVariableRule::HardMembershipOnly;
    } else if (preset == PipelinePreset::Capped) {
        config.edge_weight_mode = EdgeWeightMode::Log1p;
        config.sparsification_mode = SparsificationMode::ScoreThreshold;
        config.graph_score_threshold = 0.0;
        config.membership_transform = MembershipTransform::SigmoidCentered;
        config.shared_rule = SharedVariableRule::CappedShared;
        config.max_shared_ratio = 0.5;
        config.shared_min_second_membership = 0.8;
    }
    return config;
}

CWVIGGroupingPipelineConfig loadPipelineConfigFile(const std::filesystem::path &path)
{
    const auto values = readFlatJsonConfig(path);
    auto config = pipelineConfigForPreset(parsePipelinePreset(valueOr(values, "preset", "balanced")));
    if (hasValue(values, "mode")) {
        config.mode = parsePipelineMode(values.at("mode"));
    }
    config.synthetic_function = valueOr(values, "synthetic_function", config.synthetic_function);
    config.func = std::stoi(valueOr(values, "func", std::to_string(config.func)));
    config.dimension_limit = static_cast<std::size_t>(std::stoull(valueOr(
        values,
        "dimension_limit",
        std::to_string(config.dimension_limit))));
    config.contexts = static_cast<std::size_t>(std::stoull(valueOr(values, "contexts", std::to_string(config.contexts))));
    config.seed = static_cast<unsigned>(std::stoul(valueOr(values, "seed", std::to_string(config.seed))));
    config.delta = std::stod(valueOr(values, "delta", std::to_string(config.delta)));
    config.threshold_mode = valueOr(values, "threshold_mode", config.threshold_mode);
    config.fixed_threshold = std::stod(valueOr(values, "fixed_threshold", std::to_string(config.fixed_threshold)));
    config.edge_score_column = parseInteractionScoreColumn(valueOr(
        values,
        "edge_score_column",
        interactionScoreColumnName(config.edge_score_column)));
    config.edge_weight_mode = parseEdgeWeightMode(valueOr(
        values,
        "edge_weight_mode",
        edgeWeightModeName(config.edge_weight_mode)));
    config.sparsification_mode = parseSparsificationMode(valueOr(
        values,
        "sparsification_mode",
        sparsificationModeName(config.sparsification_mode)));
    config.graph_score_threshold = std::stod(valueOr(
        values,
        "graph_score_threshold",
        std::to_string(config.graph_score_threshold)));
    config.target_avg_degree = std::stod(valueOr(values, "target_avg_degree", std::to_string(config.target_avg_degree)));
    config.top_k_per_node = static_cast<std::size_t>(std::stoull(valueOr(
        values,
        "top_k_per_node",
        std::to_string(config.top_k_per_node))));
    config.max_avg_degree = std::stod(valueOr(values, "max_avg_degree", std::to_string(config.max_avg_degree)));
    config.membership_transform = parseMembershipTransform(valueOr(
        values,
        "membership_transform",
        membershipTransformName(config.membership_transform)));
    config.shared_rule = parseSharedVariableRule(valueOr(
        values,
        "shared_rule",
        sharedVariableRuleName(config.shared_rule)));
    config.max_shared_ratio = std::stod(valueOr(values, "max_shared_ratio", std::to_string(config.max_shared_ratio)));
    config.shared_min_second_membership = std::stod(valueOr(
        values,
        "shared_min_second_membership",
        std::to_string(config.shared_min_second_membership)));
    return config;
}

CWVIGGroupingPipelineResult runCWVIGGroupingPipeline(
    const CWVIGGroupingPipelineConfig &config)
{
    if (config.output_dir.empty()) {
        throw std::invalid_argument("output_dir is required.");
    }
    const bool truth_provided = hasTruth(config);
    std::filesystem::create_directories(config.output_dir);
    const auto outputs = outputPaths(config.output_dir, truth_provided);

    const auto estimate = estimateEdges(config);
    flyki::probe::writeCWVIGEdgesCsv(outputs.edges_csv.string(), estimate);

    const auto evidence_edges = graphEvidenceEdges(estimate.edges);
    const auto graph = buildWeightedInteractionGraph(
        evidence_edges,
        config.dimension_limit,
        config.edge_score_column,
        config.edge_weight_mode,
        sparsificationConfig(config));

    auto base_config = softConfig(config);
    const auto selected = calibratedRow(graph, config, base_config);
    base_config.score_threshold = selected.score_threshold;
    base_config.expand_threshold = std::max(selected.expand_threshold, kDefaultMinExpandThreshold);
    base_config.z_threshold = selected.z_threshold;
    base_config.shared_ratio_threshold = selected.shared_ratio_threshold;

    const auto decomposition = decomposeSoftOverlap(graph, base_config);
    writeSoftOverlapOutputs(
        outputs.predicted_groups.string(),
        outputs.predicted_overlap.string(),
        outputs.z_csv.string(),
        decomposition);

    std::vector<std::vector<int>> true_groups;
    std::vector<std::vector<int>> true_overlap_groups;
    if (truth_provided) {
        true_groups = readPoFile(config.true_po_path.string());
        true_overlap_groups = readOoFile(config.true_oo_path.string());
        writeEdgeMetricsCsv(outputs.edge_metrics_csv, estimate.edges, true_groups, config.dimension_limit);
    }

    auto row = evaluateSoftOverlapGrouping(
        decomposition,
        true_groups,
        true_overlap_groups,
        config.dimension_limit);
    row.score_column = config.edge_score_column;
    row.use_log1p_score = config.edge_weight_mode == EdgeWeightMode::Log1p;
    row.score_threshold = base_config.score_threshold;
    row.expand_threshold = base_config.expand_threshold;
    row.z_threshold = base_config.z_threshold;
    row.shared_ratio_threshold = base_config.shared_ratio_threshold;
    if (truth_provided) {
        writeCalibrationReportCsv(outputs.grouping_metrics_csv.string(), {row});
    }

    const auto loaded = loadLegacyGroupingFromFiles(outputs.predicted_groups, outputs.predicted_overlap);
    const auto validation_errors = validateLegacyGroupingView(loaded);

    CWVIGGroupingPipelineResult result;
    result.outputs = outputs;
    result.summary.function_evaluations = estimate.function_evaluations;
    result.summary.graph_edges_before_pruning = evidence_edges.size();
    result.summary.graph_edges_after_pruning = graph.edges().size();
    result.summary.groups = row.groups;
    result.summary.shared_variables = row.shared_variables;
    result.summary.over_shared_ratio = row.over_shared_ratio;
    result.summary.singleton_count = row.singleton_count;
    result.summary.mean_group_size = row.mean_group_size;
    result.summary.max_group_size = row.max_group_size;
    result.summary.validation_errors = validation_errors.size();
    result.summary.shared_metrics = row.shared_metrics;
    result.summary.mean_best_group_jaccard = row.mean_best_group_jaccard;
    result.summary.shared_set = decomposition.shared_variables;

    writeConfig(outputs.pipeline_config, config);
    writeSummary(outputs.pipeline_summary, config, result.summary);
    return result;
}

}  // namespace grouping
}  // namespace flyki
