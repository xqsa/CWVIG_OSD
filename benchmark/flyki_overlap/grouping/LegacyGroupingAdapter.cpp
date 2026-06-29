#include "grouping/LegacyGroupingAdapter.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

void requireNonNegative(const int variable)
{
    if (variable < 0) {
        throw std::invalid_argument("Grouping variable ids must be non-negative.");
    }
}

std::vector<std::vector<int>> buildLegacyOveriables(const std::vector<std::vector<int>> &overlap_groups)
{
    std::set<int> seen;
    std::vector<std::vector<int>> result;
    for (const auto &group : overlap_groups) {
        std::vector<int> unique_for_group;
        for (const int variable : group) {
            requireNonNegative(variable);
            if (seen.insert(variable).second) {
                unique_for_group.push_back(variable);
            }
        }
        if (!unique_for_group.empty()) {
            result.push_back(unique_for_group);
        }
    }
    return result;
}

std::map<int, std::vector<std::pair<int, int>>> buildSharedPositions(
    const std::vector<std::vector<int>> &groups,
    const std::vector<std::vector<int>> &overlap_groups)
{
    std::map<int, std::vector<std::pair<int, int>>> positions;
    const std::size_t count = std::min(groups.size(), overlap_groups.size());
    for (std::size_t group_index = 0; group_index < count; ++group_index) {
        for (std::size_t position = 0; position < groups[group_index].size(); ++position) {
            requireNonNegative(groups[group_index][position]);
            for (const int overlap_variable : overlap_groups[group_index]) {
                requireNonNegative(overlap_variable);
                if (groups[group_index][position] == overlap_variable) {
                    positions[overlap_variable].push_back(
                        {static_cast<int>(group_index), static_cast<int>(position)});
                }
            }
        }
    }
    return positions;
}

void appendNegativeErrors(
    const std::vector<std::vector<int>> &groups,
    const std::string &label,
    std::vector<std::string> &errors)
{
    for (std::size_t group_index = 0; group_index < groups.size(); ++group_index) {
        for (std::size_t position = 0; position < groups[group_index].size(); ++position) {
            if (groups[group_index][position] < 0) {
                errors.push_back(label + " contains a negative variable id.");
            }
        }
    }
}

int maxVariableId(const LegacyGroupingView &view)
{
    int max_variable = -1;
    const auto scan = [&max_variable](const std::vector<std::vector<int>> &groups) {
        for (const auto &group : groups) {
            for (const int variable : group) {
                max_variable = std::max(max_variable, variable);
            }
        }
    };
    scan(view.groups);
    scan(view.overiablesRedandunt);
    return max_variable;
}

}  // namespace

LegacyGroupingView makeLegacyGroupingView(const GroupingData &data)
{
    LegacyGroupingView view;
    view.groups = data.groups;
    view.overlap_groups = data.overlap_groups;
    view.overiablesRedandunt = data.overlap_groups;
    view.overiables = buildLegacyOveriables(data.overlap_groups);
    view.sharedvar_group_pos = buildSharedPositions(data.groups, data.overlap_groups);
    view.dimension = data.dimension;
    view.number_of_groups = data.groups.size();
    return view;
}

std::vector<std::string> validateLegacyGroupingView(
    const LegacyGroupingView &view,
    const bool dimension_is_explicit)
{
    std::vector<std::string> errors;
    if (view.number_of_groups != view.groups.size()) {
        errors.push_back("number_of_groups does not match groups.size().");
    }
    if (view.groups.size() != view.overiablesRedandunt.size()) {
        errors.push_back("groups and overiablesRedandunt must have the same group count.");
    }
    if (view.overlap_groups != view.overiablesRedandunt) {
        errors.push_back("overlap_groups and overiablesRedandunt differ.");
    }
    if (view.overiables != buildLegacyOveriables(view.overiablesRedandunt)) {
        errors.push_back("overiables does not match legacy first-seen overlap construction.");
    }

    appendNegativeErrors(view.groups, "groups", errors);
    appendNegativeErrors(view.overiablesRedandunt, "overiablesRedandunt", errors);

    const std::size_t count = std::min(view.groups.size(), view.overiablesRedandunt.size());
    for (std::size_t group_index = 0; group_index < count; ++group_index) {
        for (const int overlap_variable : view.overiablesRedandunt[group_index]) {
            const auto &group = view.groups[group_index];
            if (std::find(group.begin(), group.end(), overlap_variable) == group.end()) {
                errors.push_back("overlap variable is not present in its primary group.");
            }
        }
    }

    for (const auto &entry : view.sharedvar_group_pos) {
        if (entry.first < 0) {
            errors.push_back("sharedvar_group_pos contains a negative variable id.");
        }
        for (const auto &position : entry.second) {
            if (position.first < 0 || position.second < 0) {
                errors.push_back("sharedvar_group_pos contains a negative position.");
                continue;
            }
            const auto group_index = static_cast<std::size_t>(position.first);
            const auto variable_position = static_cast<std::size_t>(position.second);
            if (group_index >= view.groups.size() || variable_position >= view.groups[group_index].size()) {
                errors.push_back("sharedvar_group_pos position is out of range.");
                continue;
            }
            if (view.groups[group_index][variable_position] != entry.first) {
                errors.push_back("sharedvar_group_pos position points to a different variable.");
            }
        }
    }

    const int max_variable = maxVariableId(view);
    if (max_variable >= 0) {
        if (dimension_is_explicit) {
            if (view.dimension <= max_variable) {
                errors.push_back("explicit dimension is smaller than max variable id + 1.");
            }
        } else if (view.dimension != max_variable + 1) {
            errors.push_back("dimension does not equal max variable id + 1.");
        }
    } else if (!dimension_is_explicit && view.dimension != 0) {
        errors.push_back("empty grouping dimension should be 0.");
    }

    return errors;
}

std::string formatSharedMap(const LegacyGroupingView &view)
{
    std::ostringstream output;
    for (const auto &entry : view.sharedvar_group_pos) {
        output << entry.first << ':';
        for (const auto &position : entry.second) {
            output << ' ' << position.first << ',' << position.second;
        }
        output << '\n';
    }
    return output.str();
}

}  // namespace grouping
}  // namespace flyki
