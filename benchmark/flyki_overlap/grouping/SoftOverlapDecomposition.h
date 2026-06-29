#ifndef FLYKI_GROUPING_SOFT_OVERLAP_DECOMPOSITION_H_
#define FLYKI_GROUPING_SOFT_OVERLAP_DECOMPOSITION_H_

#include "grouping/WeightedInteractionGraph.h"

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace flyki {
namespace grouping {

struct SoftOverlapConfig {
    InteractionScoreColumn score_column = InteractionScoreColumn::MeanAbsNormalized;
    double score_threshold = 0.5;
    bool use_log1p_score = false;
    double expand_threshold = 0.5;
    double merge_jaccard_threshold = 0.8;
    double z_threshold = 0.5;
    double shared_ratio_threshold = 0.7;
    std::size_t min_group_size = 1;
    bool add_singletons_for_uncovered = true;
    std::size_t dimension = 0;
};

struct SoftOverlapResult {
    std::vector<std::vector<int>> groups;
    std::vector<std::vector<double>> z;
    std::set<int> shared_variables;
    std::vector<std::vector<int>> overlap_groups;
};

SoftOverlapResult decomposeSoftOverlap(
    const WeightedInteractionGraph &graph,
    const SoftOverlapConfig &config);

void writeSoftMembershipCsv(
    const std::string &path,
    const SoftOverlapResult &result);

void writeSoftOverlapOutputs(
    const std::string &po_path,
    const std::string &oo_path,
    const std::string &z_path,
    const SoftOverlapResult &result);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_SOFT_OVERLAP_DECOMPOSITION_H_
