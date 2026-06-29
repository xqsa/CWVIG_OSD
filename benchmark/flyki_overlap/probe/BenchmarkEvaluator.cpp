#include "probe/BenchmarkEvaluator.h"

#include "Header.h"

#include <stdexcept>

namespace flyki {
namespace probe {
namespace {

constexpr std::size_t kFlykiOverlapDimension = 905;

class ScopedCurrentPath {
public:
    explicit ScopedCurrentPath(const std::filesystem::path &path)
        : old_path_(std::filesystem::current_path())
    {
        std::filesystem::current_path(path);
    }

    ~ScopedCurrentPath()
    {
        std::filesystem::current_path(old_path_);
    }

private:
    std::filesystem::path old_path_;
};

}  // namespace

BenchmarkEvaluator::BenchmarkEvaluator(const int func, std::filesystem::path benchmark_root)
    : benchmark_root_(std::move(benchmark_root))
{
    if (func < 1 || func > 12) {
        throw std::invalid_argument("Invalid Flyki function id.");
    }
    if (!std::filesystem::exists(benchmark_root_)) {
        throw std::runtime_error("Benchmark root does not exist: " + benchmark_root_.string());
    }
    benchmark_ = generateFuncObj(func);
}

BenchmarkEvaluator::~BenchmarkEvaluator()
{
    // ponytail: Flyki destructors delete lazily initialized fields; only delete after a successful compute.
    if (computed_) {
        delete benchmark_;
    }
}

double BenchmarkEvaluator::evaluate(const std::vector<double> &x)
{
    if (x.size() != kFlykiOverlapDimension) {
        throw std::invalid_argument("Flyki evaluation vector must have dimension 905.");
    }

    std::vector<double> copy = x;
    ScopedCurrentPath cwd(benchmark_root_);
    const double value = benchmark_->compute(copy.data());
    computed_ = true;
    ++evaluations_;
    return value;
}

std::size_t BenchmarkEvaluator::dimension() const
{
    return kFlykiOverlapDimension;
}

std::size_t BenchmarkEvaluator::evaluations() const
{
    return evaluations_;
}

void BenchmarkEvaluator::resetEvaluations()
{
    evaluations_ = 0;
}

}  // namespace probe
}  // namespace flyki
