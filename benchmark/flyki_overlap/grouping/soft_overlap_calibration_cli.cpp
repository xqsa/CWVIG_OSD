#include "grouping/GroupingIO.h"
#include "grouping/SoftOverlapCalibration.h"
#include "grouping/WeightedInteractionGraph.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::map<std::string, std::string> parseArgs(const int argc, char **argv)
{
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h" || key == "--print-summary") {
            args[key] = "";
            continue;
        }
        if (key.rfind("--", 0) != 0 || i + 1 >= argc) {
            throw std::runtime_error("Invalid argument: " + key);
        }
        args[key] = argv[++i];
    }
    return args;
}

std::string requireArg(const std::map<std::string, std::string> &args, const std::string &key)
{
    const auto iter = args.find(key);
    if (iter == args.end()) {
        throw std::runtime_error("Missing required argument: " + key);
    }
    return iter->second;
}

std::string optionalArg(
    const std::map<std::string, std::string> &args,
    const std::string &key,
    const std::string &fallback)
{
    const auto iter = args.find(key);
    return iter == args.end() ? fallback : iter->second;
}

bool hasArg(const std::map<std::string, std::string> &args, const std::string &key)
{
    return args.find(key) != args.end();
}

bool parseBool(const std::string &value)
{
    if (value == "true" || value == "1") {
        return true;
    }
    if (value == "false" || value == "0") {
        return false;
    }
    throw std::runtime_error("Expected boolean true|false, got: " + value);
}

std::vector<double> parseGrid(const std::string &text)
{
    if (text == "auto") {
        return {};
    }
    std::vector<double> values;
    std::stringstream input(text);
    std::string item;
    while (std::getline(input, item, ',')) {
        if (!item.empty()) {
            values.push_back(std::stod(item));
        }
    }
    return values;
}

void fillGridDefaults(
    flyki::grouping::ThresholdGrid &grid,
    const flyki::grouping::ThresholdGrid &defaults)
{
    if (grid.score_thresholds.empty()) {
        grid.score_thresholds = defaults.score_thresholds;
    }
    if (grid.expand_thresholds.empty()) {
        grid.expand_thresholds = defaults.expand_thresholds;
    }
    if (grid.z_thresholds.empty()) {
        grid.z_thresholds = defaults.z_thresholds;
    }
    if (grid.shared_ratio_thresholds.empty()) {
        grid.shared_ratio_thresholds = defaults.shared_ratio_thresholds;
    }
}

void printUsage()
{
    std::cout << "Usage: soft_overlap_calibration_cli --edges CSV --score-column probability|mean_abs_normalized|mean_abs_raw "
                 "--use-log1p-score true|false --dimension N --mode unsupervised|oracle "
                 "[--edge-weight-mode raw|log1p|rank_normalized|uncertainty_penalized] "
                 "[--sparsification-mode score_threshold|top_k_per_node|mutual_top_k|target_avg_degree] "
                 "[--top-k-per-node K] [--graph-score-threshold VALUE] [--max-avg-degree VALUE] "
                 "[--unsupervised-method quantile_score|target_avg_degree|top_edge_fraction|elbow_score] "
                 "[--score-quantile Q] [--target-avg-degree D] [--top-edge-fraction Q] "
                 "[--membership-transform current_ratio|rank_normalized|minmax_normalized|sigmoid_centered] "
                 "[--membership-prune-threshold VALUE] "
                 "[--shared-rule hard_membership_only|hard_plus_second_membership|capped_shared|hard_plus_ratio] "
                 "[--max-shared-ratio VALUE] [--shared-min-second-membership VALUE] "
                 "[--true-po PATH --true-oo PATH] "
                 "[--score-threshold-grid values|auto] [--expand-threshold-grid values|auto] "
                 "[--z-threshold-grid values|auto] [--shared-ratio-threshold-grid values|auto] "
                 "--output-report CSV --output-best-po PATH --output-best-oo PATH --output-best-z PATH "
                 "[--print-summary]\n";
}

flyki::grouping::EdgeWeightMode edgeWeightModeFromArgs(
    const std::map<std::string, std::string> &args,
    const bool use_log1p_score)
{
    if (hasArg(args, "--edge-weight-mode")) {
        return flyki::grouping::parseEdgeWeightMode(requireArg(args, "--edge-weight-mode"));
    }
    return use_log1p_score ? flyki::grouping::EdgeWeightMode::Log1p : flyki::grouping::EdgeWeightMode::Raw;
}

flyki::grouping::SparsificationConfig sparsificationFromArgs(
    const std::map<std::string, std::string> &args)
{
    flyki::grouping::SparsificationConfig config;
    config.mode = flyki::grouping::parseSparsificationMode(
        optionalArg(args, "--sparsification-mode", "score_threshold"));
    config.score_threshold = std::stod(optionalArg(args, "--graph-score-threshold", "0"));
    config.top_k_per_node = static_cast<std::size_t>(std::stoull(optionalArg(args, "--top-k-per-node", "0")));
    config.target_avg_degree = std::stod(optionalArg(args, "--target-avg-degree", "0"));
    config.max_avg_degree = std::stod(optionalArg(args, "--max-avg-degree", "0"));
    return config;
}

flyki::grouping::SoftOverlapConfig baseConfigFromArgs(
    const std::map<std::string, std::string> &args,
    const flyki::grouping::InteractionScoreColumn score_column,
    const bool use_log1p_score,
    const std::size_t dimension)
{
    flyki::grouping::SoftOverlapConfig config;
    config.score_column = score_column;
    config.use_log1p_score = use_log1p_score;
    config.edge_weight_mode = edgeWeightModeFromArgs(args, use_log1p_score);
    config.sparsification_mode = flyki::grouping::parseSparsificationMode(
        optionalArg(args, "--sparsification-mode", "score_threshold"));
    config.dimension = dimension;
    config.z_threshold = std::stod(optionalArg(args, "--z-threshold", "0.8"));
    config.shared_ratio_threshold = std::stod(optionalArg(args, "--shared-ratio-threshold", "0.7"));
    config.top_k_per_node = static_cast<std::size_t>(std::stoull(optionalArg(args, "--top-k-per-node", "0")));
    config.target_avg_degree = std::stod(optionalArg(args, "--target-avg-degree", "0"));
    config.max_avg_degree = std::stod(optionalArg(args, "--max-avg-degree", "0"));
    config.membership_transform = flyki::grouping::parseMembershipTransform(
        optionalArg(args, "--membership-transform", "current_ratio"));
    config.suppress_weak_memberships = parseBool(optionalArg(args, "--suppress-weak-memberships", "false"));
    config.membership_prune_threshold = std::stod(optionalArg(args, "--membership-prune-threshold", "0"));
    config.shared_rule = flyki::grouping::parseSharedVariableRule(optionalArg(args, "--shared-rule", "hard_plus_ratio"));
    config.max_shared_ratio = std::stod(optionalArg(args, "--max-shared-ratio", "1"));
    config.shared_min_second_membership = std::stod(optionalArg(args, "--shared-min-second-membership", "0"));
    config.shared_min_membership_gap = std::stod(optionalArg(args, "--shared-min-membership-gap", "0"));
    return config;
}

flyki::grouping::OracleTruth truthFromArgs(
    const std::map<std::string, std::string> &args,
    const std::size_t dimension)
{
    flyki::grouping::OracleTruth truth;
    if (!hasArg(args, "--true-po") && !hasArg(args, "--true-oo")) {
        return truth;
    }
    if (!hasArg(args, "--true-po") || !hasArg(args, "--true-oo")) {
        throw std::runtime_error("Both --true-po and --true-oo are required when truth is provided.");
    }
    truth.provided = true;
    truth.true_groups = flyki::grouping::readPoFile(requireArg(args, "--true-po"));
    truth.true_overlap_groups = flyki::grouping::readOoFile(requireArg(args, "--true-oo"));
    truth.dimension = dimension;
    return truth;
}

}  // namespace

int main(int argc, char **argv)
{
    try {
        const auto args = parseArgs(argc, argv);
        if (hasArg(args, "--help") || hasArg(args, "-h")) {
            printUsage();
            return 0;
        }

        const auto score_column = flyki::grouping::parseInteractionScoreColumn(requireArg(args, "--score-column"));
        const bool use_log1p_score = parseBool(requireArg(args, "--use-log1p-score"));
        const std::size_t dimension = static_cast<std::size_t>(std::stoull(requireArg(args, "--dimension")));
        auto base_config = baseConfigFromArgs(args, score_column, use_log1p_score, dimension);
        const auto graph = flyki::grouping::buildWeightedInteractionGraphFromCsv(
            requireArg(args, "--edges"),
            dimension,
            score_column,
            base_config.edge_weight_mode,
            sparsificationFromArgs(args));
        const auto truth = truthFromArgs(args, dimension);
        const auto mode = requireArg(args, "--mode");

        flyki::grouping::SoftOverlapCalibrationRow best;
        std::vector<flyki::grouping::SoftOverlapCalibrationRow> rows;

        if (mode == "unsupervised") {
            const auto method = flyki::grouping::parseUnsupervisedMethod(
                optionalArg(args, "--unsupervised-method", "quantile_score"));
            best = flyki::grouping::selectUnsupervisedThresholds(
                graph,
                base_config,
                method,
                std::stod(optionalArg(args, "--score-quantile", "0.75")),
                std::stod(optionalArg(args, "--target-avg-degree", "1.0")),
                std::stod(optionalArg(args, "--top-edge-fraction", "0.2")));
            base_config.score_threshold = best.score_threshold;
            base_config.expand_threshold = best.expand_threshold;
            base_config.z_threshold = best.z_threshold;
            base_config.shared_ratio_threshold = best.shared_ratio_threshold;
            const auto result = flyki::grouping::decomposeSoftOverlap(graph, base_config);
            best = flyki::grouping::evaluateSoftOverlapGrouping(
                result,
                truth.true_groups,
                truth.true_overlap_groups,
                dimension);
            best.score_column = score_column;
            best.use_log1p_score = use_log1p_score;
            best.score_threshold = base_config.score_threshold;
            best.expand_threshold = base_config.expand_threshold;
            best.z_threshold = base_config.z_threshold;
            best.shared_ratio_threshold = base_config.shared_ratio_threshold;
            rows.push_back(best);
        } else if (mode == "oracle") {
            if (!truth.provided) {
                throw std::runtime_error("Oracle mode requires --true-po and --true-oo.");
            }
            flyki::grouping::ThresholdGrid grid;
            grid.score_thresholds = parseGrid(optionalArg(args, "--score-threshold-grid", "auto"));
            grid.expand_thresholds = parseGrid(optionalArg(args, "--expand-threshold-grid", "auto"));
            grid.z_thresholds = parseGrid(optionalArg(args, "--z-threshold-grid", "auto"));
            grid.shared_ratio_thresholds = parseGrid(optionalArg(args, "--shared-ratio-threshold-grid", "auto"));
            fillGridDefaults(grid, flyki::grouping::autoThresholdGrid(graph));
            const auto oracle = flyki::grouping::runOracleCalibration(graph, base_config, truth, grid);
            rows = oracle.rows;
            best = oracle.best;
        } else {
            throw std::runtime_error("Invalid mode: " + mode);
        }

        flyki::grouping::writeCalibrationReportCsv(requireArg(args, "--output-report"), rows);
        flyki::grouping::writeBestSoftOverlapOutputs(
            requireArg(args, "--output-best-po"),
            requireArg(args, "--output-best-oo"),
            requireArg(args, "--output-best-z"),
            graph,
            base_config,
            best);

        if (hasArg(args, "--print-summary")) {
            std::cout << "Mode: " << mode << '\n'
                      << "Rows: " << rows.size() << '\n'
                      << "Graph Edges: " << graph.edges().size() << '\n'
                      << "Edge Weight Mode: " << flyki::grouping::edgeWeightModeName(base_config.edge_weight_mode) << '\n'
                      << "Sparsification Mode: " << flyki::grouping::sparsificationModeName(base_config.sparsification_mode) << '\n'
                      << "Best Score Threshold: " << best.score_threshold << '\n'
                      << "Best Expand Threshold: " << best.expand_threshold << '\n'
                      << "Best Z Threshold: " << best.z_threshold << '\n'
                      << "Best Shared Ratio Threshold: " << best.shared_ratio_threshold << '\n'
                      << "Groups: " << best.groups << '\n'
                      << "Shared Variables: " << best.shared_variables << '\n'
                      << "Over Shared Ratio: " << best.over_shared_ratio << '\n'
                      << "Shared Pruned By Cap: " << best.shared_diagnostics.pruned_by_cap << '\n'
                      << "Shared Confidence Mean: " << best.shared_diagnostics.confidence_mean << '\n'
                      << "Average Top2/Top1 Ratio: " << best.shared_diagnostics.average_top2_top1_ratio << '\n'
                      << "SharedVar F1: " << best.shared_metrics.f1 << '\n'
                      << "Mean Best Group Jaccard: " << best.mean_best_group_jaccard << '\n';
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "soft_overlap_calibration_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
