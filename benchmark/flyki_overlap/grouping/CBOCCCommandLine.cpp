#include "grouping/CBOCCCommandLine.h"

#include "grouping/GroupingCoverageAudit.h"

#include <stdexcept>
#include <string>

namespace flyki {
namespace grouping {
namespace {

bool needsValue(const std::string &key)
{
    return key == "--grouping-source"
        || key == "--po"
        || key == "--oo"
        || key == "--root"
        || key == "--require-full-coverage"
        || key == "--allow-partial-grouping"
        || key == "--require-hard-overlap-compatible"
        || key == "--allow-hard-overlap-incompatible"
        || key == "--completion-policy";
}

bool parseBool(const std::string &key, const std::string &value)
{
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::runtime_error("Expected true or false for " + key + ": " + value);
}

}  // namespace

std::string cboccoUsage()
{
    return "Usage: cbocco <func> CBCCO <seed> <maxfes> "
           "[--grouping-source legacy_by_function] [--root PATH] "
           "[--require-full-coverage true|false] [--allow-partial-grouping true|false] "
           "[--require-hard-overlap-compatible true|false] [--allow-hard-overlap-incompatible true|false] "
           "[--completion-policy none|singletons|tail_group]\n"
           "   or: cbocco <func> CBCCO <seed> <maxfes> "
           "--grouping-source explicit_files --po PATH --oo PATH "
           "[--require-full-coverage true|false] [--allow-partial-grouping true|false] "
           "[--require-hard-overlap-compatible true|false] [--allow-hard-overlap-incompatible true|false] "
           "[--completion-policy none|singletons|tail_group]";
}

CBOCCCommandLine parseCBOCCCommandLine(const int argc, char **argv)
{
    if (argc < 5) {
        throw std::runtime_error(cboccoUsage());
    }

    CBOCCCommandLine options;
    options.func = std::stoi(argv[1]);
    options.method = argv[2];
    options.seed = std::stoi(argv[3]);
    options.maxfes = std::stol(argv[4]);

    for (int i = 5; i < argc; ++i) {
        const std::string key = argv[i];
        if (!needsValue(key) || i + 1 >= argc) {
            throw std::runtime_error("Invalid argument: " + key);
        }
        const std::string value = argv[++i];
        if (key == "--grouping-source") {
            options.grouping_source = value;
        } else if (key == "--po") {
            options.po_path = value;
        } else if (key == "--oo") {
            options.oo_path = value;
        } else if (key == "--root") {
            options.root = value;
        } else if (key == "--require-full-coverage") {
            options.require_full_coverage = parseBool(key, value);
        } else if (key == "--allow-partial-grouping") {
            options.allow_partial_grouping = parseBool(key, value);
        } else if (key == "--require-hard-overlap-compatible") {
            options.require_hard_overlap_compatible = parseBool(key, value);
        } else if (key == "--allow-hard-overlap-incompatible") {
            options.allow_hard_overlap_incompatible = parseBool(key, value);
        } else if (key == "--completion-policy") {
            parseCompletionPolicy(value);
            options.completion_policy = value;
        }
    }

    if (options.grouping_source != "legacy_by_function" && options.grouping_source != "explicit_files") {
        throw std::runtime_error("Invalid grouping source: " + options.grouping_source);
    }
    if (options.grouping_source == "explicit_files") {
        if (options.po_path.empty()) {
            throw std::runtime_error("Missing required argument: --po");
        }
        if (options.oo_path.empty()) {
            throw std::runtime_error("Missing required argument: --oo");
        }
    }
    return options;
}

}  // namespace grouping
}  // namespace flyki
