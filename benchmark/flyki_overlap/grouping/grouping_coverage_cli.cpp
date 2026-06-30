#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingCoverageAudit.h"
#include "grouping/GroupingIO.h"

#include <filesystem>
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
        if (key == "--print-summary" || key == "--help" || key == "-h") {
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

bool hasArg(const std::map<std::string, std::string> &args, const std::string &key)
{
    return args.find(key) != args.end();
}

void printUsage()
{
    std::cout << "Usage: grouping_coverage_cli --po PATH --oo PATH --expected-dimension N "
                 "[--completion-policy none|singletons|tail_group] "
                 "[--output-po PATH --output-oo PATH] [--print-summary]\n";
}

void ensureParentDirectory(const std::string &path)
{
    const std::filesystem::path output_path(path);
    const auto parent = output_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

void printAudit(
    const std::string &label,
    const flyki::grouping::LegacyGroupingView &view,
    const int expected_dimension)
{
    const auto audit = flyki::grouping::auditGroupingCoverage(view, expected_dimension);
    const auto errors = flyki::grouping::validateLegacyGroupingView(view, true);
    std::cout << label << '\n'
              << flyki::grouping::formatCoverageSummary(audit)
              << "Validation Errors: " << errors.size() << '\n';
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

        const auto po_path = requireArg(args, "--po");
        const auto oo_path = requireArg(args, "--oo");
        const int expected_dimension = std::stoi(requireArg(args, "--expected-dimension"));
        const auto policy_name = hasArg(args, "--completion-policy")
            ? args.at("--completion-policy")
            : std::string("none");
        const auto policy = flyki::grouping::parseCompletionPolicy(policy_name);

        if (hasArg(args, "--output-po") != hasArg(args, "--output-oo")) {
            throw std::runtime_error("--output-po and --output-oo must be provided together.");
        }

        const auto original = flyki::grouping::loadLegacyGroupingFromFiles(po_path, oo_path);
        const auto completed = flyki::grouping::completeGroupingCoverage(original, expected_dimension, policy);

        const bool should_print = hasArg(args, "--print-summary") || !hasArg(args, "--output-po");
        if (should_print) {
            printAudit("Before Completion", original, expected_dimension);
            std::cout << "Completion Policy: " << flyki::grouping::completionPolicyName(policy) << '\n';
            printAudit("After Completion", completed, expected_dimension);
        }

        if (hasArg(args, "--output-po")) {
            const auto output_po = args.at("--output-po");
            const auto output_oo = args.at("--output-oo");
            ensureParentDirectory(output_po);
            ensureParentDirectory(output_oo);
            flyki::grouping::writePoFile(output_po, completed.groups);
            flyki::grouping::writeOoFile(output_oo, completed.overlap_groups);
        }

        return 0;
    } catch (const std::exception &error) {
        std::cerr << "grouping_coverage_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
