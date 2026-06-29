#include "grouping/GroupingData.h"

#include <limits>
#include <map>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

void addVariablesToSet(const std::vector<std::vector<int>> &groups, std::set<int> &variables)
{
    for (const auto &group : groups) {
        for (const int variable : group) {
            if (variable < 0) {
                throw std::invalid_argument("Grouping variable ids must be non-negative.");
            }
            variables.insert(variable);
        }
    }
}

std::set<int> extractSharedFromGroupOccurrences(const std::vector<std::vector<int>> &groups)
{
    std::map<int, int> occurrences;
    for (const auto &group : groups) {
        for (const int variable : group) {
            if (variable < 0) {
                throw std::invalid_argument("Grouping variable ids must be non-negative.");
            }
            ++occurrences[variable];
        }
    }

    std::set<int> shared;
    for (const auto &entry : occurrences) {
        if (entry.second > 1) {
            shared.insert(entry.first);
        }
    }
    return shared;
}

int inferDimension(const std::set<int> &variables)
{
    if (variables.empty()) {
        return 0;
    }
    const int max_variable = *variables.rbegin();
    if (max_variable == std::numeric_limits<int>::max()) {
        throw std::overflow_error("Cannot infer dimension from INT_MAX variable id.");
    }
    return max_variable + 1;
}

}  // namespace

GroupingData makeGroupingData(
    const std::vector<std::vector<int>> &groups,
    const std::vector<std::vector<int>> &overlap_groups,
    const int explicit_dimension)
{
    GroupingData data;
    data.groups = groups;
    data.overlap_groups = overlap_groups;

    addVariablesToSet(groups, data.unique_variables);
    addVariablesToSet(overlap_groups, data.unique_variables);

    if (!overlap_groups.empty()) {
        addVariablesToSet(overlap_groups, data.shared_variables);
    } else {
        data.shared_variables = extractSharedFromGroupOccurrences(groups);
    }

    if (explicit_dimension >= 0) {
        data.dimension = explicit_dimension;
    } else {
        data.dimension = inferDimension(data.unique_variables);
    }

    if (!data.unique_variables.empty() && data.dimension <= *data.unique_variables.rbegin()) {
        throw std::invalid_argument("Explicit dimension is smaller than the largest variable id.");
    }

    return data;
}

}  // namespace grouping
}  // namespace flyki
