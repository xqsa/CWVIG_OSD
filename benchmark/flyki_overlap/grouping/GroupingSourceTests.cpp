#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingMetrics.h"
#include "grouping/LegacyGroupingAdapter.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

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

void writeFile(const std::filesystem::path &path, const std::string &content)
{
    std::ofstream output(path);
    output << content;
}

}  // namespace

int main()
{
    using flyki::grouping::extractSharedVariablesFromOo;
    using flyki::grouping::formatSharedMap;
    using flyki::grouping::loadLegacyGroupingForFunction;
    using flyki::grouping::loadLegacyGroupingFromFiles;
    using flyki::grouping::summarizeGroups;

    const auto legacy = loadLegacyGroupingForFunction(1, "benchmark/flyki_overlap");
    const auto explicit_same = loadLegacyGroupingFromFiles(
        "benchmark/flyki_overlap/1po.txt",
        "benchmark/flyki_overlap/1oo.txt");

    require(explicit_same.number_of_groups == 20, "explicit real group count");
    require(explicit_same.dimension == 905, "explicit real dimension");
    require(summarizeGroups(explicit_same.groups).number_unique_variables == 905, "explicit unique variables");
    require(extractSharedVariablesFromOo(explicit_same.overiablesRedandunt).size() == 95, "explicit shared variables");
    require(formatSharedMap(legacy) == formatSharedMap(explicit_same), "legacy and explicit shared maps match");

    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_grouping_source_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    const auto tiny_po = temp_root / "predicted_groups.txt";
    const auto tiny_oo = temp_root / "predicted_overlap.txt";
    writeFile(tiny_po, "2\n0 1\n2\n1 2\n");
    writeFile(tiny_oo, "1\n1\n1\n1\n");
    const auto tiny = loadLegacyGroupingFromFiles(tiny_po, tiny_oo);
    require(tiny.number_of_groups == 2, "tiny explicit group count");
    require(tiny.dimension == 3, "tiny explicit dimension");
    require(formatSharedMap(tiny) == "1: 0,1 1,0\n", "tiny explicit shared map");

    requireThrowsContaining(
        [&]() { loadLegacyGroupingFromFiles(temp_root / "missing_po.txt", tiny_oo); },
        "Missing explicit po file",
        "missing explicit po");
    requireThrowsContaining(
        [&]() { loadLegacyGroupingFromFiles(tiny_po, temp_root / "missing_oo.txt"); },
        "Missing explicit oo file",
        "missing explicit oo");

    writeFile(tiny_oo, "1\n7\n1\n1\n");
    requireThrowsContaining(
        [&]() { loadLegacyGroupingFromFiles(tiny_po, tiny_oo); },
        "validation failed",
        "inconsistent variable ids");

    std::filesystem::remove_all(temp_root);
    std::cout << "GroupingSourceTests passed\n";
    return 0;
}
