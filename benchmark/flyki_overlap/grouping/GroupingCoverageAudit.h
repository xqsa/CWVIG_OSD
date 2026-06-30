#ifndef FLYKI_GROUPING_GROUPING_COVERAGE_AUDIT_H_
#define FLYKI_GROUPING_GROUPING_COVERAGE_AUDIT_H_

#include "grouping/LegacyGroupingAdapter.h"

#include <cstddef>
#include <string>
#include <vector>

namespace flyki {
namespace grouping {

enum class CompletionPolicy {
    None,
    Singletons,
    TailGroup,
};

struct GroupingCoverageAudit {
    int expected_dimension = 0;
    int grouping_dimension = 0;
    std::size_t covered_unique_variables = 0;
    std::size_t missing_variable_count = 0;
    std::size_t duplicate_variable_occurrences = 0;
    std::size_t out_of_range_variables = 0;
    double coverage_ratio = 0.0;
    bool full_coverage = false;
    std::vector<int> missing_variables;
};

CompletionPolicy parseCompletionPolicy(const std::string &name);
std::string completionPolicyName(CompletionPolicy policy);

int expectedFlykiOverlapDimension(int func);

GroupingCoverageAudit auditGroupingCoverage(
    const LegacyGroupingView &view,
    int expected_dimension);

LegacyGroupingView completeGroupingCoverage(
    const LegacyGroupingView &view,
    int expected_dimension,
    CompletionPolicy policy);

std::string formatCoverageSummary(const GroupingCoverageAudit &audit);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_GROUPING_COVERAGE_AUDIT_H_
