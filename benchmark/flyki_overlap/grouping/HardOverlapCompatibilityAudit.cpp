#include "grouping/HardOverlapCompatibilityAudit.h"

#include "grouping/GroupingData.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

std::set<int> hardOverlapSharedVariables(const LegacyGroupingView &view)
{
    std::set<int> shared;
    for (const auto &entry : view.sharedvar_group_pos) {
        shared.insert(entry.first);
    }
    return shared;
}

bool contains(const std::vector<int> &values, const int value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

void appendUnique(std::vector<int> &values, const int value)
{
    if (!contains(values, value)) {
        values.push_back(value);
    }
}

LegacyGroupingView rebuild(
    const std::vector<std::vector<int>> &groups,
    const std::vector<std::vector<int>> &overlap_groups,
    const int expected_dimension)
{
    return makeLegacyGroupingView(makeGroupingData(groups, overlap_groups, expected_dimension));
}

std::string csvText(const std::string &value)
{
    std::string escaped;
    for (const char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped += ch;
        }
    }
    return "\"" + escaped + "\"";
}

int minSharedVariableInGroup(const std::vector<int> &group, const std::set<int> &shared)
{
    int selected = std::numeric_limits<int>::max();
    for (const int variable : group) {
        if (shared.find(variable) != shared.end()) {
            selected = std::min(selected, variable);
        }
    }
    if (selected == std::numeric_limits<int>::max()) {
        throw std::runtime_error("Cannot demote zero-effective group without shared variables.");
    }
    return selected;
}

int removeVariableFromAllOverlapRows(std::vector<std::vector<int>> &overlap_groups, const int variable)
{
    int touched = 0;
    for (auto &overlap_group : overlap_groups) {
        const auto old_size = overlap_group.size();
        overlap_group.erase(
            std::remove(overlap_group.begin(), overlap_group.end(), variable),
            overlap_group.end());
        if (overlap_group.size() != old_size) {
            ++touched;
        }
    }
    return touched;
}

void eraseIndices(
    std::vector<std::vector<int>> &groups,
    std::vector<std::vector<int>> &overlap_groups,
    const std::vector<std::size_t> &indices)
{
    for (auto iter = indices.rbegin(); iter != indices.rend(); ++iter) {
        const auto index = *iter;
        groups.erase(groups.begin() + static_cast<std::ptrdiff_t>(index));
        overlap_groups.erase(overlap_groups.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

std::map<int, int> variableCounts(const std::vector<std::vector<int>> &groups)
{
    std::map<int, int> counts;
    for (const auto &group : groups) {
        for (const int variable : group) {
            ++counts[variable];
        }
    }
    return counts;
}

bool canDropGroupsWithoutCoverageLoss(
    const std::vector<std::vector<int>> &groups,
    const std::vector<std::size_t> &indices)
{
    const auto counts = variableCounts(groups);
    for (const auto index : indices) {
        for (const int variable : groups[index]) {
            const auto iter = counts.find(variable);
            if (iter == counts.end() || iter->second <= 1) {
                return false;
            }
        }
    }
    return true;
}

std::size_t intersectionSize(const std::vector<int> &left, const std::vector<int> &right)
{
    std::size_t count = 0;
    for (const int variable : left) {
        if (contains(right, variable)) {
            ++count;
        }
    }
    return count;
}

std::size_t nearestNonEmptyGroup(
    const std::vector<std::vector<int>> &groups,
    const HardOverlapCompatibilityAudit &audit,
    const std::size_t source_index)
{
    std::set<std::size_t> zero_indices(
        audit.zero_effective_group_indices.begin(),
        audit.zero_effective_group_indices.end());
    std::size_t best_index = groups.size();
    std::size_t best_overlap = 0;
    for (std::size_t index = 0; index < groups.size(); ++index) {
        if (index == source_index || zero_indices.find(index) != zero_indices.end()) {
            continue;
        }
        const auto overlap = intersectionSize(groups[source_index], groups[index]);
        if (best_index == groups.size() || overlap > best_overlap) {
            best_index = index;
            best_overlap = overlap;
        }
    }
    if (best_index == groups.size()) {
        throw std::runtime_error("Cannot merge zero-effective group without a non-empty target group.");
    }
    return best_index;
}

HardOverlapSanitizerResult sanitizeDropEmptyGroups(
    const LegacyGroupingView &view,
    const int expected_dimension,
    const HardOverlapCompatibilityAudit &before)
{
    auto groups = view.groups;
    auto overlap_groups = view.overlap_groups;
    if (!canDropGroupsWithoutCoverageLoss(groups, before.zero_effective_group_indices)) {
        throw std::runtime_error("drop_empty_groups would remove a uniquely covered variable.");
    }
    std::vector<HardOverlapRepairAction> actions;
    for (const auto index : before.zero_effective_group_indices) {
        actions.push_back({"drop_empty_groups", index, -1, "removed zero-effective group"});
    }
    eraseIndices(groups, overlap_groups, before.zero_effective_group_indices);
    auto repaired = rebuild(groups, overlap_groups, expected_dimension);
    return {repaired, before, auditHardOverlapCompatibility(repaired, expected_dimension), actions};
}

HardOverlapSanitizerResult sanitizeDemoteMinShared(
    const LegacyGroupingView &view,
    const int expected_dimension,
    const HardOverlapCompatibilityAudit &before)
{
    auto groups = view.groups;
    auto overlap_groups = view.overlap_groups;
    std::vector<HardOverlapRepairAction> actions;
    auto current = rebuild(groups, overlap_groups, expected_dimension);

    for (int round = 0; round < expected_dimension + 1; ++round) {
        const auto audit = auditHardOverlapCompatibility(current, expected_dimension);
        if (audit.compatible) {
            return {current, before, audit, actions};
        }
        const auto shared = hardOverlapSharedVariables(current);
        const auto index = audit.zero_effective_group_indices.front();
        const int variable = minSharedVariableInGroup(groups[index], shared);
        const int touched = removeVariableFromAllOverlapRows(overlap_groups, variable);
        std::ostringstream detail;
        detail << "removed variable from " << touched << " overlap rows";
        actions.push_back({"demote_min_shared", index, variable, detail.str()});
        current = rebuild(groups, overlap_groups, expected_dimension);
    }
    throw std::runtime_error("demote_min_shared did not converge.");
}

HardOverlapSanitizerResult sanitizeMergeEmptyToNearest(
    const LegacyGroupingView &view,
    const int expected_dimension,
    const HardOverlapCompatibilityAudit &before)
{
    auto groups = view.groups;
    auto overlap_groups = view.overlap_groups;
    std::vector<HardOverlapRepairAction> actions;
    const auto indices = before.zero_effective_group_indices;
    for (const auto source_index : indices) {
        const auto target_index = nearestNonEmptyGroup(groups, before, source_index);
        for (const int variable : groups[source_index]) {
            appendUnique(groups[target_index], variable);
        }
        for (const int variable : overlap_groups[source_index]) {
            appendUnique(overlap_groups[target_index], variable);
        }
        std::ostringstream detail;
        detail << "merged into group " << target_index;
        actions.push_back({"merge_empty_to_nearest", source_index, -1, detail.str()});
    }
    eraseIndices(groups, overlap_groups, indices);
    auto repaired = rebuild(groups, overlap_groups, expected_dimension);
    return {repaired, before, auditHardOverlapCompatibility(repaired, expected_dimension), actions};
}

}  // namespace

HardOverlapRepairPolicy parseHardOverlapRepairPolicy(const std::string &name)
{
    if (name == "none") {
        return HardOverlapRepairPolicy::None;
    }
    if (name == "drop_empty_groups") {
        return HardOverlapRepairPolicy::DropEmptyGroups;
    }
    if (name == "demote_min_shared") {
        return HardOverlapRepairPolicy::DemoteMinShared;
    }
    if (name == "merge_empty_to_nearest") {
        return HardOverlapRepairPolicy::MergeEmptyToNearest;
    }
    throw std::invalid_argument("Invalid hard-overlap repair policy: " + name);
}

std::string hardOverlapRepairPolicyName(const HardOverlapRepairPolicy policy)
{
    switch (policy) {
    case HardOverlapRepairPolicy::None:
        return "none";
    case HardOverlapRepairPolicy::DropEmptyGroups:
        return "drop_empty_groups";
    case HardOverlapRepairPolicy::DemoteMinShared:
        return "demote_min_shared";
    case HardOverlapRepairPolicy::MergeEmptyToNearest:
        return "merge_empty_to_nearest";
    }
    return "unknown";
}

HardOverlapCompatibilityAudit auditHardOverlapCompatibility(
    const LegacyGroupingView &view,
    const int expected_dimension)
{
    if (expected_dimension < 0) {
        throw std::invalid_argument("Expected dimension must be non-negative.");
    }

    HardOverlapCompatibilityAudit audit;
    audit.expected_dimension = expected_dimension;
    audit.group_count = view.groups.size();
    const auto shared = hardOverlapSharedVariables(view);
    audit.shared_variable_count = shared.size();
    audit.original_group_sizes.reserve(view.groups.size());
    audit.effective_group_sizes.reserve(view.groups.size());

    for (std::size_t index = 0; index < view.groups.size(); ++index) {
        const auto &group = view.groups[index];
        audit.original_group_sizes.push_back(group.size());
        std::size_t effective_size = 0;
        for (const int variable : group) {
            if (shared.find(variable) == shared.end()) {
                ++effective_size;
            }
        }
        audit.effective_group_sizes.push_back(effective_size);
        if (effective_size == 0) {
            audit.zero_effective_group_indices.push_back(index);
        }
    }

    audit.zero_effective_group_count = audit.zero_effective_group_indices.size();
    audit.compatible = audit.zero_effective_group_count == 0;
    if (!audit.effective_group_sizes.empty()) {
        const auto minmax = std::minmax_element(
            audit.effective_group_sizes.begin(),
            audit.effective_group_sizes.end());
        audit.min_effective_group_size = *minmax.first;
        audit.max_effective_group_size = *minmax.second;
        const auto total = std::accumulate(
            audit.effective_group_sizes.begin(),
            audit.effective_group_sizes.end(),
            std::size_t{0});
        audit.mean_effective_group_size =
            static_cast<double>(total) / static_cast<double>(audit.effective_group_sizes.size());
    }
    return audit;
}

HardOverlapSanitizerResult sanitizeHardOverlapGrouping(
    const LegacyGroupingView &view,
    const int expected_dimension,
    const HardOverlapRepairPolicy policy)
{
    const auto before = auditHardOverlapCompatibility(view, expected_dimension);
    if (policy == HardOverlapRepairPolicy::None || before.compatible) {
        return {view, before, before, {}};
    }
    if (policy == HardOverlapRepairPolicy::DropEmptyGroups) {
        return sanitizeDropEmptyGroups(view, expected_dimension, before);
    }
    if (policy == HardOverlapRepairPolicy::DemoteMinShared) {
        return sanitizeDemoteMinShared(view, expected_dimension, before);
    }
    if (policy == HardOverlapRepairPolicy::MergeEmptyToNearest) {
        return sanitizeMergeEmptyToNearest(view, expected_dimension, before);
    }
    throw std::runtime_error("Unknown hard-overlap repair policy.");
}

std::string formatHardOverlapCompatibilitySummary(const HardOverlapCompatibilityAudit &audit)
{
    std::ostringstream output;
    output << "Expected Dimension: " << audit.expected_dimension << '\n'
           << "Hard-Overlap Group Count: " << audit.group_count << '\n'
           << "Hard-Overlap Shared Variables: " << audit.shared_variable_count << '\n'
           << "Zero Effective Group Count: " << audit.zero_effective_group_count << '\n'
           << "Min Effective Group Size: " << audit.min_effective_group_size << '\n'
           << "Max Effective Group Size: " << audit.max_effective_group_size << '\n'
           << "Mean Effective Group Size: " << audit.mean_effective_group_size << '\n'
           << "Hard-Overlap Compatible: " << (audit.compatible ? "true" : "false") << '\n';
    if (!audit.zero_effective_group_indices.empty()) {
        output << "Zero Effective Group Indices:";
        for (const auto index : audit.zero_effective_group_indices) {
            output << ' ' << index;
        }
        output << '\n';
    }
    return output.str();
}

std::string formatHardOverlapRepairActions(const std::vector<HardOverlapRepairAction> &actions)
{
    std::ostringstream output;
    output << "Repair Actions: " << actions.size() << '\n';
    for (const auto &action : actions) {
        output << action.policy << " group=" << action.group_index;
        if (action.variable >= 0) {
            output << " variable=" << action.variable;
        }
        output << " detail=" << action.detail << '\n';
    }
    return output.str();
}

void writeHardOverlapAuditCsv(const std::string &path, const HardOverlapCompatibilityAudit &audit)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write hard-overlap audit report: " + path);
    }
    output << "group_index,original_size,effective_size,zero_effective\n";
    for (std::size_t index = 0; index < audit.original_group_sizes.size(); ++index) {
        output << index << ','
               << audit.original_group_sizes[index] << ','
               << audit.effective_group_sizes[index] << ','
               << (audit.effective_group_sizes[index] == 0 ? "true" : "false") << '\n';
    }
}

void writeHardOverlapRepairCsv(const std::string &path, const HardOverlapSanitizerResult &result)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write hard-overlap repair report: " + path);
    }
    output << "policy,group_index,variable,detail\n";
    for (const auto &action : result.actions) {
        output << action.policy << ','
               << action.group_index << ','
               << action.variable << ','
               << csvText(action.detail) << '\n';
    }
}

}  // namespace grouping
}  // namespace flyki
