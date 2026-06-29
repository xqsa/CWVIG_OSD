#include "probe/BenchmarkEvaluator.h"
#include "probe/FiniteDifferenceProbe.h"
#include "probe/SyntheticFunctions.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

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
    std::cout << "Usage: probe_smoke_cli --mode synthetic --dimension-limit K --seed SEED --delta VALUE --output CSV\n"
                 "   or: probe_smoke_cli --mode flyki --func ID --dimension-limit K --seed SEED --delta VALUE --output CSV\n";
}

std::vector<double> randomVector(const std::size_t dimension, const unsigned seed)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<double> x(dimension);
    for (double &value : x) {
        value = dist(rng);
    }
    return x;
}

void writeHeader(std::ofstream &output)
{
    output << "i,j,evidence\n";
}

void runSynthetic(
    const std::size_t limit,
    const unsigned seed,
    const double delta,
    const std::string &output_path)
{
    if (limit < 3) {
        throw std::runtime_error("Synthetic mode requires dimension-limit >= 3.");
    }

    auto x = randomVector(limit, seed);
    std::size_t evaluations = 0;
    const auto counted_interaction = [&evaluations](const std::vector<double> &candidate) {
        ++evaluations;
        return flyki::probe::twoVariableInteraction(candidate);
    };

    std::ofstream output(output_path);
    if (!output) {
        throw std::runtime_error("Failed to write output CSV: " + output_path);
    }
    writeHeader(output);

    double interaction_evidence = 0.0;
    double separable_max_abs = 0.0;
    for (std::size_t i = 0; i < limit; ++i) {
        for (std::size_t j = i + 1; j < limit; ++j) {
            const double evidence = flyki::probe::probePair(counted_interaction, x, i, j, delta);
            output << i << ',' << j << ',' << evidence << '\n';
            if (i == 0 && j == 1) {
                interaction_evidence = evidence;
            } else {
                separable_max_abs = std::max(separable_max_abs, std::fabs(evidence));
            }
        }
    }

    if (std::fabs(interaction_evidence) <= 1e-12 || separable_max_abs > 1e-10) {
        throw std::runtime_error("Synthetic probe check failed.");
    }

    std::cout << "Synthetic Interaction Evidence: " << interaction_evidence << '\n'
              << "Synthetic Separable Max Abs Evidence: " << separable_max_abs << '\n'
              << "FE Count: " << evaluations << '\n';
}

void runFlyki(
    const int func,
    const std::size_t limit,
    const unsigned seed,
    const double delta,
    const std::string &output_path)
{
    flyki::probe::BenchmarkEvaluator evaluator(func);
    if (limit < 2 || limit > evaluator.dimension()) {
        throw std::runtime_error("Flyki mode requires 2 <= dimension-limit <= 905.");
    }

    auto x = randomVector(evaluator.dimension(), seed);
    std::ofstream output(output_path);
    if (!output) {
        throw std::runtime_error("Failed to write output CSV: " + output_path);
    }
    writeHeader(output);

    for (std::size_t i = 0; i < limit; ++i) {
        for (std::size_t j = i + 1; j < limit; ++j) {
            const double evidence = flyki::probe::probePair([&evaluator](const std::vector<double> &candidate) {
                return evaluator.evaluate(candidate);
            }, x, i, j, delta);
            output << i << ',' << j << ',' << evidence << '\n';
        }
    }

    std::cout << "Flyki Function: " << func << '\n'
              << "Dimension Limit: " << limit << '\n'
              << "FE Count: " << evaluator.evaluations() << '\n';
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
        const unsigned seed = static_cast<unsigned>(std::stoul(optionalArg(args, "--seed", "1")));
        const double delta = std::stod(optionalArg(args, "--delta", "0.0001"));

        if (mode == "synthetic") {
            runSynthetic(
                static_cast<std::size_t>(std::stoul(optionalArg(args, "--dimension-limit", "3"))),
                seed,
                delta,
                output);
            return 0;
        }
        if (mode == "flyki") {
            runFlyki(
                std::stoi(requireArg(args, "--func")),
                static_cast<std::size_t>(std::stoul(optionalArg(args, "--dimension-limit", "10"))),
                seed,
                delta,
                output);
            return 0;
        }

        throw std::runtime_error("Invalid mode: " + mode);
    } catch (const std::exception &error) {
        std::cerr << "probe_smoke_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
