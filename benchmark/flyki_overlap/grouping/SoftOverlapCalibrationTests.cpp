#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingIO.h"
#include "grouping/SoftOverlapCalibration.h"
#include "grouping/SoftOverlapDecomposition.h"
#include "grouping/WeightedInteractionGraph.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
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
    require(std::fabs(actual - expected) < 1e-12, message);
}

template <typename Fn>
void requireThrows(Fn fn, const std::string &message)
{
    try {
        fn();
    } catch (const std::exception &) {
        return;
    }
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
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

flyki::grouping::ThresholdGrid smallGrid()
{
    flyki::grouping::ThresholdGrid grid;
    grid.score_thresholds = {0.8, 1.0};
    grid.expand_thresholds = {0.75, 1.5};
    grid.z_thresholds = {0.8};
    grid.shared_ratio_thresholds = {0.7, 1.1};
    return grid;
}

}  // namespace

int main()
{
    using flyki::grouping::InteractionScoreColumn;
    using flyki::grouping::OracleTruth;
    using flyki::grouping::SoftOverlapConfig;
    using flyki::grouping::ThresholdGrid;
    using flyki::grouping::UnsupervisedMethod;
    using flyki::grouping::buildWeightedInteractionGraphFromCsv;
    using flyki::grouping::decomposeSoftOverlap;
    using flyki::grouping::evaluateSoftOverlapGrouping;
    using flyki::grouping::runOracleCalibration;
    using flyki::grouping::selectUnsupervisedThresholds;
    using flyki::grouping::writeBestSoftOverlapOutputs;

    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_soft_overlap_calibration_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    const auto path_edges = temp_root / "path_edges.csv";
    writeFile(path_edges, edgeCsv(
        "0,1,2,2,0,0.5,1,0,3\n"
        "0,2,0,0,0,0.5,0,0,3\n"
        "1,2,2,2,0,0.5,1,0,3\n"));
    const auto path_graph = buildWeightedInteractionGraphFromCsv(
        path_edges.string(),
        3,
        InteractionScoreColumn::MeanAbsNormalized,
        false);
    OracleTruth path_truth;
    path_truth.provided = true;
    path_truth.true_groups = {{0, 1}, {1, 2}};
    path_truth.true_overlap_groups = {{1}, {1}};
    path_truth.dimension = 3;

    const auto oracle = runOracleCalibration(path_graph, baseConfig(3), path_truth, smallGrid());
    require(!oracle.rows.empty(), "oracle rows are written");
    requireNear(oracle.best.shared_metrics.f1, 1.0, "path oracle SharedVar-F1");
    require(oracle.best.groups == 2, "path oracle group count");
    require(oracle.best.shared_variables == 1, "path oracle shared count");

    const auto best_po = temp_root / "best_groups.txt";
    const auto best_oo = temp_root / "best_overlap.txt";
    const auto best_z = temp_root / "best_z.csv";
    writeBestSoftOverlapOutputs(best_po.string(), best_oo.string(), best_z.string(), path_graph, baseConfig(3), oracle.best);
    const auto loaded = flyki::grouping::loadLegacyGroupingFromFiles(best_po, best_oo);
    require(loaded.number_of_groups == 2, "best po/oo readable by explicit loader");

    const auto clique_edges = temp_root / "clique_edges.csv";
    writeFile(clique_edges, edgeCsv(
        "0,1,2,2,0,0.5,1,0,3\n"
        "0,2,2,2,0,0.5,1,0,3\n"
        "1,2,2,2,0,0.5,1,0,3\n"));
    const auto clique_graph = buildWeightedInteractionGraphFromCsv(
        clique_edges.string(),
        3,
        InteractionScoreColumn::MeanAbsNormalized,
        false);
    OracleTruth clique_truth;
    clique_truth.provided = true;
    clique_truth.true_groups = {{0, 1, 2}};
    clique_truth.true_overlap_groups = {{}};
    clique_truth.dimension = 3;
    const auto clique_oracle = runOracleCalibration(clique_graph, baseConfig(3), clique_truth, smallGrid());
    require(clique_oracle.best.shared_variables == 0, "clique avoids all-shared when one group is correct");
    requireNear(clique_oracle.best.over_shared_ratio, 0.0, "clique over-shared ratio");

    const auto no_edges = temp_root / "no_edges.csv";
    writeFile(no_edges, edgeCsv(""));
    const auto no_graph = buildWeightedInteractionGraphFromCsv(
        no_edges.string(),
        3,
        InteractionScoreColumn::MeanAbsNormalized,
        false);
    auto unsup_config = baseConfig(3);
    const auto selected = selectUnsupervisedThresholds(
        no_graph,
        unsup_config,
        UnsupervisedMethod::QuantileScore,
        0.75,
        1.0,
        0.2);
    unsup_config.score_threshold = selected.score_threshold;
    unsup_config.expand_threshold = selected.expand_threshold;
    unsup_config.z_threshold = selected.z_threshold;
    unsup_config.shared_ratio_threshold = selected.shared_ratio_threshold;
    const auto no_result = decomposeSoftOverlap(no_graph, unsup_config);
    const auto no_eval = evaluateSoftOverlapGrouping(no_result, {}, {}, 3);
    require(no_eval.groups == 3, "no-edge unsupervised singleton groups");
    require(no_eval.shared_variables == 0, "no-edge unsupervised no shared vars");
    requireNear(no_eval.over_shared_ratio, 0.0, "no-edge over-shared ratio");

    const auto selected_again = selectUnsupervisedThresholds(
        no_graph,
        baseConfig(3),
        UnsupervisedMethod::QuantileScore,
        0.75,
        1.0,
        0.2);
    require(selected.score_threshold == selected_again.score_threshold, "unsupervised deterministic score threshold");
    require(selected.expand_threshold == selected_again.expand_threshold, "unsupervised deterministic expand threshold");

    flyki::grouping::SoftOverlapResult over_shared;
    over_shared.groups = {{0, 1}, {0, 1}, {2}, {2}};
    over_shared.shared_variables = {0, 1, 2};
    over_shared.overlap_groups = {{0, 1}, {0, 1}, {2}, {2}};
    over_shared.z = {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 1, 1}};
    const auto over_eval = evaluateSoftOverlapGrouping(over_shared, {{0, 1}, {2}}, {{0}}, 3);
    requireNear(over_eval.over_shared_ratio, 1.0, "over-shared ratio detects all variables shared");

    OracleTruth missing_truth;
    requireThrows(
        [&]() { runOracleCalibration(path_graph, baseConfig(3), missing_truth, smallGrid()); },
        "oracle requires true po/oo");

    std::filesystem::remove_all(temp_root);
    std::cout << "SoftOverlapCalibrationTests passed\n";
    return 0;
}
