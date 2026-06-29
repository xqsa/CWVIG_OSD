#ifndef FLYKI_GROUPING_GROUPING_METRICS_H_
#define FLYKI_GROUPING_GROUPING_METRICS_H_

#include <cstddef>
#include <set>
#include <vector>

namespace flyki {
namespace grouping {

struct SetMetrics {
    std::size_t true_positive = 0;
    std::size_t false_positive = 0;
    std::size_t false_negative = 0;
    double precision = 0.0;
    double recall = 0.0;
    double f1 = 0.0;
};

struct GroupSummary {
    std::size_t number_groups = 0;
    std::size_t total_variable_occurrences = 0;
    std::size_t number_unique_variables = 0;
    std::size_t min_group_size = 0;
    std::size_t max_group_size = 0;
    double mean_group_size = 0.0;
    std::size_t number_shared_variables = 0;
};

std::set<int> extractSharedVariablesFromGroups(const std::vector<std::vector<int>> &groups);
std::set<int> extractSharedVariablesFromOo(const std::vector<std::vector<int>> &overlap_groups);

SetMetrics calculateSetMetrics(const std::set<int> &truth, const std::set<int> &prediction);
GroupSummary summarizeGroups(const std::vector<std::vector<int>> &groups);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_GROUPING_METRICS_H_
