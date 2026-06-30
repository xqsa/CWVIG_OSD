#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingIO.h"
#include "grouping/HardOverlapCompatibilityAudit.h"

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

void ensureParentDirectory(const std::string &path)
{
    const std::filesystem::path output_path(path);
    const auto parent = output_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

void printUsage()
{
    std::cout << "Usage: hard_overlap_sanitize_cli --po PATH --oo PATH --expected-dimension N "
                 "--repair-policy none|drop_empty_groups|demote_min_shared|merge_empty_to_nearest "
                 "--output-po PATH --output-oo PATH [--output-report PATH] [--print-summary]\n";
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
        const auto policy = flyki::grouping::parseHardOverlapRepairPolicy(
            requireArg(args, "--repair-policy"));
        const auto output_po = requireArg(args, "--output-po");
        const auto output_oo = requireArg(args, "--output-oo");

        const auto grouping = flyki::grouping::loadLegacyGroupingFromFiles(po_path, oo_path);
        const auto result = flyki::grouping::sanitizeHardOverlapGrouping(
            grouping,
            expected_dimension,
            policy);

        ensureParentDirectory(output_po);
        ensureParentDirectory(output_oo);
        flyki::grouping::writePoFile(output_po, result.grouping.groups);
        flyki::grouping::writeOoFile(output_oo, result.grouping.overlap_groups);

        if (hasArg(args, "--output-report")) {
            const auto path = args.at("--output-report");
            ensureParentDirectory(path);
            flyki::grouping::writeHardOverlapRepairCsv(path, result);
        }
        if (hasArg(args, "--print-summary")) {
            std::cout << "Repair Policy: "
                      << flyki::grouping::hardOverlapRepairPolicyName(policy) << '\n'
                      << "Before Repair\n"
                      << flyki::grouping::formatHardOverlapCompatibilitySummary(result.before)
                      << "After Repair\n"
                      << flyki::grouping::formatHardOverlapCompatibilitySummary(result.after)
                      << flyki::grouping::formatHardOverlapRepairActions(result.actions);
        }
        if (policy == flyki::grouping::HardOverlapRepairPolicy::None) {
            return 0;
        }
        return result.after.compatible ? 0 : 2;
    } catch (const std::exception &error) {
        std::cerr << "hard_overlap_sanitize_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
