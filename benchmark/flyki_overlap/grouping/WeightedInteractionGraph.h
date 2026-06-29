#ifndef FLYKI_GROUPING_WEIGHTED_INTERACTION_GRAPH_H_
#define FLYKI_GROUPING_WEIGHTED_INTERACTION_GRAPH_H_

#include "probe/CWVIGEstimator.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace flyki {
namespace grouping {

enum class InteractionScoreColumn {
    Probability,
    MeanAbsNormalized,
    MeanAbsRaw,
};

enum class EdgeWeightMode {
    Raw,
    Log1p,
    RankNormalized,
    UncertaintyPenalized,
};

enum class SparsificationMode {
    ScoreThreshold,
    TopKPerNode,
    MutualTopK,
    TargetAvgDegree,
};

struct SparsificationConfig {
    SparsificationMode mode = SparsificationMode::ScoreThreshold;
    double score_threshold = 0.0;
    std::size_t top_k_per_node = 0;
    double target_avg_degree = 0.0;
    double max_avg_degree = 0.0;
};

struct WeightedInteractionEdge {
    std::size_t i = 0;
    std::size_t j = 0;
    double weight = 0.0;
};

class WeightedInteractionGraph {
public:
    explicit WeightedInteractionGraph(std::size_t dimension);

    std::size_t dimension() const;
    void addEdge(std::size_t i, std::size_t j, double weight);
    double score(std::size_t i, std::size_t j) const;
    double weightedDegree(std::size_t variable) const;
    std::vector<WeightedInteractionEdge> edges() const;
    std::vector<WeightedInteractionEdge> strongEdges(double threshold) const;

private:
    std::size_t dimension_ = 0;
    std::map<std::pair<std::size_t, std::size_t>, double> weights_;
};

InteractionScoreColumn parseInteractionScoreColumn(const std::string &name);
std::string interactionScoreColumnName(InteractionScoreColumn column);
EdgeWeightMode parseEdgeWeightMode(const std::string &name);
std::string edgeWeightModeName(EdgeWeightMode mode);
SparsificationMode parseSparsificationMode(const std::string &name);
std::string sparsificationModeName(SparsificationMode mode);

WeightedInteractionGraph buildWeightedInteractionGraph(
    const std::vector<flyki::probe::CWVIGEdge> &edges,
    std::size_t dimension,
    InteractionScoreColumn column,
    bool use_log1p_score);

WeightedInteractionGraph buildWeightedInteractionGraph(
    const std::vector<flyki::probe::CWVIGEdge> &edges,
    std::size_t dimension,
    InteractionScoreColumn column,
    EdgeWeightMode edge_weight_mode,
    const SparsificationConfig &sparsification);

WeightedInteractionGraph buildWeightedInteractionGraphFromCsv(
    const std::string &path,
    std::size_t dimension,
    InteractionScoreColumn column,
    bool use_log1p_score);

WeightedInteractionGraph buildWeightedInteractionGraphFromCsv(
    const std::string &path,
    std::size_t dimension,
    InteractionScoreColumn column,
    EdgeWeightMode edge_weight_mode,
    const SparsificationConfig &sparsification);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_WEIGHTED_INTERACTION_GRAPH_H_
