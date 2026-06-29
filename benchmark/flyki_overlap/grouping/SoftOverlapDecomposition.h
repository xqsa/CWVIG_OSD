#ifndef FLYKI_GROUPING_SOFT_OVERLAP_DECOMPOSITION_H_
#define FLYKI_GROUPING_SOFT_OVERLAP_DECOMPOSITION_H_

#include "grouping/WeightedInteractionGraph.h"

#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace flyki {
namespace grouping {

enum class MembershipTransform {
    CurrentRatio,
    RankNormalized,
    MinmaxNormalized,
    SigmoidCentered,
};

enum class SharedVariableRule {
    HardPlusRatio,
    HardMembershipOnly,
    HardPlusSecondMembership,
    CappedShared,
};

struct SharedVariableDiagnostics {
    double confidence_min = 0.0;
    double confidence_mean = 0.0;
    double confidence_max = 0.0;
    std::size_t pruned_by_cap = 0;
    double average_top1_membership = 0.0;
    double average_top2_membership = 0.0;
    double average_top2_top1_ratio = 0.0;
    std::vector<std::pair<int, double>> top_shared_candidates;
};

struct SoftOverlapConfig {
    InteractionScoreColumn score_column = InteractionScoreColumn::MeanAbsNormalized;
    EdgeWeightMode edge_weight_mode = EdgeWeightMode::Raw;
    SparsificationMode sparsification_mode = SparsificationMode::ScoreThreshold;
    double score_threshold = 0.5;
    bool use_log1p_score = false;
    double expand_threshold = 0.5;
    double merge_jaccard_threshold = 0.8;
    double z_threshold = 0.5;
    double shared_ratio_threshold = 0.7;
    std::size_t top_k_per_node = 0;
    bool mutual_top_k = false;
    double target_avg_degree = 0.0;
    double max_avg_degree = 0.0;
    MembershipTransform membership_transform = MembershipTransform::CurrentRatio;
    bool suppress_weak_memberships = false;
    double membership_prune_threshold = 0.0;
    SharedVariableRule shared_rule = SharedVariableRule::HardPlusRatio;
    bool require_multi_group_hard_membership_for_shared = false;
    double max_shared_ratio = 1.0;
    double shared_min_second_membership = 0.0;
    double shared_min_membership_gap = 0.0;
    std::size_t min_group_size = 1;
    bool add_singletons_for_uncovered = true;
    std::size_t dimension = 0;
};

struct SoftOverlapResult {
    std::vector<std::vector<int>> groups;
    std::vector<std::vector<double>> z;
    std::set<int> shared_variables;
    std::vector<std::vector<int>> overlap_groups;
    SharedVariableDiagnostics shared_diagnostics;
};

MembershipTransform parseMembershipTransform(const std::string &name);
std::string membershipTransformName(MembershipTransform transform);
SharedVariableRule parseSharedVariableRule(const std::string &name);
std::string sharedVariableRuleName(SharedVariableRule rule);

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
