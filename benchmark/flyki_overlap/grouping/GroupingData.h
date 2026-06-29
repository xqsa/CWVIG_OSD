#ifndef FLYKI_GROUPING_GROUPING_DATA_H_
#define FLYKI_GROUPING_GROUPING_DATA_H_

#include <set>
#include <vector>

namespace flyki {
namespace grouping {

struct GroupingData {
    std::vector<std::vector<int>> groups;
    std::vector<std::vector<int>> overlap_groups;
    std::set<int> unique_variables;
    std::set<int> shared_variables;
    int dimension = 0;
};

GroupingData makeGroupingData(
    const std::vector<std::vector<int>> &groups,
    const std::vector<std::vector<int>> &overlap_groups,
    int explicit_dimension = -1);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_GROUPING_DATA_H_
