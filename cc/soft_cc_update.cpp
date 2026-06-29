// Shared-variable-aware cooperative coevolution update helpers.

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace cwvig {

struct CandidateUpdate {
    std::vector<double> values;
    double fitness = 0.0;
};

double shared_blend_weight(double shared_confidence, double min_weight, double max_weight) {
    if (min_weight < 0.0 || max_weight > 1.0 || min_weight > max_weight) {
        throw std::invalid_argument("invalid blend weight range");
    }
    const double confidence = std::clamp(shared_confidence, 0.0, 1.0);
    return min_weight + (max_weight - min_weight) * confidence;
}

void blend_shared_variables(
    std::vector<double>& context,
    const std::vector<double>& candidate,
    const std::vector<int>& variable_indices,
    const std::vector<double>& shared_confidence,
    double min_weight,
    double max_weight
) {
    if (context.size() != candidate.size() || context.size() != shared_confidence.size()) {
        throw std::invalid_argument("context, candidate, and shared_confidence sizes must match");
    }

    for (const int variable : variable_indices) {
        if (variable < 0 || static_cast<std::size_t>(variable) >= context.size()) {
            throw std::out_of_range("variable index is out of range");
        }
        const std::size_t index = static_cast<std::size_t>(variable);
        const double alpha = shared_blend_weight(shared_confidence[index], min_weight, max_weight);
        context[index] = (1.0 - alpha) * context[index] + alpha * candidate[index];
    }
}

}  // namespace cwvig
