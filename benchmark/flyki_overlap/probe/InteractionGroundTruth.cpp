#include "probe/InteractionGroundTruth.h"

#include <stdexcept>

namespace flyki {
namespace probe {

InteractionEdge canonicalEdge(const std::size_t i, const std::size_t j)
{
    if (i == j) {
        throw std::invalid_argument("Interaction edge endpoints must differ.");
    }
    return i < j ? InteractionEdge{i, j} : InteractionEdge{j, i};
}

InteractionEdgeSet trueInteractionEdgesFromGroups(
    const std::vector<std::vector<int>> &groups,
    const std::size_t dimension_limit)
{
    InteractionEdgeSet positives;
    for (const auto &group : groups) {
        std::set<std::size_t> variables;
        for (const int variable : group) {
            if (variable >= 0 && static_cast<std::size_t>(variable) < dimension_limit) {
                variables.insert(static_cast<std::size_t>(variable));
            }
        }

        for (auto first = variables.begin(); first != variables.end(); ++first) {
            auto second = first;
            ++second;
            for (; second != variables.end(); ++second) {
                positives.insert({*first, *second});
            }
        }
    }
    return positives;
}

}  // namespace probe
}  // namespace flyki
