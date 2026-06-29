#include "probe/CWVIGEstimator.h"

#include "probe/FiniteDifferenceProbe.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <random>
#include <stdexcept>

namespace flyki {
namespace probe {
namespace {

std::vector<double> randomContext(const std::size_t dimension, std::mt19937 &rng)
{
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<double> x(dimension);
    for (double &value : x) {
        value = dist(rng);
    }
    return x;
}

double mean(const std::vector<double> &values)
{
    if (values.empty()) {
        return 0.0;
    }
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double standardDeviation(const std::vector<double> &values, const double value_mean)
{
    if (values.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const double value : values) {
        const double diff = value - value_mean;
        sum += diff * diff;
    }
    return std::sqrt(sum / static_cast<double>(values.size()));
}

double medianThreshold(std::vector<CWVIGEdge> edges)
{
    if (edges.empty()) {
        return 0.0;
    }
    std::vector<double> values;
    values.reserve(edges.size());
    for (const auto &edge : edges) {
        values.push_back(edge.mean_abs_normalized);
    }
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

void validateConfig(const std::size_t full_dimension, const CWVIGEstimatorConfig &config)
{
    if (config.dimension_limit < 2 || config.dimension_limit > full_dimension) {
        throw std::invalid_argument("dimension_limit must be in [2, full_dimension].");
    }
    if (config.contexts == 0) {
        throw std::invalid_argument("contexts must be positive.");
    }
    if (config.delta == 0.0) {
        throw std::invalid_argument("delta must be nonzero.");
    }
    if (config.epsilon <= 0.0) {
        throw std::invalid_argument("epsilon must be positive.");
    }
    if (config.threshold_mode != "fixed" && config.threshold_mode != "quantile") {
        throw std::invalid_argument("threshold_mode must be fixed or quantile.");
    }
}

}  // namespace

double interactionProbability(
    const double mean_abs_normalized,
    const double threshold,
    const double std_normalized,
    const std::size_t contexts,
    const double epsilon)
{
    const double denom = (std_normalized / std::sqrt(static_cast<double>(contexts))) + epsilon;
    const double z = (mean_abs_normalized - threshold) / denom;
    if (z >= 40.0) {
        return 1.0;
    }
    if (z <= -40.0) {
        return 0.0;
    }
    return 1.0 / (1.0 + std::exp(-z));
}

double interactionUncertainty(const double probability, const double epsilon)
{
    const double p = std::min(1.0, std::max(0.0, probability));
    const double q = 1.0 - p;
    const double entropy = -p * std::log(p + epsilon) - q * std::log(q + epsilon);
    return std::max(0.0, entropy);
}

CWVIGEstimateResult estimateCWVIG(
    const std::function<double(const std::vector<double> &)> &f,
    const std::size_t full_dimension,
    const CWVIGEstimatorConfig &config)
{
    validateConfig(full_dimension, config);

    CWVIGEstimateResult result;
    std::mt19937 rng(config.seed);
    const double norm_scale = std::fabs(config.delta * config.delta) + config.epsilon;

    for (std::size_t i = 0; i < config.dimension_limit; ++i) {
        for (std::size_t j = i + 1; j < config.dimension_limit; ++j) {
            std::vector<double> raw_values;
            std::vector<double> normalized_values;
            raw_values.reserve(config.contexts);
            normalized_values.reserve(config.contexts);

            for (std::size_t r = 0; r < config.contexts; ++r) {
                const auto x = randomContext(full_dimension, rng);
                std::size_t calls = 0;
                const auto counted = [&](const std::vector<double> &candidate) {
                    ++calls;
                    return f(candidate);
                };
                const double raw = probePair(counted, x, i, j, config.delta);
                result.function_evaluations += calls;
                raw_values.push_back(std::fabs(raw));
                normalized_values.push_back(std::fabs(raw) / norm_scale);
            }

            CWVIGEdge edge;
            edge.i = i;
            edge.j = j;
            edge.mean_abs_raw = mean(raw_values);
            edge.mean_abs_normalized = mean(normalized_values);
            edge.std_normalized = standardDeviation(normalized_values, edge.mean_abs_normalized);
            edge.contexts = config.contexts;
            result.edges.push_back(edge);
        }
    }

    const double threshold = config.threshold_mode == "fixed" ? config.fixed_threshold : medianThreshold(result.edges);
    result.threshold = threshold;
    for (auto &edge : result.edges) {
        edge.threshold = threshold;
        edge.probability = interactionProbability(
            edge.mean_abs_normalized,
            threshold,
            edge.std_normalized,
            config.contexts,
            config.epsilon);
        edge.uncertainty = interactionUncertainty(edge.probability, config.epsilon);
    }

    if (!config.output_all_pairs) {
        result.edges.erase(std::remove_if(result.edges.begin(), result.edges.end(), [](const auto &edge) {
            return edge.probability <= 0.5;
        }), result.edges.end());
    }
    return result;
}

void writeCWVIGEdgesCsv(const std::string &path, const CWVIGEstimateResult &result)
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to write CWVIG CSV: " + path);
    }
    output << "i,j,mean_abs_raw,mean_abs_normalized,std_normalized,threshold,probability,uncertainty,contexts\n";
    for (const auto &edge : result.edges) {
        output << edge.i << ','
               << edge.j << ','
               << edge.mean_abs_raw << ','
               << edge.mean_abs_normalized << ','
               << edge.std_normalized << ','
               << edge.threshold << ','
               << edge.probability << ','
               << edge.uncertainty << ','
               << edge.contexts << '\n';
    }
}

}  // namespace probe
}  // namespace flyki
