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

LegacyGroupingView loadLegacyGroupingFromPaths(
    const std::filesystem::path &po_path,
    const std::filesystem::path &oo_path,
    const std::string &missing_po_message,
    const std::string &missing_oo_message,
    const std::string &validation_message)
{
    if (!std::filesystem::exists(po_path)) {
        throw std::runtime_error(missing_po_message + ": " + po_path.string());
    }
    if (!std::filesystem::exists(oo_path)) {
        throw std::runtime_error(missing_oo_message + ": " + oo_path.string());
    }

    const auto groups = readPoFile(po_path.string());
    const auto overlap_groups = readOoFile(oo_path.string());
    auto view = makeLegacyGroupingView(makeGroupingData(groups, overlap_groups));
    const auto errors = validateLegacyGroupingView(view);
    if (!errors.empty()) {
        throw std::runtime_error(validation_message + ": " + joinErrors(errors));
    }
    return view;
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
    return loadLegacyGroupingFromPaths(
        po_path,
        oo_path,
        "Missing po file",
        "Missing oo file",
        "Legacy grouping validation failed");
}

LegacyGroupingView loadLegacyGroupingFromFiles(
    const std::filesystem::path &po_path,
    const std::filesystem::path &oo_path)
{
    return loadLegacyGroupingFromPaths(
        po_path,
        oo_path,
        "Missing explicit po file",
        "Missing explicit oo file",
        "Explicit grouping validation failed");
}

}  // namespace grouping
}  // namespace flyki
