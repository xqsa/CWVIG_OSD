#include "grouping/GroupingIO.h"
#include "grouping/GroupingMetrics.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using flyki::grouping::GroupSummary;

void printUsage()
{
    std::cout << "Usage: grouping_metrics_cli --true-po PATH --true-oo PATH "
                 "[--pred-po PATH] [--pred-oo PATH]\n";
}

std::map<std::string, std::string> parseArgs(const int argc, char **argv)
{
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h") {
            args[key] = "";
            continue;
        }
        if (key.rfind("--", 0) != 0) {
            throw std::runtime_error("Unexpected argument: " + key);
        }
        if (i + 1 >= argc) {
            throw std::runtime_error("Missing value for argument: " + key);
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
    if (iter == args.end()) {
        return fallback;
    }
    return iter->second;
}

void printSummary(const std::string &label, const GroupSummary &summary)
{
    std::cout << label << " Groups: " << summary.number_groups << '\n'
              << label << " Total Variable Occurrences: " << summary.total_variable_occurrences << '\n'
              << label << " Unique Variables: " << summary.number_unique_variables << '\n'
              << label << " Group Size Min: " << summary.min_group_size << '\n'
              << label << " Group Size Max: " << summary.max_group_size << '\n'
              << label << " Group Size Mean: " << summary.mean_group_size << '\n'
              << label << " Shared Variables From Groups: " << summary.number_shared_variables << '\n';
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

        const std::string true_po_path = requireArg(args, "--true-po");
        const std::string true_oo_path = requireArg(args, "--true-oo");
        const std::string pred_po_path = optionalArg(args, "--pred-po", true_po_path);
        const std::string pred_oo_path = optionalArg(args, "--pred-oo", true_oo_path);

        const auto true_groups = flyki::grouping::readPoFile(true_po_path);
        const auto true_overlap_groups = flyki::grouping::readOoFile(true_oo_path);
        const auto pred_groups = flyki::grouping::readPoFile(pred_po_path);
        const auto pred_overlap_groups = flyki::grouping::readOoFile(pred_oo_path);

        const auto true_shared = flyki::grouping::extractSharedVariablesFromOo(true_overlap_groups);
        const auto pred_shared = flyki::grouping::extractSharedVariablesFromOo(pred_overlap_groups);
        const auto metrics = flyki::grouping::calculateSetMetrics(true_shared, pred_shared);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "SharedVar Precision: " << metrics.precision << '\n'
                  << "SharedVar Recall: " << metrics.recall << '\n'
                  << "SharedVar F1: " << metrics.f1 << '\n'
                  << "SharedVar True Positives: " << metrics.true_positive << '\n'
                  << "SharedVar False Positives: " << metrics.false_positive << '\n'
                  << "SharedVar False Negatives: " << metrics.false_negative << '\n';

        printSummary("True", flyki::grouping::summarizeGroups(true_groups));
        printSummary("Pred", flyki::grouping::summarizeGroups(pred_groups));
        printSummary("TrueOO", flyki::grouping::summarizeGroups(true_overlap_groups));
        printSummary("PredOO", flyki::grouping::summarizeGroups(pred_overlap_groups));
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "grouping_metrics_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
