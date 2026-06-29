#ifndef FLYKI_GROUPING_CBOCC_GROUPING_LOADER_H_
#define FLYKI_GROUPING_CBOCC_GROUPING_LOADER_H_

#include "grouping/LegacyGroupingAdapter.h"

#include <filesystem>

namespace flyki {
namespace grouping {

LegacyGroupingView loadLegacyGroupingForFunction(
    int func,
    const std::filesystem::path &benchmark_root);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_CBOCC_GROUPING_LOADER_H_
