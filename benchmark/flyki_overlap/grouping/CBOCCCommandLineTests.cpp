#include "grouping/CBOCCCommandLine.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

template <typename Fn>
void requireThrowsContaining(Fn fn, const std::string &needle, const std::string &message)
{
    try {
        fn();
    } catch (const std::exception &error) {
        if (std::string(error.what()).find(needle) != std::string::npos) {
            return;
        }
        std::cerr << "FAIL: " << message << " wrong error: " << error.what() << '\n';
        std::exit(1);
    }
    std::cerr << "FAIL: " << message << " did not throw\n";
    std::exit(1);
}

flyki::grouping::CBOCCCommandLine parse(std::initializer_list<const char *> values)
{
    std::vector<char *> argv;
    for (const char *value : values) {
        argv.push_back(const_cast<char *>(value));
    }
    return flyki::grouping::parseCBOCCCommandLine(static_cast<int>(argv.size()), argv.data());
}

}  // namespace

int main()
{
    const auto legacy = parse({"cbocco", "1", "CBCCO", "2", "1000"});
    require(legacy.func == 1, "legacy func");
    require(legacy.method == "CBCCO", "legacy method");
    require(legacy.seed == 2, "legacy seed");
    require(legacy.maxfes == 1000, "legacy maxfes");
    require(legacy.grouping_source == "legacy_by_function", "legacy default grouping source");
    require(legacy.root == ".", "legacy default root");

    const auto explicit_files = parse({
        "cbocco",
        "1",
        "CBCCO",
        "3",
        "2000",
        "--grouping-source",
        "explicit_files",
        "--po",
        "predicted_groups.txt",
        "--oo",
        "predicted_overlap.txt"});
    require(explicit_files.grouping_source == "explicit_files", "explicit grouping source");
    require(explicit_files.po_path == "predicted_groups.txt", "explicit po path");
    require(explicit_files.oo_path == "predicted_overlap.txt", "explicit oo path");

    const auto rooted = parse({
        "cbocco",
        "2",
        "CBCCO",
        "4",
        "3000",
        "--grouping-source",
        "legacy_by_function",
        "--root",
        "benchmark/flyki_overlap"});
    require(rooted.root == "benchmark/flyki_overlap", "legacy explicit root");

    const auto guarded = parse({
        "cbocco",
        "1",
        "CBCCO",
        "5",
        "4000",
        "--require-full-coverage",
        "true",
        "--allow-partial-grouping",
        "false",
        "--completion-policy",
        "singletons"});
    require(guarded.require_full_coverage, "require full coverage flag");
    require(!guarded.allow_partial_grouping, "allow partial grouping flag");
    require(guarded.completion_policy == "singletons", "completion policy flag");

    requireThrowsContaining(
        []() { parse({"cbocco", "1", "CBCCO", "1"}); },
        "Usage",
        "missing positional argument");
    requireThrowsContaining(
        []() {
            parse({"cbocco", "1", "CBCCO", "1", "1000", "--grouping-source", "explicit_files", "--po", "x"});
        },
        "Missing required argument: --oo",
        "explicit missing oo");
    requireThrowsContaining(
        []() { parse({"cbocco", "1", "CBCCO", "1", "1000", "--grouping-source", "bad"}); },
        "Invalid grouping source",
        "invalid grouping source");
    requireThrowsContaining(
        []() { parse({"cbocco", "1", "CBCCO", "1", "1000", "--unexpected", "x"}); },
        "Invalid argument",
        "unknown option");
    requireThrowsContaining(
        []() { parse({"cbocco", "1", "CBCCO", "1", "1000", "--allow-partial-grouping", "maybe"}); },
        "Expected true or false",
        "invalid boolean");
    requireThrowsContaining(
        []() { parse({"cbocco", "1", "CBCCO", "1", "1000", "--completion-policy", "mystery"}); },
        "Invalid completion policy",
        "invalid completion policy");

    std::cout << "CBOCCCommandLineTests passed\n";
    return 0;
}
