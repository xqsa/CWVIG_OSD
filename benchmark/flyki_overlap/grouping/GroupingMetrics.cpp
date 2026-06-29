#include "grouping/GroupingMetrics.h"

#include <algorithm>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace flyki {
namespace grouping {
namespace {

void requireNonNegative(const int variable)
{
    if (variable < 0) {
        throw std::invalid_argument("Grouping variable ids must be non-negative.");
    }
}

double divideOrOneWhenBothEmpty(const std::size_t numerator, const std::size_t denominator, const bool both_empty)
{
    if (denominator == 0) {
        return both_empty ? 1.0 : 0.0;
    }
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

}  // namespace

std::set<int> extractSharedVariablesFromGroups(const std::vector<std::vector<int>> &groups)
{
    std::map<int, int> occurrences;
    for (const auto &group : groups) {
        for (const int variable : group) {
            requireNonNegative(variable);
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

std::set<int> extractSharedVariablesFromOo(const std::vector<std::vector<int>> &overlap_groups)
{
    std::set<int> shared;
    for (const auto &group : overlap_groups) {
        for (const int variable : group) {
            requireNonNegative(variable);
            shared.insert(variable);
        }
    }
    return shared;
}

SetMetrics calculateSetMetrics(const std::set<int> &truth, const std::set<int> &prediction)
{
    SetMetrics metrics;
    for (const int variable : prediction) {
        if (truth.find(variable) != truth.end()) {
            ++metrics.true_positive;
        } else {
            ++metrics.false_positive;
        }
    }

    for (const int variable : truth) {
        if (prediction.find(variable) == prediction.end()) {
            ++metrics.false_negative;
        }
    }

    const bool both_empty = truth.empty() && prediction.empty();
    metrics.precision = divideOrOneWhenBothEmpty(
        metrics.true_positive, metrics.true_positive + metrics.false_positive, both_empty);
    metrics.recall = divideOrOneWhenBothEmpty(
        metrics.true_positive, metrics.true_positive + metrics.false_negative, both_empty);

    if (metrics.precision + metrics.recall == 0.0) {
        metrics.f1 = 0.0;
    } else {
        metrics.f1 = 2.0 * metrics.precision * metrics.recall / (metrics.precision + metrics.recall);
    }
    return metrics;
}

GroupSummary summarizeGroups(const std::vector<std::vector<int>> &groups)
{
    GroupSummary summary;
    summary.number_groups = groups.size();
    if (groups.empty()) {
        return summary;
    }

    std::set<int> unique_variables;
    std::vector<std::size_t> sizes;
    sizes.reserve(groups.size());

    for (const auto &group : groups) {
        sizes.push_back(group.size());
        summary.total_variable_occurrences += group.size();
        for (const int variable : group) {
            requireNonNegative(variable);
            unique_variables.insert(variable);
        }
    }

    summary.number_unique_variables = unique_variables.size();
    summary.min_group_size = *std::min_element(sizes.begin(), sizes.end());
    summary.max_group_size = *std::max_element(sizes.begin(), sizes.end());
    summary.mean_group_size =
        static_cast<double>(summary.total_variable_occurrences) / static_cast<double>(summary.number_groups);
    summary.number_shared_variables = extractSharedVariablesFromGroups(groups).size();
    return summary;
}

}  // namespace grouping
}  // namespace flyki
