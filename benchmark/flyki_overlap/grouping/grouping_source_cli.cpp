#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingMetrics.h"

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

void printUsage()
{
    std::cout << "Usage: grouping_source_cli --source legacy_by_function --func ID --root PATH "
                 "[--print-summary] [--dump-shared-map PATH]\n"
                 "   or: grouping_source_cli --source explicit_files --po PATH --oo PATH "
                 "[--print-summary] [--dump-shared-map PATH]\n";
}

void writeTextFile(const std::string &path, const std::string &content)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write file: " + path);
    }
    output << content;
}

flyki::grouping::LegacyGroupingView loadFromArgs(const std::map<std::string, std::string> &args)
{
    const auto source = requireArg(args, "--source");
    if (source == "legacy_by_function") {
        return flyki::grouping::loadLegacyGroupingForFunction(
            std::stoi(requireArg(args, "--func")),
            requireArg(args, "--root"));
    }
    if (source == "explicit_files") {
        return flyki::grouping::loadLegacyGroupingFromFiles(
            requireArg(args, "--po"),
            requireArg(args, "--oo"));
    }
    throw std::runtime_error("Invalid grouping source mode: " + source);
}

void printSummary(const flyki::grouping::LegacyGroupingView &view)
{
    const auto summary = flyki::grouping::summarizeGroups(view.groups);
    const auto errors = flyki::grouping::validateLegacyGroupingView(view);
    std::cout << "Number Of Groups: " << view.number_of_groups << '\n'
              << "Dimension: " << view.dimension << '\n'
              << "Unique Variables: " << summary.number_unique_variables << '\n'
              << "Shared Variables: " << flyki::grouping::extractSharedVariablesFromOo(view.overiablesRedandunt).size()
              << '\n'
              << "Sharedvar Group Pos Entries: " << view.sharedvar_group_pos.size() << '\n'
              << "Validation Errors: " << errors.size() << '\n';
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

        const auto view = loadFromArgs(args);
        if (args.find("--print-summary") != args.end() || args.find("--dump-shared-map") == args.end()) {
            printSummary(view);
        }

        const auto dump_iter = args.find("--dump-shared-map");
        if (dump_iter != args.end()) {
            writeTextFile(dump_iter->second, flyki::grouping::formatSharedMap(view));
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "grouping_source_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
