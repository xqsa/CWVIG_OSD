#include "grouping/CBOCCGroupingLoader.h"

#include "grouping/GroupingIO.h"

#include <sstream>
#include <stdexcept>
#include <string>

namespace flyki {
namespace grouping {
namespace {

std::filesystem::path legacyGroupingPath(
    const std::filesystem::path &benchmark_root,
    const int func,
    const std::string &suffix)
{
    return benchmark_root / (std::to_string(func) + suffix);
}

std::string joinErrors(const std::vector<std::string> &errors)
{
    std::ostringstream output;
    for (std::size_t i = 0; i < errors.size(); ++i) {
        if (i > 0) {
            output << "; ";
        }
        output << errors[i];
    }
    return output.str();
}

}  // namespace

LegacyGroupingView loadLegacyGroupingForFunction(
    const int func,
    const std::filesystem::path &benchmark_root)
{
    if (func < 1 || func > 12) {
        throw std::invalid_argument("Invalid function id: " + std::to_string(func));
    }

    const auto po_path = legacyGroupingPath(benchmark_root, func, "po.txt");
    const auto oo_path = legacyGroupingPath(benchmark_root, func, "oo.txt");
    if (!std::filesystem::exists(po_path)) {
        throw std::runtime_error("Missing po file: " + po_path.string());
    }
    if (!std::filesystem::exists(oo_path)) {
        throw std::runtime_error("Missing oo file: " + oo_path.string());
    }

    const auto groups = readPoFile(po_path.string());
    const auto overlap_groups = readOoFile(oo_path.string());
    auto view = makeLegacyGroupingView(makeGroupingData(groups, overlap_groups));
    const auto errors = validateLegacyGroupingView(view);
    if (!errors.empty()) {
        throw std::runtime_error("Legacy grouping validation failed: " + joinErrors(errors));
    }
    return view;
}

}  // namespace grouping
}  // namespace flyki
