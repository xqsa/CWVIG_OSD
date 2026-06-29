#include "probe/CWVIGEstimator.h"
#include "probe/SyntheticFunctions.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

const flyki::probe::CWVIGEdge *findEdge(
    const flyki::probe::CWVIGEstimateResult &result,
    const std::size_t i,
    const std::size_t j)
{
    const auto iter = std::find_if(result.edges.begin(), result.edges.end(), [&](const auto &edge) {
        return edge.i == i && edge.j == j;
    });
    return iter == result.edges.end() ? nullptr : &(*iter);
}

}  // namespace

int main()
{
    using flyki::probe::CWVIGEstimatorConfig;
    using flyki::probe::estimateCWVIG;
    using flyki::probe::interactionUncertainty;
    using flyki::probe::separableSphere;
    using flyki::probe::tinyOverlappingFunction;
    using flyki::probe::twoVariableInteraction;

    CWVIGEstimatorConfig config;
    config.dimension_limit = 4;
    config.contexts = 3;
    config.delta = 1e-4;
    config.seed = 11;
    config.epsilon = 1e-12;
    config.threshold_mode = "fixed";
    config.fixed_threshold = 0.5;

    const auto sphere = estimateCWVIG(separableSphere, config.dimension_limit, config);
    require(sphere.function_evaluations == config.contexts * 6 * 4, "sphere FE count");
    for (const auto &edge : sphere.edges) {
        require(edge.probability >= 0.0 && edge.probability <= 0.01, "sphere probabilities should be low");
        require(edge.uncertainty >= 0.0, "sphere uncertainty nonnegative");
    }

    const auto interaction = estimateCWVIG(twoVariableInteraction, config.dimension_limit, config);
    const auto *true_edge = findEdge(interaction, 0, 1);
    require(true_edge != nullptr, "true edge exists");
    for (const auto &edge : interaction.edges) {
        require(edge.probability >= 0.0 && edge.probability <= 1.0, "probability in range");
        require(edge.uncertainty >= 0.0, "uncertainty nonnegative");
        if (!(edge.i == 0 && edge.j == 1)) {
            require(true_edge->mean_abs_normalized > edge.mean_abs_normalized, "true edge ranks highest");
        }
    }

    CWVIGEstimatorConfig overlap_config = config;
    overlap_config.dimension_limit = 3;
    const auto overlap = estimateCWVIG(tinyOverlappingFunction, overlap_config.dimension_limit, overlap_config);
    std::map<std::size_t, double> weighted_degree;
    for (const auto &edge : overlap.edges) {
        weighted_degree[edge.i] += edge.probability;
        weighted_degree[edge.j] += edge.probability;
    }
    require(weighted_degree[1] > weighted_degree[0], "shared variable beats variable 0");
    require(weighted_degree[1] > weighted_degree[2], "shared variable beats variable 2");

    require(interactionUncertainty(0.5, config.epsilon) > interactionUncertainty(0.99, config.epsilon),
        "uncertainty is maximal near p=0.5");

    std::cout << "CWVIGEstimatorTests passed\n";
    return 0;
}
