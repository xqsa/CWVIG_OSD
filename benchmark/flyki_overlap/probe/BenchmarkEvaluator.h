#ifndef FLYKI_PROBE_BENCHMARK_EVALUATOR_H_
#define FLYKI_PROBE_BENCHMARK_EVALUATOR_H_

#include <cstddef>
#include <filesystem>
#include <vector>

class Benchmarks;

namespace flyki {
namespace probe {

class BenchmarkEvaluator {
public:
    explicit BenchmarkEvaluator(
        int func,
        std::filesystem::path benchmark_root = std::filesystem::path("benchmark/flyki_overlap"));
    ~BenchmarkEvaluator();

    BenchmarkEvaluator(const BenchmarkEvaluator &) = delete;
    BenchmarkEvaluator &operator=(const BenchmarkEvaluator &) = delete;

    double evaluate(const std::vector<double> &x);
    std::size_t dimension() const;
    std::size_t evaluations() const;
    void resetEvaluations();

private:
    Benchmarks *benchmark_ = nullptr;
    std::filesystem::path benchmark_root_;
    std::size_t evaluations_ = 0;
    bool computed_ = false;
};

}  // namespace probe
}  // namespace flyki

#endif  // FLYKI_PROBE_BENCHMARK_EVALUATOR_H_
