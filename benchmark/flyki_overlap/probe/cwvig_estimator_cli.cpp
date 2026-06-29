#include "probe/BenchmarkEvaluator.h"
#include "probe/CWVIGEstimator.h"
#include "probe/SyntheticFunctions.h"

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
        if (key == "--help" || key == "-h") {
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

void printUsage()
{
    std::cout << "Usage: cwvig_estimator_cli --mode synthetic --synthetic-function sphere|interaction|overlap "
                 "--dimension-limit K --contexts R --seed SEED --delta VALUE --threshold-mode fixed|quantile "
                 "--fixed-threshold VALUE --output CSV\n"
                 "   or: cwvig_estimator_cli --mode flyki --func ID --dimension-limit K --contexts R --seed SEED "
                 "--delta VALUE --threshold-mode fixed|quantile --fixed-threshold VALUE --output CSV\n";
}

flyki::probe::CWVIGEstimatorConfig configFromArgs(const std::map<std::string, std::string> &args)
{
    flyki::probe::CWVIGEstimatorConfig config;
    config.dimension_limit = static_cast<std::size_t>(std::stoul(optionalArg(args, "--dimension-limit", "10")));
    config.contexts = static_cast<std::size_t>(std::stoul(optionalArg(args, "--contexts", "3")));
    config.seed = static_cast<unsigned>(std::stoul(optionalArg(args, "--seed", "1")));
    config.delta = std::stod(optionalArg(args, "--delta", "0.0001"));
    config.threshold_mode = optionalArg(args, "--threshold-mode", "fixed");
    config.fixed_threshold = std::stod(optionalArg(args, "--fixed-threshold", "0.5"));
    return config;
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
    throw std::runtime_error("Invalid synthetic function: " + name);
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

        const auto mode = requireArg(args, "--mode");
        const auto output = requireArg(args, "--output");
        const auto config = configFromArgs(args);

        flyki::probe::CWVIGEstimateResult result;
        if (mode == "synthetic") {
            const auto fn = syntheticFunction(optionalArg(args, "--synthetic-function", "interaction"));
            result = flyki::probe::estimateCWVIG(fn, config.dimension_limit, config);
        } else if (mode == "flyki") {
            flyki::probe::BenchmarkEvaluator evaluator(std::stoi(requireArg(args, "--func")));
            result = flyki::probe::estimateCWVIG([&evaluator](const std::vector<double> &candidate) {
                return evaluator.evaluate(candidate);
            }, evaluator.dimension(), config);
        } else {
            throw std::runtime_error("Invalid mode: " + mode);
        }

        flyki::probe::writeCWVIGEdgesCsv(output, result);
        std::cout << "Edges Written: " << result.edges.size() << '\n'
                  << "Threshold: " << result.threshold << '\n'
                  << "FE Count: " << result.function_evaluations << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "cwvig_estimator_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
