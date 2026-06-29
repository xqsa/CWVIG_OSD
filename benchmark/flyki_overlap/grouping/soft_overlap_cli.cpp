#include "grouping/SoftOverlapDecomposition.h"
#include "grouping/WeightedInteractionGraph.h"

#include <iostream>
#include <map>
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

void printUsage()
{
    std::cout << "Usage: soft_overlap_cli --edges CWVIG_EDGES.csv "
                 "--score-column probability|mean_abs_normalized|mean_abs_raw "
                 "--score-threshold VALUE --use-log1p-score true|false "
                 "--expand-threshold VALUE --merge-jaccard-threshold VALUE "
                 "--z-threshold VALUE --shared-ratio-threshold VALUE --dimension N "
                 "--output-z Z.csv --output-po predicted_groups.txt --output-oo predicted_overlap.txt "
                 "[--print-summary]\n";
}

flyki::grouping::EdgeWeightMode edgeWeightModeFromArgs(
    const std::map<std::string, std::string> &args,
    const bool use_log1p_score)
{
    const auto iter = args.find("--edge-weight-mode");
    if (iter != args.end()) {
        return flyki::grouping::parseEdgeWeightMode(iter->second);
    }
    return use_log1p_score ? flyki::grouping::EdgeWeightMode::Log1p : flyki::grouping::EdgeWeightMode::Raw;
}

flyki::grouping::SparsificationConfig sparsificationFromArgs(
    const std::map<std::string, std::string> &args,
    const double score_threshold)
{
    flyki::grouping::SparsificationConfig config;
    config.mode = flyki::grouping::parseSparsificationMode(optionalArg(args, "--sparsification-mode", "score_threshold"));
    config.score_threshold = score_threshold;
    config.top_k_per_node = static_cast<std::size_t>(std::stoull(optionalArg(args, "--top-k-per-node", "0")));
    config.target_avg_degree = std::stod(optionalArg(args, "--target-avg-degree", "0"));
    config.max_avg_degree = std::stod(optionalArg(args, "--max-avg-degree", "0"));
    return config;
}

flyki::grouping::SoftOverlapConfig configFromArgs(const std::map<std::string, std::string> &args)
{
    flyki::grouping::SoftOverlapConfig config;
    config.score_column = flyki::grouping::parseInteractionScoreColumn(requireArg(args, "--score-column"));
    config.score_threshold = std::stod(requireArg(args, "--score-threshold"));
    config.use_log1p_score = parseBool(requireArg(args, "--use-log1p-score"));
    config.edge_weight_mode = edgeWeightModeFromArgs(args, config.use_log1p_score);
    config.sparsification_mode = flyki::grouping::parseSparsificationMode(optionalArg(args, "--sparsification-mode", "score_threshold"));
    config.expand_threshold = std::stod(requireArg(args, "--expand-threshold"));
    config.merge_jaccard_threshold = std::stod(requireArg(args, "--merge-jaccard-threshold"));
    config.z_threshold = std::stod(requireArg(args, "--z-threshold"));
    config.shared_ratio_threshold = std::stod(requireArg(args, "--shared-ratio-threshold"));
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
    config.dimension = static_cast<std::size_t>(std::stoull(requireArg(args, "--dimension")));
    config.min_group_size = static_cast<std::size_t>(std::stoull(optionalArg(args, "--min-group-size", "1")));
    config.add_singletons_for_uncovered = parseBool(optionalArg(args, "--add-singletons-for-uncovered", "true"));
    return config;
}

}  // namespace

int main(int argc, char **argv)
{
    try {
        const auto args = parseArgs(argc, argv);
        if (args.find("--help") != args.end() || args.find("-h") != args.end()) {
            printUsage();
            return 0;
        }

        const auto config = configFromArgs(args);
        const auto graph = flyki::grouping::buildWeightedInteractionGraphFromCsv(
            requireArg(args, "--edges"),
            config.dimension,
            config.score_column,
            config.edge_weight_mode,
            sparsificationFromArgs(args, config.score_threshold));
        const auto result = flyki::grouping::decomposeSoftOverlap(graph, config);

        flyki::grouping::writeSoftOverlapOutputs(
            requireArg(args, "--output-po"),
            requireArg(args, "--output-oo"),
            requireArg(args, "--output-z"),
            result);

        if (args.find("--print-summary") != args.end()) {
            std::cout << "Dimension: " << graph.dimension() << '\n'
                      << "Graph Edges: " << graph.edges().size() << '\n'
                      << "Score Column: " << flyki::grouping::interactionScoreColumnName(config.score_column) << '\n'
                      << "Edge Weight Mode: " << flyki::grouping::edgeWeightModeName(config.edge_weight_mode) << '\n'
                      << "Sparsification Mode: " << flyki::grouping::sparsificationModeName(config.sparsification_mode) << '\n'
                      << "Score Threshold: " << config.score_threshold << '\n'
                      << "Groups: " << result.groups.size() << '\n'
                      << "Shared Variables: " << result.shared_variables.size() << '\n'
                      << "Shared Pruned By Cap: " << result.shared_diagnostics.pruned_by_cap << '\n'
                      << "Z Rows: " << result.z.size() << '\n'
                      << "Z Columns: " << (result.z.empty() ? 0 : result.z.front().size()) << '\n';
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "soft_overlap_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
