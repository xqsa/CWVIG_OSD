#include "probe/SyntheticFunctions.h"

#include <stdexcept>

namespace flyki {
namespace probe {

double separableSphere(const std::vector<double> &x)
{
    double result = 0.0;
    for (const double value : x) {
        result += value * value;
    }
    return result;
}

double twoVariableInteraction(const std::vector<double> &x)
{
    if (x.size() < 2) {
        throw std::invalid_argument("twoVariableInteraction requires at least two variables.");
    }
    return x[0] * x[1];
}

double tinyOverlappingFunction(const std::vector<double> &x)
{
    if (x.size() < 3) {
        throw std::invalid_argument("tinyOverlappingFunction requires at least three variables.");
    }
    const double first = x[0] + x[1];
    const double second = x[1] + x[2];
    return first * first + second * second;
}

double tinyCliqueFunction(const std::vector<double> &x)
{
    if (x.size() < 3) {
        throw std::invalid_argument("tinyCliqueFunction requires at least three variables.");
    }
    const double sum = x[0] + x[1] + x[2];
    return sum * sum;
}

}  // namespace probe
}  // namespace flyki
