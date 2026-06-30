#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingData.h"
#include "grouping/HardOverlapCompatibilityAudit.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using flyki::grouping::HardOverlapRepairPolicy;
    using flyki::grouping::auditHardOverlapCompatibility;
    using flyki::grouping::loadLegacyGroupingForFunction;
    using flyki::grouping::makeGroupingData;
    using flyki::grouping::makeLegacyGroupingView;
    using flyki::grouping::sanitizeHardOverlapGrouping;
    using flyki::grouping::validateLegacyGroupingView;

    const auto legacy = loadLegacyGroupingForFunction(1, "benchmark/flyki_overlap");
    const auto legacy_audit = auditHardOverlapCompatibility(legacy, 905);
    require(legacy_audit.group_count == 20, "legacy group count");
    require(legacy_audit.shared_variable_count == 95, "legacy shared variable count");
    require(legacy_audit.zero_effective_group_count == 0, "legacy hard-overlap compatible");
    require(legacy_audit.compatible, "legacy compatibility flag");

    const auto incompatible = makeLegacyGroupingView(makeGroupingData(
        {{0, 1}, {1}, {2}},
        {{1}, {1}, {}},
        3));
    const auto incompatible_audit = auditHardOverlapCompatibility(incompatible, 3);
    require(incompatible_audit.group_count == 3, "synthetic group count");
    require(incompatible_audit.original_group_sizes == std::vector<std::size_t>({2, 1, 1}), "original group sizes");
    require(incompatible_audit.effective_group_sizes == std::vector<std::size_t>({1, 0, 1}), "effective group sizes");
    require(incompatible_audit.zero_effective_group_count == 1, "zero-effective count");
    require(incompatible_audit.zero_effective_group_indices == std::vector<std::size_t>({1}), "zero-effective index");
    require(incompatible_audit.min_effective_group_size == 0, "min effective size");
    require(incompatible_audit.max_effective_group_size == 1, "max effective size");
    require(incompatible_audit.mean_effective_group_size > 0.66, "mean effective size");
    require(!incompatible_audit.compatible, "incompatible flag");

    const auto unchanged = sanitizeHardOverlapGrouping(incompatible, 3, HardOverlapRepairPolicy::None);
    require(!unchanged.after.compatible, "none policy leaves incompatible");
    require(unchanged.actions.empty(), "none policy has no actions");

    const auto repaired = sanitizeHardOverlapGrouping(incompatible, 3, HardOverlapRepairPolicy::DemoteMinShared);
    require(repaired.after.compatible, "demote_min_shared repairs compatibility");
    require(!repaired.actions.empty(), "demote_min_shared logs repair");
    require(validateLegacyGroupingView(repaired.grouping, true).empty(), "repaired grouping validates");

    std::cout << "HardOverlapCompatibilityTests passed\n";
    return 0;
}
