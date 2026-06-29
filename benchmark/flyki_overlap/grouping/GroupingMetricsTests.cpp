#include "grouping/GroupingData.h"
#include "grouping/GroupingIO.h"
#include "grouping/GroupingMetrics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
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

void requireNear(double actual, double expected, const std::string &message)
{
    require(std::fabs(actual - expected) < 1e-12, message);
}

}  // namespace

int main()
{
    using namespace flyki::grouping;

    const std::vector<std::vector<int>> groups = {
        {0, 1, 2},
        {2, 3},
        {4},
    };
    const std::vector<std::vector<int>> overlapGroups = {
        {2},
    };

    const GroupingData data = makeGroupingData(groups, overlapGroups);
    require(data.dimension == 5, "dimension should be inferred from max variable id + 1");
    require(data.unique_variables == std::set<int>({0, 1, 2, 3, 4}), "unique variables mismatch");
    require(data.shared_variables == std::set<int>({2}), "shared variables mismatch");

    const GroupSummary summary = summarizeGroups(groups);
    require(summary.number_groups == 3, "group count mismatch");
    require(summary.total_variable_occurrences == 6, "occurrence count mismatch");
    require(summary.number_unique_variables == 5, "unique count mismatch");
    require(summary.min_group_size == 1, "min group size mismatch");
    require(summary.max_group_size == 3, "max group size mismatch");
    requireNear(summary.mean_group_size, 2.0, "mean group size mismatch");
    require(summary.number_shared_variables == 1, "summary shared count mismatch");

    const auto trueShared = extractSharedVariablesFromOo({{2, 7}, {9}});
    const auto predShared = extractSharedVariablesFromOo({{2, 5}, {9}});
    const SetMetrics metrics = calculateSetMetrics(trueShared, predShared);
    requireNear(metrics.precision, 2.0 / 3.0, "precision mismatch");
    requireNear(metrics.recall, 2.0 / 3.0, "recall mismatch");
    requireNear(metrics.f1, 2.0 / 3.0, "f1 mismatch");

    std::cout << "GroupingMetricsTests passed\n";
    return 0;
}
