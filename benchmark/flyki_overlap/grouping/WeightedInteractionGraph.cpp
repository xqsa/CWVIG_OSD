#include "grouping/WeightedInteractionGraph.h"

#include "probe/CWVIGEdgeData.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
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

double clamp01(const double value)
{
    return std::max(0.0, std::min(1.0, value));
}

std::map<double, double> rankMap(std::vector<double> scores)
{
    std::sort(scores.begin(), scores.end());
    scores.erase(std::unique(scores.begin(), scores.end()), scores.end());
    std::map<double, double> ranks;
    if (scores.empty()) {
        return ranks;
    }
    if (scores.size() == 1) {
        ranks[scores.front()] = 1.0;
        return ranks;
    }
    for (std::size_t i = 0; i < scores.size(); ++i) {
        ranks[scores[i]] = static_cast<double>(i) / static_cast<double>(scores.size() - 1);
    }
    return ranks;
}

std::vector<WeightedInteractionEdge> sortedEdgesByWeight(const WeightedInteractionGraph &graph)
{
    auto edges = graph.edges();
    std::sort(edges.begin(), edges.end(), [](const auto &left, const auto &right) {
        if (left.weight != right.weight) {
            return left.weight > right.weight;
        }
        if (left.i != right.i) {
            return left.i < right.i;
        }
        return left.j < right.j;
    });
    return edges;
}

std::set<std::pair<std::size_t, std::size_t>> topNeighbors(
    const WeightedInteractionGraph &graph,
    const std::size_t k)
{
    std::set<std::pair<std::size_t, std::size_t>> selected;
    if (k == 0) {
        return selected;
    }
    for (std::size_t variable = 0; variable < graph.dimension(); ++variable) {
        std::vector<WeightedInteractionEdge> incident;
        for (const auto &edge : graph.edges()) {
            if (edge.i == variable || edge.j == variable) {
                incident.push_back(edge);
            }
        }
        std::sort(incident.begin(), incident.end(), [variable](const auto &left, const auto &right) {
            const auto left_neighbor = left.i == variable ? left.j : left.i;
            const auto right_neighbor = right.i == variable ? right.j : right.i;
            if (left.weight != right.weight) {
                return left.weight > right.weight;
            }
            return left_neighbor < right_neighbor;
        });
        for (std::size_t idx = 0; idx < std::min(k, incident.size()); ++idx) {
            selected.insert(canonicalPair(incident[idx].i, incident[idx].j));
        }
    }
    return selected;
}

std::map<std::size_t, std::set<std::pair<std::size_t, std::size_t>>> topNeighborsByNode(
    const WeightedInteractionGraph &graph,
    const std::size_t k)
{
    std::map<std::size_t, std::set<std::pair<std::size_t, std::size_t>>> selected;
    if (k == 0) {
        return selected;
    }
    for (std::size_t variable = 0; variable < graph.dimension(); ++variable) {
        std::vector<WeightedInteractionEdge> incident;
        for (const auto &edge : graph.edges()) {
            if (edge.i == variable || edge.j == variable) {
                incident.push_back(edge);
            }
        }
        std::sort(incident.begin(), incident.end(), [variable](const auto &left, const auto &right) {
            const auto left_neighbor = left.i == variable ? left.j : left.i;
            const auto right_neighbor = right.i == variable ? right.j : right.i;
            if (left.weight != right.weight) {
                return left.weight > right.weight;
            }
            return left_neighbor < right_neighbor;
        });
        for (std::size_t idx = 0; idx < std::min(k, incident.size()); ++idx) {
            selected[variable].insert(canonicalPair(incident[idx].i, incident[idx].j));
        }
    }
    return selected;
}

WeightedInteractionGraph graphFromSelected(
    const WeightedInteractionGraph &graph,
    const std::set<std::pair<std::size_t, std::size_t>> &selected)
{
    WeightedInteractionGraph result(graph.dimension());
    for (const auto &edge : graph.edges()) {
        if (selected.count(canonicalPair(edge.i, edge.j)) != 0) {
            result.addEdge(edge.i, edge.j, edge.weight);
        }
    }
    return result;
}

WeightedInteractionGraph capAverageDegree(
    const WeightedInteractionGraph &graph,
    const double max_avg_degree)
{
    if (max_avg_degree <= 0.0 || graph.dimension() == 0) {
        return graph;
    }
    const std::size_t max_edges = static_cast<std::size_t>(
        std::floor(max_avg_degree * static_cast<double>(graph.dimension()) / 2.0));
    if (graph.edges().size() <= max_edges) {
        return graph;
    }
    WeightedInteractionGraph result(graph.dimension());
    const auto ranked = sortedEdgesByWeight(graph);
    for (std::size_t idx = 0; idx < std::min(max_edges, ranked.size()); ++idx) {
        result.addEdge(ranked[idx].i, ranked[idx].j, ranked[idx].weight);
    }
    return result;
}

WeightedInteractionGraph sparsifyGraph(
    const WeightedInteractionGraph &graph,
    const SparsificationConfig &config)
{
    WeightedInteractionGraph result(graph.dimension());
    if (config.mode == SparsificationMode::ScoreThreshold) {
        for (const auto &edge : graph.strongEdges(config.score_threshold)) {
            result.addEdge(edge.i, edge.j, edge.weight);
        }
        return capAverageDegree(result, config.max_avg_degree);
    }
    if (config.mode == SparsificationMode::TopKPerNode) {
        return capAverageDegree(graphFromSelected(graph, topNeighbors(graph, config.top_k_per_node)), config.max_avg_degree);
    }
    if (config.mode == SparsificationMode::MutualTopK) {
        const auto selected_by_node = topNeighborsByNode(graph, config.top_k_per_node);
        std::set<std::pair<std::size_t, std::size_t>> mutual;
        for (const auto &edge : graph.edges()) {
            const auto key = canonicalPair(edge.i, edge.j);
            const auto i_iter = selected_by_node.find(edge.i);
            const auto j_iter = selected_by_node.find(edge.j);
            if (i_iter != selected_by_node.end() && j_iter != selected_by_node.end()
                && i_iter->second.count(key) != 0 && j_iter->second.count(key) != 0) {
                mutual.insert(key);
            }
        }
        return capAverageDegree(graphFromSelected(graph, mutual), config.max_avg_degree);
    }
    if (config.mode == SparsificationMode::TargetAvgDegree) {
        const std::size_t keep = static_cast<std::size_t>(
            std::floor(config.target_avg_degree * static_cast<double>(graph.dimension()) / 2.0));
        const auto ranked = sortedEdgesByWeight(graph);
        for (std::size_t idx = 0; idx < std::min(keep, ranked.size()); ++idx) {
            result.addEdge(ranked[idx].i, ranked[idx].j, ranked[idx].weight);
        }
        return capAverageDegree(result, config.max_avg_degree);
    }
    return capAverageDegree(graph, config.max_avg_degree);
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

EdgeWeightMode parseEdgeWeightMode(const std::string &name)
{
    if (name == "raw") {
        return EdgeWeightMode::Raw;
    }
    if (name == "log1p") {
        return EdgeWeightMode::Log1p;
    }
    if (name == "rank_normalized") {
        return EdgeWeightMode::RankNormalized;
    }
    if (name == "uncertainty_penalized") {
        return EdgeWeightMode::UncertaintyPenalized;
    }
    throw std::invalid_argument("Invalid edge weight mode: " + name);
}

std::string edgeWeightModeName(const EdgeWeightMode mode)
{
    switch (mode) {
    case EdgeWeightMode::Raw:
        return "raw";
    case EdgeWeightMode::Log1p:
        return "log1p";
    case EdgeWeightMode::RankNormalized:
        return "rank_normalized";
    case EdgeWeightMode::UncertaintyPenalized:
        return "uncertainty_penalized";
    }
    return "unknown";
}

SparsificationMode parseSparsificationMode(const std::string &name)
{
    if (name == "score_threshold") {
        return SparsificationMode::ScoreThreshold;
    }
    if (name == "top_k_per_node") {
        return SparsificationMode::TopKPerNode;
    }
    if (name == "mutual_top_k") {
        return SparsificationMode::MutualTopK;
    }
    if (name == "target_avg_degree") {
        return SparsificationMode::TargetAvgDegree;
    }
    throw std::invalid_argument("Invalid sparsification mode: " + name);
}

std::string sparsificationModeName(const SparsificationMode mode)
{
    switch (mode) {
    case SparsificationMode::ScoreThreshold:
        return "score_threshold";
    case SparsificationMode::TopKPerNode:
        return "top_k_per_node";
    case SparsificationMode::MutualTopK:
        return "mutual_top_k";
    case SparsificationMode::TargetAvgDegree:
        return "target_avg_degree";
    }
    return "unknown";
}

WeightedInteractionGraph buildWeightedInteractionGraph(
    const std::vector<flyki::probe::CWVIGEdge> &edges,
    const std::size_t dimension,
    const InteractionScoreColumn column,
    const bool use_log1p_score)
{
    SparsificationConfig sparsification;
    WeightedInteractionGraph graph(dimension);
    for (const auto &edge : edges) {
        if (edge.i >= dimension || edge.j >= dimension) {
            continue;
        }
        graph.addEdge(edge.i, edge.j, scaleScore(edgeScore(edge, column), use_log1p_score));
    }
    return sparsifyGraph(graph, sparsification);
}

WeightedInteractionGraph buildWeightedInteractionGraph(
    const std::vector<flyki::probe::CWVIGEdge> &edges,
    const std::size_t dimension,
    const InteractionScoreColumn column,
    const EdgeWeightMode edge_weight_mode,
    const SparsificationConfig &sparsification)
{
    std::vector<double> raw_scores;
    raw_scores.reserve(edges.size());
    for (const auto &edge : edges) {
        if (edge.i < dimension && edge.j < dimension) {
            raw_scores.push_back(edgeScore(edge, column));
        }
    }
    const auto ranks = rankMap(raw_scores);

    WeightedInteractionGraph graph(dimension);
    for (const auto &edge : edges) {
        if (edge.i >= dimension || edge.j >= dimension) {
            continue;
        }
        const double raw = edgeScore(edge, column);
        double weight = raw;
        if (edge_weight_mode == EdgeWeightMode::Log1p) {
            weight = scaleScore(raw, true);
        } else if (edge_weight_mode == EdgeWeightMode::RankNormalized) {
            weight = ranks.empty() ? 0.0 : ranks.at(raw);
        } else if (edge_weight_mode == EdgeWeightMode::UncertaintyPenalized) {
            const double normalized = ranks.empty() ? 0.0 : ranks.at(raw);
            const double certainty = clamp01(1.0 - edge.uncertainty / std::log(2.0));
            weight = clamp01(normalized * certainty);
        }
        graph.addEdge(edge.i, edge.j, weight);
    }
    return sparsifyGraph(graph, sparsification);
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

WeightedInteractionGraph buildWeightedInteractionGraphFromCsv(
    const std::string &path,
    const std::size_t dimension,
    const InteractionScoreColumn column,
    const EdgeWeightMode edge_weight_mode,
    const SparsificationConfig &sparsification)
{
    return buildWeightedInteractionGraph(
        flyki::probe::readCWVIGEdgesCsv(path),
        dimension,
        column,
        edge_weight_mode,
        sparsification);
}

}  // namespace grouping
}  // namespace flyki
