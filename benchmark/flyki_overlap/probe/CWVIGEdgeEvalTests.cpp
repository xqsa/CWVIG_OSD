#include "probe/CWVIGEdgeData.h"
#include "probe/CWVIGEdgeMetrics.h"
#include "probe/InteractionGroundTruth.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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

}  // namespace

int main()
{
    using flyki::probe::averageScoresByLabel;
    using flyki::probe::bestF1ThresholdByProbability;
    using flyki::probe::bestF1ThresholdByScore;
    using flyki::probe::calculateProbabilityMetrics;
    using flyki::probe::calculateScoreMetrics;
    using flyki::probe::readCWVIGEdgesCsv;
    using flyki::probe::rankingAccuracyByScore;
    using flyki::probe::summarizeScores;
    using flyki::probe::topKPrecisionByScore;
    using flyki::probe::trueInteractionEdgesFromGroups;

    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_edge_eval_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    const auto interaction_csv = temp_root / "interaction_edges.csv";
    writeFile(interaction_csv,
        "i,j,mean_abs_raw,mean_abs_normalized,std_normalized,threshold,probability,uncertainty,contexts\n"
        "0,1,1e-08,0.9999,0,0.5,1,0,3\n"
        "0,2,0,0,0,0.5,0,0,3\n"
        "0,3,0,0,0,0.5,0,0,3\n"
        "1,2,0,0,0,0.5,0,0,3\n"
        "1,3,0,0,0,0.5,0,0,3\n"
        "2,3,0,0,0,0.5,0,0,3\n");

    const auto interaction_edges = readCWVIGEdgesCsv(interaction_csv.string());
    const auto interaction_truth = trueInteractionEdgesFromGroups({{0, 1}, {2}, {3}}, 4);
    const auto interaction_metrics = calculateScoreMetrics(interaction_edges, interaction_truth, 0.5);
    require(interaction_edges.size() == 6, "interaction edge count");
    requireNear(interaction_metrics.f1, 1.0, "one-interaction score F1");
    requireNear(calculateProbabilityMetrics(interaction_edges, interaction_truth, 0.5).f1, 1.0, "one-interaction probability F1");

    const auto overlap_csv = temp_root / "overlap_edges.csv";
    writeFile(overlap_csv,
        "i,j,mean_abs_raw,mean_abs_normalized,std_normalized,threshold,probability,uncertainty,contexts\n"
        "0,1,2e-08,1.9998,0,0.5,1,0,3\n"
        "0,2,0,0,0,0.5,0,0,3\n"
        "1,2,2e-08,1.9998,0,0.5,1,0,3\n");

    const auto overlap_edges = readCWVIGEdgesCsv(overlap_csv.string());
    const auto overlap_truth = trueInteractionEdgesFromGroups({{0, 1}, {1, 2}}, 3);
    require(overlap_truth.count({0, 1}) == 1, "overlap truth has 0,1");
    require(overlap_truth.count({1, 2}) == 1, "overlap truth has 1,2");
    require(overlap_truth.count({0, 2}) == 0, "overlap truth rejects 0,2");

    const auto overlap_metrics = calculateScoreMetrics(overlap_edges, overlap_truth, 0.5);
    require(overlap_metrics.true_positive == 2, "overlap true positives");
    require(overlap_metrics.true_negative == 1, "overlap true negative");
    requireNear(overlap_metrics.f1, 1.0, "overlap score F1");
    requireNear(topKPrecisionByScore(overlap_edges, overlap_truth, 2), 1.0, "overlap top-k precision");
    requireNear(rankingAccuracyByScore(overlap_edges, overlap_truth), 1.0, "overlap ranking accuracy");
    requireNear(bestF1ThresholdByScore(overlap_edges, overlap_truth).f1, 1.0, "overlap best score F1");
    requireNear(bestF1ThresholdByProbability(overlap_edges, overlap_truth).f1, 1.0, "overlap best probability F1");

    const auto averages = averageScoresByLabel(overlap_edges, overlap_truth);
    require(averages.positive_mean > averages.negative_mean, "positive average score above negative");

    const auto summary = summarizeScores(overlap_edges);
    requireNear(summary.min, 0.0, "score min");
    requireNear(summary.max, 1.9998, "score max");
    requireNear(summary.median, 1.9998, "score median");

    std::filesystem::remove_all(temp_root);
    std::cout << "CWVIGEdgeEvalTests passed\n";
    return 0;
}
