#ifndef FLYKI_GROUPING_HARD_OVERLAP_COMPATIBILITY_AUDIT_H_
#define FLYKI_GROUPING_HARD_OVERLAP_COMPATIBILITY_AUDIT_H_

#include "grouping/LegacyGroupingAdapter.h"

#include <cstddef>
#include <string>
#include <vector>

namespace flyki {
namespace grouping {

struct HardOverlapCompatibilityAudit {
    int expected_dimension = 0;
    std::size_t group_count = 0;
    std::size_t shared_variable_count = 0;
    std::vector<std::size_t> original_group_sizes;
    std::vector<std::size_t> effective_group_sizes;
    std::size_t zero_effective_group_count = 0;
    std::vector<std::size_t> zero_effective_group_indices;
    std::size_t min_effective_group_size = 0;
    std::size_t max_effective_group_size = 0;
    double mean_effective_group_size = 0.0;
    bool compatible = true;
};

enum class HardOverlapRepairPolicy {
    None,
    DropEmptyGroups,
    DemoteMinShared,
    MergeEmptyToNearest,
};

struct HardOverlapRepairAction {
    std::string policy;
    std::size_t group_index = 0;
    int variable = -1;
    std::string detail;
};

struct HardOverlapSanitizerResult {
    LegacyGroupingView grouping;
    HardOverlapCompatibilityAudit before;
    HardOverlapCompatibilityAudit after;
    std::vector<HardOverlapRepairAction> actions;
};

HardOverlapRepairPolicy parseHardOverlapRepairPolicy(const std::string &name);
std::string hardOverlapRepairPolicyName(HardOverlapRepairPolicy policy);

HardOverlapCompatibilityAudit auditHardOverlapCompatibility(
    const LegacyGroupingView &view,
    int expected_dimension);

HardOverlapSanitizerResult sanitizeHardOverlapGrouping(
    const LegacyGroupingView &view,
    int expected_dimension,
    HardOverlapRepairPolicy policy);

std::string formatHardOverlapCompatibilitySummary(const HardOverlapCompatibilityAudit &audit);
std::string formatHardOverlapRepairActions(const std::vector<HardOverlapRepairAction> &actions);

void writeHardOverlapAuditCsv(const std::string &path, const HardOverlapCompatibilityAudit &audit);
void writeHardOverlapRepairCsv(const std::string &path, const HardOverlapSanitizerResult &result);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_HARD_OVERLAP_COMPATIBILITY_AUDIT_H_
