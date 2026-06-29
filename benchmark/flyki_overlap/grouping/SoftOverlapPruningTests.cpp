#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/SoftOverlapDecomposition.h"
#include "grouping/WeightedInteractionGraph.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void requireNear(const double actual, const double expected, const std::string &message)
{
    require(std::fabs(actual - expected) < 1e-9, message);
}

void writeFile(const std::filesystem::path &path, const std::string &content)
{
    std::ofstream output(path);
    output << content;
}

std::string edgeCsv(const std::string &rows)
{
    return "i,j,mean_abs_raw,mean_abs_normalized,std_normalized,threshold,probability,uncertainty,contexts\n" + rows;
}

flyki::grouping::SoftOverlapConfig strictConfig(const std::size_t dimension)
{
    flyki::grouping::SoftOverlapConfig config;
    config.dimension = dimension;
    config.score_threshold = 0.7;
    config.expand_threshold = 0.7;
    config.merge_jaccard_threshold = 0.8;
    config.z_threshold = 0.8;
    config.shared_ratio_threshold = 0.7;
    config.membership_transform = flyki::grouping::MembershipTransform::CurrentRatio;
    config.shared_rule = flyki::grouping::SharedVariableRule::HardPlusSecondMembership;
    config.shared_min_second_membership = 0.8;
    config.max_shared_ratio = 1.0;
    return config;
}

std::set<std::set<int>> groupSet(const std::vector<std::vector<int>> &groups)
{
    std::set<std::set<int>> result;
    for (const auto &group : groups) {
        result.insert(std::set<int>(group.begin(), group.end()));
    }
    return result;
}

}  // namespace

int main()
{
    using flyki::grouping::EdgeWeightMode;
    using flyki::grouping::InteractionScoreColumn;
    using flyki::grouping::MembershipTransform;
    using flyki::grouping::SharedVariableRule;
    using flyki::grouping::SparsificationMode;
    using flyki::grouping::SparsificationConfig;
    using flyki::grouping::buildWeightedInteractionGraphFromCsv;
    using flyki::grouping::decomposeSoftOverlap;
    using flyki::grouping::writeSoftOverlapOutputs;

    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_soft_overlap_pruning_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    const auto path_edges = temp_root / "path_edges.csv";
    writeFile(path_edges, edgeCsv(
        "0,1,0.9,0.9,0,0.5,0.9,0,3\n"
        "0,2,0.1,0.1,0,0.5,0.1,0,3\n"
        "1,2,0.9,0.9,0,0.5,0.9,0,3\n"));
    const auto path_graph = buildWeightedInteractionGraphFromCsv(
        path_edges.string(), 3, InteractionScoreColumn::MeanAbsNormalized, EdgeWeightMode::Raw, {});
    const auto path_result = decomposeSoftOverlap(path_graph, strictConfig(3));
    require(path_result.shared_variables == std::set<int>({1}), "path overlap keeps variable 1 shared");
    require(groupSet(path_result.groups) == std::set<std::set<int>>({{0, 1}, {1, 2}}), "path overlap groups survive pruning");

    const auto clique_edges = temp_root / "clique_edges.csv";
    writeFile(clique_edges, edgeCsv(
        "0,1,0.9,0.9,0,0.5,0.9,0,3\n"
        "0,2,0.9,0.9,0,0.5,0.9,0,3\n"
        "1,2,0.9,0.9,0,0.5,0.9,0,3\n"));
    auto clique_config = strictConfig(3);
    clique_config.shared_rule = SharedVariableRule::HardMembershipOnly;
    const auto clique_result = decomposeSoftOverlap(
        buildWeightedInteractionGraphFromCsv(
            clique_edges.string(), 3, InteractionScoreColumn::MeanAbsNormalized, EdgeWeightMode::Raw, {}),
        clique_config);
    require(groupSet(clique_result.groups) == std::set<std::set<int>>({{0, 1, 2}}), "clique remains one group");
    require(clique_result.shared_variables.empty(), "strict clique has no shared variables");

    const auto uncertainty_edges = temp_root / "uncertainty_edges.csv";
    writeFile(uncertainty_edges, edgeCsv(
        "0,1,1,10,0,0.5,0.5,0.6931471805599453,3\n"
        "0,2,1,5,0,0.5,1,0,3\n"));
    const auto uncertain_graph = buildWeightedInteractionGraphFromCsv(
        uncertainty_edges.string(), 3, InteractionScoreColumn::MeanAbsNormalized, EdgeWeightMode::UncertaintyPenalized, {});
    require(uncertain_graph.score(0, 1) < 0.01, "high entropy edge is downweighted");
    requireNear(uncertain_graph.score(0, 2), 0.0, "lower raw score receives rank 0 when certainty is high");

    SparsificationConfig mutual;
    mutual.mode = SparsificationMode::MutualTopK;
    mutual.top_k_per_node = 1;
    const auto mutual_graph = buildWeightedInteractionGraphFromCsv(
        path_edges.string(), 3, InteractionScoreColumn::MeanAbsNormalized, EdgeWeightMode::Raw, mutual);
    require(mutual_graph.edges().size() == 1, "mutual top-k keeps deterministic one edge on tie");
    require(mutual_graph.score(0, 1) > 0.0, "mutual top-k tie keeps lexicographic edge");

    const auto dense_edges = temp_root / "dense_edges.csv";
    writeFile(dense_edges, edgeCsv(
        "0,1,1,1,0,0.5,1,0,3\n"
        "0,2,1,0.9,0,0.5,0.9,0,3\n"
        "0,3,1,0.8,0,0.5,0.8,0,3\n"
        "1,2,1,0.7,0,0.5,0.7,0,3\n"
        "1,3,1,0.6,0,0.5,0.6,0,3\n"
        "2,3,1,0.5,0,0.5,0.5,0,3\n"));
    auto capped_config = strictConfig(4);
    capped_config.score_threshold = 0.5;
    capped_config.expand_threshold = 0.5;
    capped_config.z_threshold = 0.8;
    capped_config.shared_rule = SharedVariableRule::CappedShared;
    capped_config.max_shared_ratio = 0.5;
    const auto dense_result = decomposeSoftOverlap(
        buildWeightedInteractionGraphFromCsv(
            dense_edges.string(), 4, InteractionScoreColumn::MeanAbsNormalized, EdgeWeightMode::Raw, {}),
        capped_config);
    require(dense_result.shared_variables.size() <= 2, "shared cap prevents all variables shared");

    const auto po = temp_root / "pruned_groups.txt";
    const auto oo = temp_root / "pruned_overlap.txt";
    const auto z = temp_root / "pruned_z.csv";
    writeSoftOverlapOutputs(po.string(), oo.string(), z.string(), path_result);
    const auto loaded = flyki::grouping::loadLegacyGroupingFromFiles(po, oo);
    require(loaded.number_of_groups == 2, "pruned po/oo remains readable");

    std::filesystem::remove_all(temp_root);
    std::cout << "SoftOverlapPruningTests passed\n";
    return 0;
}
