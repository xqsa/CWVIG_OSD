#ifndef FLYKI_PROBE_INTERACTION_GROUND_TRUTH_H_
#define FLYKI_PROBE_INTERACTION_GROUND_TRUTH_H_

#include <cstddef>
#include <set>
#include <utility>
#include <vector>

namespace flyki {
namespace probe {

using InteractionEdge = std::pair<std::size_t, std::size_t>;
using InteractionEdgeSet = std::set<InteractionEdge>;

InteractionEdge canonicalEdge(std::size_t i, std::size_t j);
InteractionEdgeSet trueInteractionEdgesFromGroups(
    const std::vector<std::vector<int>> &groups,
    std::size_t dimension_limit);

}  // namespace probe
}  // namespace flyki

#endif  // FLYKI_PROBE_INTERACTION_GROUND_TRUTH_H_
