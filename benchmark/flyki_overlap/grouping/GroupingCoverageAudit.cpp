#include "grouping/GroupingCoverageAudit.h"

#include "grouping/GroupingData.h"

#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

std::map<int, int> variableCounts(const LegacyGroupingView &view)
{
    std::map<int, int> counts;
    for (const auto &group : view.groups) {
        for (const int variable : group) {
            ++counts[variable];
        }
    }
    return counts;
}

void collectOutOfRangeVariables(
    const std::vector<std::vector<int>> &groups,
    const int expected_dimension,
    std::set<int> &variables)
{
    for (const auto &group : groups) {
        for (const int variable : group) {
            if (variable < 0 || variable >= expected_dimension) {
                variables.insert(variable);
            }
        }
    }
}

}  // namespace

CompletionPolicy parseCompletionPolicy(const std::string &name)
{
    if (name == "none") {
        return CompletionPolicy::None;
    }
    if (name == "singletons") {
        return CompletionPolicy::Singletons;
    }
    if (name == "tail_group") {
        return CompletionPolicy::TailGroup;
    }
    throw std::invalid_argument("Invalid completion policy: " + name);
}

std::string completionPolicyName(const CompletionPolicy policy)
{
    switch (policy) {
    case CompletionPolicy::None:
        return "none";
    case CompletionPolicy::Singletons:
        return "singletons";
    case CompletionPolicy::TailGroup:
        return "tail_group";
    }
    return "unknown";
}

int expectedFlykiOverlapDimension(const int func)
{
    if (func < 1 || func > 12) {
        throw std::invalid_argument("Invalid Flyki function id.");
    }
    // Same Flyki overlap benchmark dimension used by BenchmarkEvaluator.
    return 905;
}

GroupingCoverageAudit auditGroupingCoverage(
    const LegacyGroupingView &view,
    const int expected_dimension)
{
    if (expected_dimension < 0) {
        throw std::invalid_argument("Expected dimension must be non-negative.");
    }

    GroupingCoverageAudit audit;
    audit.expected_dimension = expected_dimension;
    audit.grouping_dimension = view.dimension;

    std::set<int> out_of_range_variables;
    collectOutOfRangeVariables(view.groups, expected_dimension, out_of_range_variables);
    collectOutOfRangeVariables(view.overiablesRedandunt, expected_dimension, out_of_range_variables);
    audit.out_of_range_variables = out_of_range_variables.size();

    const auto counts = variableCounts(view);
    for (const auto &entry : counts) {
        const int variable = entry.first;
        const int count = entry.second;
        if (variable < 0 || variable >= expected_dimension) {
            continue;
        }
        ++audit.covered_unique_variables;
        if (count > 1) {
            audit.duplicate_variable_occurrences += static_cast<std::size_t>(count - 1);
        }
    }

    for (int variable = 0; variable < expected_dimension; ++variable) {
        if (counts.find(variable) == counts.end()) {
            audit.missing_variables.push_back(variable);
        }
    }
    audit.missing_variable_count = audit.missing_variables.size();
    audit.coverage_ratio = expected_dimension == 0
        ? 1.0
        : static_cast<double>(audit.covered_unique_variables) / static_cast<double>(expected_dimension);
    audit.full_coverage = audit.missing_variable_count == 0 && audit.out_of_range_variables == 0;
    return audit;
}

LegacyGroupingView completeGroupingCoverage(
    const LegacyGroupingView &view,
    const int expected_dimension,
    const CompletionPolicy policy)
{
    if (policy == CompletionPolicy::None) {
        return view;
    }

    auto groups = view.groups;
    auto overlap_groups = view.overiablesRedandunt;
    const auto audit = auditGroupingCoverage(view, expected_dimension);
    if (audit.missing_variables.empty()) {
        return view;
    }

    if (policy == CompletionPolicy::Singletons) {
        for (const int variable : audit.missing_variables) {
            groups.push_back({variable});
            overlap_groups.push_back({});
        }
    } else if (policy == CompletionPolicy::TailGroup) {
        groups.push_back(audit.missing_variables);
        overlap_groups.push_back({});
    }

    return makeLegacyGroupingView(makeGroupingData(groups, overlap_groups, expected_dimension));
}

std::string formatCoverageSummary(const GroupingCoverageAudit &audit)
{
    std::ostringstream output;
    output << std::setprecision(12)
           << "Expected Dimension: " << audit.expected_dimension << '\n'
           << "Grouping Inferred Dimension: " << audit.grouping_dimension << '\n'
           << "Covered Unique Variables: " << audit.covered_unique_variables << '\n'
           << "Missing Variable Count: " << audit.missing_variable_count << '\n'
           << "Duplicate Variable Occurrences: " << audit.duplicate_variable_occurrences << '\n'
           << "Out Of Range Variables: " << audit.out_of_range_variables << '\n'
           << "Coverage Ratio: " << audit.coverage_ratio << '\n'
           << "Full Coverage: " << (audit.full_coverage ? "true" : "false") << '\n';
    return output.str();
}

}  // namespace grouping
}  // namespace flyki
