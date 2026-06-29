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
#include <vector>

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
    require(std::fabs(actual - expected) < 1e-12, message);
}

void writeFile(const std::filesystem::path &path, const std::string &content)
{
    std::ofstream output(path);
    output << content;
}

std::set<std::set<int>> groupSet(const std::vector<std::vector<int>> &groups)
{
    std::set<std::set<int>> result;
    for (const auto &group : groups) {
        result.insert(std::set<int>(group.begin(), group.end()));
    }
    return result;
}

std::string edgeCsv(const std::string &rows)
{
    return "i,j,mean_abs_raw,mean_abs_normalized,std_normalized,threshold,probability,uncertainty,contexts\n" + rows;
}

flyki::grouping::SoftOverlapConfig baseConfig(const std::size_t dimension)
{
    flyki::grouping::SoftOverlapConfig config;
    config.dimension = dimension;
    config.score_column = flyki::grouping::InteractionScoreColumn::MeanAbsNormalized;
    config.score_threshold = 0.8;
    config.expand_threshold = 0.75;
    config.merge_jaccard_threshold = 0.8;
    config.z_threshold = 0.8;
    config.shared_ratio_threshold = 0.7;
    config.min_group_size = 1;
    config.add_singletons_for_uncovered = true;
    return config;
}

void requireMembershipRange(const flyki::grouping::SoftOverlapResult &result)
{
    for (const auto &row : result.z) {
        for (const double value : row) {
            require(value >= 0.0 && value <= 1.0, "Z membership must be in [0,1]");
        }
    }
}

}  // namespace

int main()
{
    using flyki::grouping::InteractionScoreColumn;
    using flyki::grouping::buildWeightedInteractionGraphFromCsv;
    using flyki::grouping::decomposeSoftOverlap;
    using flyki::grouping::writeSoftOverlapOutputs;

    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_soft_overlap_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    const auto path_edges = temp_root / "path_edges.csv";
    writeFile(path_edges, edgeCsv(
        "0,1,0.9,0.9,0,0.5,0.9,0,3\n"
        "0,2,0.1,0.1,0,0.5,0.1,0,3\n"
        "1,2,0.9,0.9,0,0.5,0.9,0,3\n"));

    const auto path_graph = buildWeightedInteractionGraphFromCsv(
        path_edges.string(),
        3,
        InteractionScoreColumn::MeanAbsNormalized,
        false);
    requireNear(path_graph.score(0, 1), 0.9, "path graph score lookup");
    requireNear(path_graph.weightedDegree(1), 1.8, "path graph weighted degree");

    const auto path_result = decomposeSoftOverlap(path_graph, baseConfig(3));
    require(groupSet(path_result.groups) == std::set<std::set<int>>({{0, 1}, {1, 2}}), "path overlap groups");
    require(path_result.shared_variables == std::set<int>({1}), "path shared variable");
    require(path_result.z.size() == 3, "path Z row count");
    require(path_result.z[0].size() == path_result.groups.size(), "path Z column count");
    requireMembershipRange(path_result);

    const auto path_again = decomposeSoftOverlap(path_graph, baseConfig(3));
    require(path_again.groups == path_result.groups, "path output deterministic groups");
    require(path_again.overlap_groups == path_result.overlap_groups, "path output deterministic overlap");
    require(path_again.z == path_result.z, "path output deterministic Z");

    const auto path_po = temp_root / "path_predicted_groups.txt";
    const auto path_oo = temp_root / "path_predicted_overlap.txt";
    const auto path_z = temp_root / "path_z.csv";
    writeSoftOverlapOutputs(path_po.string(), path_oo.string(), path_z.string(), path_result);
    const auto loaded_path = flyki::grouping::loadLegacyGroupingFromFiles(path_po, path_oo);
    require(loaded_path.number_of_groups == path_result.groups.size(), "path output readable by explicit loader");

    const auto clique_edges = temp_root / "clique_edges.csv";
    writeFile(clique_edges, edgeCsv(
        "0,1,0.9,0.9,0,0.5,0.9,0,3\n"
        "0,2,0.9,0.9,0,0.5,0.9,0,3\n"
        "1,2,0.9,0.9,0,0.5,0.9,0,3\n"));
    const auto clique = decomposeSoftOverlap(
        buildWeightedInteractionGraphFromCsv(clique_edges.string(), 3, InteractionScoreColumn::MeanAbsNormalized, false),
        baseConfig(3));
    require(groupSet(clique.groups) == std::set<std::set<int>>({{0, 1, 2}}), "clique merges duplicate edge groups");

    const auto two_cliques_edges = temp_root / "two_cliques_edges.csv";
    writeFile(two_cliques_edges, edgeCsv(
        "0,1,0.9,0.9,0,0.5,0.9,0,3\n"
        "2,3,0.9,0.9,0,0.5,0.9,0,3\n"));
    const auto two_cliques = decomposeSoftOverlap(
        buildWeightedInteractionGraphFromCsv(two_cliques_edges.string(), 4, InteractionScoreColumn::MeanAbsNormalized, false),
        baseConfig(4));
    require(groupSet(two_cliques.groups) == std::set<std::set<int>>({{0, 1}, {2, 3}}), "two disconnected cliques");

    const auto interaction_edges = temp_root / "interaction_edges.csv";
    writeFile(interaction_edges, edgeCsv("0,1,3,3,0,0.5,0.9,0,3\n"));
    const auto log_graph = buildWeightedInteractionGraphFromCsv(
        interaction_edges.string(),
        4,
        InteractionScoreColumn::MeanAbsRaw,
        true);
    requireNear(log_graph.score(0, 1), std::log1p(3.0), "log1p raw score");
    const auto one_interaction = decomposeSoftOverlap(log_graph, baseConfig(4));
    require(groupSet(one_interaction.groups) == std::set<std::set<int>>({{0, 1}, {2}, {3}}), "one interaction plus singletons");

    const auto no_edges = temp_root / "no_edges.csv";
    writeFile(no_edges, edgeCsv(""));
    const auto no_strong = decomposeSoftOverlap(
        buildWeightedInteractionGraphFromCsv(no_edges.string(), 3, InteractionScoreColumn::MeanAbsNormalized, false),
        baseConfig(3));
    require(groupSet(no_strong.groups) == std::set<std::set<int>>({{0}, {1}, {2}}), "no strong edges produce singletons");

    std::filesystem::remove_all(temp_root);
    std::cout << "SoftOverlapDecompositionTests passed\n";
    return 0;
}
