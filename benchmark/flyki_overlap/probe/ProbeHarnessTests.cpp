#include "probe/BenchmarkEvaluator.h"
#include "probe/FiniteDifferenceProbe.h"
#include "probe/SyntheticFunctions.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void requireNear(double actual, double expected, double tolerance, const std::string &message)
{
    require(std::fabs(actual - expected) <= tolerance, message);
}

}  // namespace

int main()
{
    using flyki::probe::BenchmarkEvaluator;
    using flyki::probe::probePair;
    using flyki::probe::separableSphere;
    using flyki::probe::tinyOverlappingFunction;
    using flyki::probe::twoVariableInteraction;

    const std::vector<double> x = {0.3, -0.2, 0.7};
    requireNear(probePair(separableSphere, x, 0, 1, 1e-4), 0.0, 1e-10, "sphere pair should be near zero");
    require(std::fabs(probePair(twoVariableInteraction, x, 0, 1, 1e-4)) > 1e-10, "interaction pair should be nonzero");
    require(std::fabs(probePair(tinyOverlappingFunction, x, 0, 1, 1e-4)) > 1e-10, "overlap pair 0,1 should be nonzero");
    requireNear(probePair(tinyOverlappingFunction, x, 0, 2, 1e-4), 0.0, 1e-10, "overlap pair 0,2 should be near zero");

    BenchmarkEvaluator evaluator(1, "benchmark/flyki_overlap");
    require(evaluator.dimension() == 905, "Flyki evaluator dimension");
    std::vector<double> flyki_x(evaluator.dimension(), 0.0);
    const double value = evaluator.evaluate(flyki_x);
    require(std::isfinite(value), "Flyki F1 value should be finite");
    require(evaluator.evaluations() == 1, "FE count after one evaluate");
    probePair([&](const std::vector<double> &candidate) {
        return evaluator.evaluate(candidate);
    }, flyki_x, 0, 1, 1e-4);
    require(evaluator.evaluations() == 5, "FE count after one pair probe");
    evaluator.resetEvaluations();
    require(evaluator.evaluations() == 0, "FE reset");

    std::cout << "ProbeHarnessTests passed\n";
    return 0;
}
