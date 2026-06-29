#ifndef FLYKI_GROUPING_LEGACY_GROUPING_ADAPTER_H_
#define FLYKI_GROUPING_LEGACY_GROUPING_ADAPTER_H_

#include "grouping/GroupingData.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace flyki {
namespace grouping {

struct LegacyGroupingView {
    std::vector<std::vector<int>> groups;
    std::vector<std::vector<int>> overlap_groups;
    std::vector<std::vector<int>> overiablesRedandunt;
    std::vector<std::vector<int>> overiables;
    std::map<int, std::vector<std::pair<int, int>>> sharedvar_group_pos;
    int dimension = 0;
    std::size_t number_of_groups = 0;
};

LegacyGroupingView makeLegacyGroupingView(const GroupingData &data);

std::vector<std::string> validateLegacyGroupingView(
    const LegacyGroupingView &view,
    bool dimension_is_explicit = false);

std::string formatSharedMap(const LegacyGroupingView &view);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_LEGACY_GROUPING_ADAPTER_H_
