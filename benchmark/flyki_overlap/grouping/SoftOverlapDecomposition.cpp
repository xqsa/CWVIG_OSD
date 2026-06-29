#include "grouping/SoftOverlapDecomposition.h"

#include "grouping/GroupingIO.h"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

double clamp01(const double value)
{
    return std::max(0.0, std::min(1.0, value));
}

double affinityMembership(const double affinity)
{
    if (affinity <= 0.0) {
        return 0.0;
    }
    return clamp01(affinity / (1.0 + affinity));
}

double averageAffinity(
    const WeightedInteractionGraph &graph,
    const std::size_t variable,
    const std::set<std::size_t> &community)
{
    if (community.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const auto member : community) {
        sum += graph.score(variable, member);
    }
    return sum / static_cast<double>(community.size());
}

std::set<std::size_t> expandCommunity(
    const WeightedInteractionGraph &graph,
    std::set<std::size_t> community,
    const double expand_threshold)
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t variable = 0; variable < graph.dimension(); ++variable) {
            if (community.count(variable) != 0) {
                continue;
            }
            if (averageAffinity(graph, variable, community) >= expand_threshold) {
                community.insert(variable);
                changed = true;
            }
        }
    }
    return community;
}

double jaccard(const std::set<std::size_t> &left, const std::set<std::size_t> &right)
{
    if (left.empty() && right.empty()) {
        return 1.0;
    }
    std::vector<std::size_t> intersection;
    std::set_intersection(
        left.begin(),
        left.end(),
        right.begin(),
        right.end(),
        std::back_inserter(intersection));
    const std::size_t union_size = left.size() + right.size() - intersection.size();
    return union_size == 0 ? 0.0 : static_cast<double>(intersection.size()) / static_cast<double>(union_size);
}

std::vector<std::set<std::size_t>> sortCommunities(std::vector<std::set<std::size_t>> communities)
{
    std::sort(communities.begin(), communities.end(), [](const auto &left, const auto &right) {
        return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
    });
    return communities;
}

std::vector<std::set<std::size_t>> mergeCommunities(
    std::vector<std::set<std::size_t>> communities,
    const double merge_jaccard_threshold)
{
    communities = sortCommunities(std::move(communities));
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = 0; i < communities.size() && !changed; ++i) {
            for (std::size_t j = i + 1; j < communities.size(); ++j) {
                if (jaccard(communities[i], communities[j]) >= merge_jaccard_threshold) {
                    communities[i].insert(communities[j].begin(), communities[j].end());
                    communities.erase(communities.begin() + static_cast<std::ptrdiff_t>(j));
                    communities = sortCommunities(std::move(communities));
                    changed = true;
                    break;
                }
            }
        }
    }
    return communities;
}

void addSingletonsForUncovered(
    const std::size_t dimension,
    std::vector<std::set<std::size_t>> &communities)
{
    std::set<std::size_t> covered;
    for (const auto &community : communities) {
        covered.insert(community.begin(), community.end());
    }
    for (std::size_t variable = 0; variable < dimension; ++variable) {
        if (covered.count(variable) == 0) {
            communities.push_back({variable});
        }
    }
    communities = sortCommunities(std::move(communities));
}

std::vector<std::vector<double>> buildZ(
    const WeightedInteractionGraph &graph,
    const std::vector<std::set<std::size_t>> &communities)
{
    std::vector<std::vector<double>> z(graph.dimension(), std::vector<double>(communities.size(), 0.0));
    for (std::size_t group_index = 0; group_index < communities.size(); ++group_index) {
        const auto &community = communities[group_index];
        for (std::size_t variable = 0; variable < graph.dimension(); ++variable) {
            z[variable][group_index] = community.count(variable) != 0
                ? 1.0
                : affinityMembership(averageAffinity(graph, variable, community));
        }
    }
    return z;
}

std::vector<std::vector<int>> groupsFromZ(
    const std::vector<std::vector<double>> &z,
    const double z_threshold,
    const std::size_t min_group_size)
{
    const std::size_t dimension = z.size();
    const std::size_t groups = dimension == 0 ? 0 : z.front().size();
    std::vector<std::vector<int>> result;
    for (std::size_t group_index = 0; group_index < groups; ++group_index) {
        std::vector<int> group;
        for (std::size_t variable = 0; variable < dimension; ++variable) {
            if (z[variable][group_index] >= z_threshold) {
                group.push_back(static_cast<int>(variable));
            }
        }
        if (group.size() >= min_group_size) {
            result.push_back(group);
        }
    }
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

std::set<int> sharedVariablesFromGroupsAndZ(
    const std::vector<std::vector<int>> &groups,
    const std::vector<std::vector<double>> &z,
    const double shared_ratio_threshold)
{
    std::vector<std::size_t> counts(z.size(), 0);
    for (const auto &group : groups) {
        for (const int variable : group) {
            if (variable >= 0 && static_cast<std::size_t>(variable) < counts.size()) {
                ++counts[static_cast<std::size_t>(variable)];
            }
        }
    }

    std::set<int> shared;
    for (std::size_t variable = 0; variable < z.size(); ++variable) {
        if (counts[variable] > 1) {
            shared.insert(static_cast<int>(variable));
            continue;
        }
        auto memberships = z[variable];
        std::sort(memberships.begin(), memberships.end(), std::greater<double>());
        if (memberships.size() >= 2 && memberships[0] > 0.0
            && memberships[1] / memberships[0] >= shared_ratio_threshold) {
            shared.insert(static_cast<int>(variable));
        }
    }
    return shared;
}

std::vector<std::vector<int>> overlapGroupsFromShared(
    const std::vector<std::vector<int>> &groups,
    const std::set<int> &shared_variables)
{
    std::vector<std::vector<int>> overlap_groups;
    overlap_groups.reserve(groups.size());
    for (const auto &group : groups) {
        std::vector<int> overlap_group;
        for (const int variable : group) {
            if (shared_variables.count(variable) != 0) {
                overlap_group.push_back(variable);
            }
        }
        overlap_groups.push_back(overlap_group);
    }
    return overlap_groups;
}

void validateConfig(const WeightedInteractionGraph &graph, const SoftOverlapConfig &config)
{
    if (config.dimension != 0 && config.dimension != graph.dimension()) {
        throw std::invalid_argument("Soft overlap config dimension does not match graph dimension.");
    }
    if (config.min_group_size == 0) {
        throw std::invalid_argument("min_group_size must be positive.");
    }
    if (config.merge_jaccard_threshold < 0.0 || config.merge_jaccard_threshold > 1.0) {
        throw std::invalid_argument("merge_jaccard_threshold must be in [0,1].");
    }
    if (config.z_threshold < 0.0 || config.z_threshold > 1.0) {
        throw std::invalid_argument("z_threshold must be in [0,1].");
    }
}

}  // namespace

SoftOverlapResult decomposeSoftOverlap(
    const WeightedInteractionGraph &graph,
    const SoftOverlapConfig &config)
{
    validateConfig(graph, config);

    std::vector<std::set<std::size_t>> communities;
    for (const auto &edge : graph.strongEdges(config.score_threshold)) {
        communities.push_back(expandCommunity(graph, {edge.i, edge.j}, config.expand_threshold));
    }

    communities = mergeCommunities(std::move(communities), config.merge_jaccard_threshold);
    if (config.add_singletons_for_uncovered) {
        addSingletonsForUncovered(graph.dimension(), communities);
    }

    SoftOverlapResult result;
    result.z = buildZ(graph, communities);
    result.groups = groupsFromZ(result.z, config.z_threshold, config.min_group_size);
    result.shared_variables = sharedVariablesFromGroupsAndZ(
        result.groups,
        result.z,
        config.shared_ratio_threshold);
    result.overlap_groups = overlapGroupsFromShared(result.groups, result.shared_variables);
    return result;
}

void writeSoftMembershipCsv(
    const std::string &path,
    const SoftOverlapResult &result)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write Z CSV: " + path);
    }
    output << "variable,group,membership\n";
    for (std::size_t variable = 0; variable < result.z.size(); ++variable) {
        for (std::size_t group = 0; group < result.z[variable].size(); ++group) {
            output << variable << ',' << group << ',' << result.z[variable][group] << '\n';
        }
    }
}

void writeSoftOverlapOutputs(
    const std::string &po_path,
    const std::string &oo_path,
    const std::string &z_path,
    const SoftOverlapResult &result)
{
    writePoFile(po_path, result.groups);
    writeOoFile(oo_path, result.overlap_groups);
    writeSoftMembershipCsv(z_path, result);
}

}  // namespace grouping
}  // namespace flyki
