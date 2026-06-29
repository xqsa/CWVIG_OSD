#ifndef FLYKI_PROBE_CWVIG_ESTIMATOR_H_
#define FLYKI_PROBE_CWVIG_ESTIMATOR_H_

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace flyki {
namespace probe {

struct CWVIGEstimatorConfig {
    std::size_t dimension_limit = 10;
    std::size_t contexts = 3;
    double delta = 1e-4;
    unsigned seed = 1;
    double epsilon = 1e-12;
    std::string threshold_mode = "fixed";
    double fixed_threshold = 0.5;
    bool output_all_pairs = true;
};

struct CWVIGEdge {
    std::size_t i = 0;
    std::size_t j = 0;
    double mean_abs_raw = 0.0;
    double mean_abs_normalized = 0.0;
    double std_normalized = 0.0;
    double threshold = 0.0;
    double probability = 0.0;
    double uncertainty = 0.0;
    std::size_t contexts = 0;
};

struct CWVIGEstimateResult {
    std::vector<CWVIGEdge> edges;
    std::size_t function_evaluations = 0;
    double threshold = 0.0;
};

double interactionProbability(double mean_abs_normalized, double threshold, double std_normalized, std::size_t contexts, double epsilon);
double interactionUncertainty(double probability, double epsilon);

CWVIGEstimateResult estimateCWVIG(
    const std::function<double(const std::vector<double> &)> &f,
    std::size_t full_dimension,
    const CWVIGEstimatorConfig &config);

void writeCWVIGEdgesCsv(const std::string &path, const CWVIGEstimateResult &result);

}  // namespace probe
}  // namespace flyki

#endif  // FLYKI_PROBE_CWVIG_ESTIMATOR_H_
