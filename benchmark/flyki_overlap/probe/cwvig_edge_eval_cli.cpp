#include "grouping/GroupingIO.h"
#include "probe/CWVIGEdgeData.h"
#include "probe/CWVIGEdgeMetrics.h"
#include "probe/InteractionGroundTruth.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

std::map<std::string, std::string> parseArgs(const int argc, char **argv)
{
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h" || key == "--print-summary") {
            args[key] = "";
            continue;
        }
        if (key.rfind("--", 0) != 0 || i + 1 >= argc) {
            throw std::runtime_error("Invalid argument: " + key);
        }
        args[key] = argv[++i];
    }
    return args;
}

std::string requireArg(const std::map<std::string, std::string> &args, const std::string &key)
{
    const auto iter = args.find(key);
    if (iter == args.end()) {
        throw std::runtime_error("Missing required argument: " + key);
    }
    return iter->second;
}

std::string optionalArg(
    const std::map<std::string, std::string> &args,
    const std::string &key,
    const std::string &fallback)
{
    const auto iter = args.find(key);
    return iter == args.end() ? fallback : iter->second;
}

void printUsage()
{
    std::cout << "Usage: cwvig_edge_eval_cli --edges CWVIG_EDGES.csv --po TRUE_PO.txt --oo TRUE_OO.txt "
                 "--dimension-limit K [--prob-threshold VALUE] [--score-threshold VALUE] [--top-k K] "
                 "[--print-summary] [--output metrics.csv]\n";
}

void writeMetric(std::ostream &output, const std::string &name, const std::string &value)
{
    output << name << ',' << value << '\n';
}

void writeMetric(std::ostream &output, const std::string &name, const double value)
{
    output << name << ',' << value << '\n';
}

void writeMetricsCsv(
    const std::string &path,
    const std::size_t evaluated_edges,
    const flyki::probe::EdgeLabelCounts &counts,
    const double prob_threshold,
    const flyki::probe::EdgeClassificationMetrics &prob_metrics,
    const double score_threshold,
    const flyki::probe::EdgeClassificationMetrics &score_metrics,
    const flyki::probe::EdgeThresholdResult &best_prob,
    const flyki::probe::EdgeThresholdResult &best_score,
    const std::size_t top_k,
    const double top_k_precision,
    const double ranking_accuracy,
    const flyki::probe::EdgeScoreAverages &averages,
    const flyki::probe::ScoreDistributionSummary &scores)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write edge metrics CSV: " + path);
    }
    output << std::setprecision(12);
    output << "metric,value\n";
    writeMetric(output, "evaluated_edges", std::to_string(evaluated_edges));
    writeMetric(output, "positive_edges", std::to_string(counts.positive));
    writeMetric(output, "negative_edges", std::to_string(counts.negative));
    writeMetric(output, "prob_threshold", prob_threshold);
    writeMetric(output, "prob_precision", prob_metrics.precision);
    writeMetric(output, "prob_recall", prob_metrics.recall);
    writeMetric(output, "prob_f1", prob_metrics.f1);
    writeMetric(output, "score_threshold", score_threshold);
    writeMetric(output, "score_precision", score_metrics.precision);
    writeMetric(output, "score_recall", score_metrics.recall);
    writeMetric(output, "score_f1", score_metrics.f1);
    writeMetric(output, "best_probability_threshold", best_prob.threshold);
    writeMetric(output, "best_probability_f1", best_prob.f1);
    writeMetric(output, "best_score_threshold", best_score.threshold);
    writeMetric(output, "best_score_f1", best_score.f1);
    writeMetric(output, "top_k", std::to_string(top_k));
    writeMetric(output, "top_k_precision", top_k_precision);
    writeMetric(output, "ranking_accuracy", ranking_accuracy);
    writeMetric(output, "positive_mean_score", averages.positive_mean);
    writeMetric(output, "negative_mean_score", averages.negative_mean);
    writeMetric(output, "score_min", scores.min);
    writeMetric(output, "score_max", scores.max);
    writeMetric(output, "score_mean", scores.mean);
    writeMetric(output, "score_median", scores.median);
    writeMetric(output, "score_q25", scores.q25);
    writeMetric(output, "score_q75", scores.q75);
    writeMetric(output, "score_q90", scores.q90);
    writeMetric(output, "score_q95", scores.q95);
}

}  // namespace

int main(int argc, char **argv)
{
    try {
        const auto args = parseArgs(argc, argv);
        if (args.find("--help") != args.end() || args.find("-h") != args.end()) {
            printUsage();
            return 0;
        }

        const auto edges = flyki::probe::readCWVIGEdgesCsv(requireArg(args, "--edges"));
        const auto groups = flyki::grouping::readPoFile(requireArg(args, "--po"));
        const auto overlap_groups = flyki::grouping::readOoFile(requireArg(args, "--oo"));
        (void)overlap_groups;

        const auto dimension_limit = static_cast<std::size_t>(std::stoull(requireArg(args, "--dimension-limit")));
        const double prob_threshold = std::stod(optionalArg(args, "--prob-threshold", "0.5"));
        const double score_threshold = std::stod(optionalArg(args, "--score-threshold", "0.5"));
        const auto truth = flyki::probe::trueInteractionEdgesFromGroups(groups, dimension_limit);
        const auto counts = flyki::probe::countEdgeLabels(edges, truth);
        const auto prob_metrics = flyki::probe::calculateProbabilityMetrics(edges, truth, prob_threshold);
        const auto score_metrics = flyki::probe::calculateScoreMetrics(edges, truth, score_threshold);
        const auto best_prob = flyki::probe::bestF1ThresholdByProbability(edges, truth);
        const auto best_score = flyki::probe::bestF1ThresholdByScore(edges, truth);
        const std::size_t top_k = static_cast<std::size_t>(std::stoull(optionalArg(
            args,
            "--top-k",
            std::to_string(counts.positive == 0 ? 1 : counts.positive))));
        const double top_k_precision = flyki::probe::topKPrecisionByScore(edges, truth, top_k);
        const double ranking_accuracy = flyki::probe::rankingAccuracyByScore(edges, truth);
        const auto averages = flyki::probe::averageScoresByLabel(edges, truth);
        const auto scores = flyki::probe::summarizeScores(edges);

        if (args.find("--print-summary") != args.end()) {
            std::cout << std::setprecision(12);
            std::cout << "Evaluated Edges: " << edges.size() << '\n'
                      << "Positive Edges: " << counts.positive << '\n'
                      << "Negative Edges: " << counts.negative << '\n'
                      << "Probability Threshold: " << prob_threshold << '\n'
                      << "Probability Precision: " << prob_metrics.precision << '\n'
                      << "Probability Recall: " << prob_metrics.recall << '\n'
                      << "Probability F1: " << prob_metrics.f1 << '\n'
                      << "Score Threshold: " << score_threshold << '\n'
                      << "Score Precision: " << score_metrics.precision << '\n'
                      << "Score Recall: " << score_metrics.recall << '\n'
                      << "Score F1: " << score_metrics.f1 << '\n'
                      << "Best Probability Threshold: " << best_prob.threshold << '\n'
                      << "Best Probability F1: " << best_prob.f1 << '\n'
                      << "Best Score Threshold: " << best_score.threshold << '\n'
                      << "Best Score F1: " << best_score.f1 << '\n'
                      << "TopK: " << top_k << '\n'
                      << "TopK Precision: " << top_k_precision << '\n'
                      << "Ranking Accuracy: " << ranking_accuracy << '\n'
                      << "Positive Mean Score: " << averages.positive_mean << '\n'
                      << "Negative Mean Score: " << averages.negative_mean << '\n'
                      << "Score Min: " << scores.min << '\n'
                      << "Score Max: " << scores.max << '\n'
                      << "Score Mean: " << scores.mean << '\n'
                      << "Score Median: " << scores.median << '\n'
                      << "Score Q25: " << scores.q25 << '\n'
                      << "Score Q75: " << scores.q75 << '\n'
                      << "Score Q90: " << scores.q90 << '\n'
                      << "Score Q95: " << scores.q95 << '\n';
        }

        const auto output_path = optionalArg(args, "--output", "");
        if (!output_path.empty()) {
            writeMetricsCsv(
                output_path,
                edges.size(),
                counts,
                prob_threshold,
                prob_metrics,
                score_threshold,
                score_metrics,
                best_prob,
                best_score,
                top_k,
                top_k_precision,
                ranking_accuracy,
                averages,
                scores);
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "cwvig_edge_eval_cli: " << error.what() << '\n';
        printUsage();
        return 1;
    }
}
