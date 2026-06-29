#include "grouping/CBOCCGroupingLoader.h"
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
    using flyki::grouping::formatSharedMap;
    using flyki::grouping::loadLegacyGroupingForFunction;

    const auto real = loadLegacyGroupingForFunction(1, "benchmark/flyki_overlap");
    require(real.number_of_groups == 20, "real group count");
    require(real.dimension == 905, "real dimension");
    require(real.groups.size() == 20, "real groups size");
    require(real.overiablesRedandunt.size() == 20, "real raw overlap size");
    require(real.sharedvar_group_pos.size() == 95, "real shared map entries");
    require(formatSharedMap(real) == formatSharedMap(loadLegacyGroupingForFunction(1, "benchmark/flyki_overlap")),
        "shared map dump should be deterministic");

    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_cbocco_loader_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    writeFile(temp_root / "1oo.txt", "1\n0\n");
    requireThrowsContaining(
        [&]() { loadLegacyGroupingForFunction(1, temp_root); },
        "Missing po file",
        "missing po");

    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);
    writeFile(temp_root / "1po.txt", "1\n0\n");
    requireThrowsContaining(
        [&]() { loadLegacyGroupingForFunction(1, temp_root); },
        "Missing oo file",
        "missing oo");

    requireThrowsContaining(
        [&]() { loadLegacyGroupingForFunction(0, "benchmark/flyki_overlap"); },
        "Invalid function id",
        "invalid function id");

    std::filesystem::remove_all(temp_root);
    std::cout << "CBOCCGroupingLoaderTests passed\n";
    return 0;
}
