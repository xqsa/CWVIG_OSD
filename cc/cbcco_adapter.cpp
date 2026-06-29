// Adapter boundary for integrating CWVIG-OSD grouping outputs with Flyki CBCCO.
//
// This file deliberately avoids depending on concrete Flyki headers until the
// exact optimizer entry point is selected. The exported C ABI can be used by a
// thin wrapper or replaced with native Flyki types later.

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace cwvig {

struct SoftGroupingView {
    const double* memberships;
    const double* shared_confidence;
    std::size_t variables;
    std::size_t groups;
};

void validate_grouping_view(const SoftGroupingView& view) {
    if (view.memberships == nullptr) {
        throw std::invalid_argument("memberships pointer is null");
    }
    if (view.shared_confidence == nullptr) {
        throw std::invalid_argument("shared_confidence pointer is null");
    }
    if (view.variables == 0 || view.groups == 0) {
        throw std::invalid_argument("variables and groups must be positive");
    }
}

std::vector<std::vector<int>> harden_soft_groups(
    const SoftGroupingView& view,
    double membership_threshold
) {
    validate_grouping_view(view);
    if (membership_threshold < 0.0 || membership_threshold > 1.0) {
        throw std::invalid_argument("membership_threshold must be in [0, 1]");
    }

    std::vector<std::vector<int>> groups(view.groups);
    for (std::size_t variable = 0; variable < view.variables; ++variable) {
        std::size_t winner = 0;
        double best = view.memberships[variable * view.groups];
        for (std::size_t group = 1; group < view.groups; ++group) {
            const double value = view.memberships[variable * view.groups + group];
            if (value > best) {
                best = value;
                winner = group;
            }
        }

        for (std::size_t group = 0; group < view.groups; ++group) {
            const double value = view.memberships[variable * view.groups + group];
            if (value >= membership_threshold || group == winner) {
                groups[group].push_back(static_cast<int>(variable));
            }
        }
    }
    return groups;
}

}  // namespace cwvig

extern "C" int cwvig_validate_grouping(
    const double* memberships,
    const double* shared_confidence,
    std::size_t variables,
    std::size_t groups
) {
    try {
        cwvig::validate_grouping_view({memberships, shared_confidence, variables, groups});
        return 0;
    } catch (...) {
        return 1;
    }
}
