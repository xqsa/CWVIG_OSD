#include "grouping/CWVIGGroupingPipeline.h"

#include <fstream>
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

bool hasArg(const std::map<std::string, std::string> &args, const std::string &key)
{
    return args.find(key) != args.end();
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

void printUsage()
{
    std::cout << "Usage: cwvig_grouping_pipeline_cli [--config PATH] [--mode synthetic|flyki] --output-dir PATH "
                 "[--preset conservative|balanced|capped] [--synthetic-function sphere|interaction|overlap|clique] "
                 "[--func ID] [--dimension-limit K] [--contexts R] [--seed SEED] [--delta VALUE] "
                 "[--edge-score-column probability|mean_abs_normalized|mean_abs_raw] "
                 "[--edge-weight-mode raw|log1p|rank_normalized|uncertainty_penalized] "
                 "[--sparsification-mode score_threshold|top_k_per_node|mutual_top_k|target_avg_degree] "
                 "[--target-avg-degree VALUE] [--top-k-per-node K] [--max-avg-degree VALUE] "
                 "[--membership-transform current_ratio|rank_normalized|minmax_normalized|sigmoid_centered] "
                 "[--shared-rule hard_membership_only|hard_plus_second_membership|capped_shared] "
                 "[--max-shared-ratio VALUE] [--shared-min-second-membership VALUE] "
                 "[--true-po PATH --true-oo PATH] [--print-summary]\n";
}

flyki::grouping::PipelineMode parseMode(const std::string &mode)
{
    if (mode == "synthetic") {
        return flyki::grouping::PipelineMode::Synthetic;
    }
    if (mode == "flyki") {
        return flyki::grouping::PipelineMode::Flyki;
    }
    throw std::runtime_error("Invalid mode: " + mode);
}

flyki::grouping::CWVIGGroupingPipelineConfig configFromArgs(
    const std::map<std::string, std::string> &args)
{
    const bool from_config = hasArg(args, "--config");
    auto config = from_config
        ? flyki::grouping::loadPipelineConfigFile(requireArg(args, "--config"))
        : flyki::grouping::pipelineConfigForPreset(flyki::grouping::parsePipelinePreset(
            optionalArg(args, "--preset", "balanced")));
    if (hasArg(args, "--preset")) {
        config.preset = flyki::grouping::parsePipelinePreset(requireArg(args, "--preset"));
    }
    if (hasArg(args, "--mode")) {
        config.mode = parseMode(requireArg(args, "--mode"));
    } else if (!from_config) {
        config.mode = parseMode(requireArg(args, "--mode"));
    }
    config.output_dir = requireArg(args, "--output-dir");
    config.synthetic_function = optionalArg(args, "--synthetic-function", config.synthetic_function);
    config.func = std::stoi(optionalArg(args, "--func", std::to_string(config.func)));
    config.dimension_limit = static_cast<std::size_t>(std::stoull(optionalArg(
        args,
        "--dimension-limit",
        std::to_string(config.dimension_limit))));
    config.contexts = static_cast<std::size_t>(std::stoull(optionalArg(args, "--contexts", std::to_string(config.contexts))));
    config.seed = static_cast<unsigned>(std::stoul(optionalArg(args, "--seed", std::to_string(config.seed))));
    config.delta = std::stod(optionalArg(args, "--delta", std::to_string(config.delta)));
    config.edge_score_column = flyki::grouping::parseInteractionScoreColumn(optionalArg(
        args,
        "--edge-score-column",
        flyki::grouping::interactionScoreColumnName(config.edge_score_column)));
    config.edge_weight_mode = flyki::grouping::parseEdgeWeightMode(optionalArg(
        args,
        "--edge-weight-mode",
        flyki::grouping::edgeWeightModeName(config.edge_weight_mode)));
    config.sparsification_mode = flyki::grouping::parseSparsificationMode(optionalArg(
        args,
        "--sparsification-mode",
        flyki::grouping::sparsificationModeName(config.sparsification_mode)));
    config.target_avg_degree = std::stod(optionalArg(args, "--target-avg-degree", std::to_string(config.target_avg_degree)));
    config.top_k_per_node = static_cast<std::size_t>(std::stoull(optionalArg(
        args,
        "--top-k-per-node",
        std::to_string(config.top_k_per_node))));
    config.max_avg_degree = std::stod(optionalArg(args, "--max-avg-degree", std::to_string(config.max_avg_degree)));
    config.membership_transform = flyki::grouping::parseMembershipTransform(optionalArg(
        args,
        "--membership-transform",
        flyki::grouping::membershipTransformName(config.membership_transform)));
    config.shared_rule = flyki::grouping::parseSharedVariableRule(optionalArg(
        args,
        "--shared-rule",
        flyki::grouping::sharedVariableRuleName(config.shared_rule)));
    config.max_shared_ratio = std::stod(optionalArg(args, "--max-shared-ratio", std::to_string(config.max_shared_ratio)));
    config.shared_min_second_membership = std::stod(optionalArg(
        args,
        "--shared-min-second-membership",
        std::to_string(config.shared_min_second_membership)));
    if (hasArg(args, "--true-po") || hasArg(args, "--true-oo")) {
        config.true_po_path = requireArg(args, "--true-po");
        config.true_oo_path = requireArg(args, "--true-oo");
    }
    return config;
}

void printFile(const std::filesystem::path &path)
{
    std::ifstream input(path);
    std::cout << input.rdbuf();
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
        const auto result = flyki::grouping::runCWVIGGroupingPipeline(configFromArgs(args));
        if (hasArg(args, "--print-summary")) {
            printFile(result.outputs.pipeline_summary);
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "cwvig_grouping_pipeline_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
