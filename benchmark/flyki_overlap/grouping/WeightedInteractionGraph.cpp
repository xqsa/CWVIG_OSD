#include "grouping/WeightedInteractionGraph.h"

#include "probe/CWVIGEdgeData.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

std::pair<std::size_t, std::size_t> canonicalPair(const std::size_t i, const std::size_t j)
{
    if (i == j) {
        throw std::invalid_argument("Self interaction edges are not supported.");
    }
    return i < j ? std::make_pair(i, j) : std::make_pair(j, i);
}

double edgeScore(const flyki::probe::CWVIGEdge &edge, const InteractionScoreColumn column)
{
    switch (column) {
    case InteractionScoreColumn::Probability:
        return edge.probability;
    case InteractionScoreColumn::MeanAbsNormalized:
        return edge.mean_abs_normalized;
    case InteractionScoreColumn::MeanAbsRaw:
        return edge.mean_abs_raw;
    }
    throw std::invalid_argument("Unknown interaction score column.");
}

double scaleScore(const double score, const bool use_log1p_score)
{
    if (!use_log1p_score) {
        return score;
    }
    if (score < 0.0) {
        throw std::invalid_argument("log1p score scaling requires non-negative scores.");
    }
    return std::log1p(score);
}

}  // namespace

WeightedInteractionGraph::WeightedInteractionGraph(const std::size_t dimension)
    : dimension_(dimension)
{
}

std::size_t WeightedInteractionGraph::dimension() const
{
    return dimension_;
}

void WeightedInteractionGraph::addEdge(const std::size_t i, const std::size_t j, const double weight)
{
    if (i >= dimension_ || j >= dimension_) {
        throw std::out_of_range("Weighted interaction edge endpoint exceeds graph dimension.");
    }
    const auto key = canonicalPair(i, j);
    const auto iter = weights_.find(key);
    if (iter == weights_.end()) {
        weights_[key] = weight;
    } else {
        iter->second = std::max(iter->second, weight);
    }
}

double WeightedInteractionGraph::score(const std::size_t i, const std::size_t j) const
{
    if (i == j || i >= dimension_ || j >= dimension_) {
        return 0.0;
    }
    const auto iter = weights_.find(i < j ? std::make_pair(i, j) : std::make_pair(j, i));
    return iter == weights_.end() ? 0.0 : iter->second;
}

double WeightedInteractionGraph::weightedDegree(const std::size_t variable) const
{
    double degree = 0.0;
    for (const auto &entry : weights_) {
        if (entry.first.first == variable || entry.first.second == variable) {
            degree += entry.second;
        }
    }
    return degree;
}

std::vector<WeightedInteractionEdge> WeightedInteractionGraph::edges() const
{
    std::vector<WeightedInteractionEdge> result;
    result.reserve(weights_.size());
    for (const auto &entry : weights_) {
        result.push_back({entry.first.first, entry.first.second, entry.second});
    }
    return result;
}

std::vector<WeightedInteractionEdge> WeightedInteractionGraph::strongEdges(const double threshold) const
{
    std::vector<WeightedInteractionEdge> result;
    for (const auto &edge : edges()) {
        if (edge.weight >= threshold) {
            result.push_back(edge);
        }
    }
    return result;
}

InteractionScoreColumn parseInteractionScoreColumn(const std::string &name)
{
    if (name == "probability") {
        return InteractionScoreColumn::Probability;
    }
    if (name == "mean_abs_normalized") {
        return InteractionScoreColumn::MeanAbsNormalized;
    }
    if (name == "mean_abs_raw") {
        return InteractionScoreColumn::MeanAbsRaw;
    }
    throw std::invalid_argument("Invalid score column: " + name);
}

std::string interactionScoreColumnName(const InteractionScoreColumn column)
{
    switch (column) {
    case InteractionScoreColumn::Probability:
        return "probability";
    case InteractionScoreColumn::MeanAbsNormalized:
        return "mean_abs_normalized";
    case InteractionScoreColumn::MeanAbsRaw:
        return "mean_abs_raw";
    }
    return "unknown";
}

WeightedInteractionGraph buildWeightedInteractionGraph(
    const std::vector<flyki::probe::CWVIGEdge> &edges,
    const std::size_t dimension,
    const InteractionScoreColumn column,
    const bool use_log1p_score)
{
    WeightedInteractionGraph graph(dimension);
    for (const auto &edge : edges) {
        if (edge.i >= dimension || edge.j >= dimension) {
            continue;
        }
        graph.addEdge(edge.i, edge.j, scaleScore(edgeScore(edge, column), use_log1p_score));
    }
    return graph;
}

WeightedInteractionGraph buildWeightedInteractionGraphFromCsv(
    const std::string &path,
    const std::size_t dimension,
    const InteractionScoreColumn column,
    const bool use_log1p_score)
{
    return buildWeightedInteractionGraph(
        flyki::probe::readCWVIGEdgesCsv(path),
        dimension,
        column,
        use_log1p_score);
}

}  // namespace grouping
}  // namespace flyki
