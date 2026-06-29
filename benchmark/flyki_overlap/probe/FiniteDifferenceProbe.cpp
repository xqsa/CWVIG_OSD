#include "probe/FiniteDifferenceProbe.h"

#include <stdexcept>

namespace flyki {
namespace probe {

double probePair(
    const std::function<double(const std::vector<double> &)> &f,
    const std::vector<double> &x,
    const std::size_t i,
    const std::size_t j,
    const double delta)
{
    if (i >= x.size() || j >= x.size() || i == j) {
        throw std::invalid_argument("probePair requires two distinct valid indices.");
    }
    if (delta == 0.0) {
        throw std::invalid_argument("probePair delta must be nonzero.");
    }

    std::vector<double> xi = x;
    std::vector<double> xj = x;
    std::vector<double> xij = x;
    xi[i] += delta;
    xj[j] += delta;
    xij[i] += delta;
    xij[j] += delta;
    return f(xij) - f(xi) - f(xj) + f(x);
}

}  // namespace probe
}  // namespace flyki
