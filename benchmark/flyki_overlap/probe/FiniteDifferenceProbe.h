#ifndef FLYKI_PROBE_FINITE_DIFFERENCE_PROBE_H_
#define FLYKI_PROBE_FINITE_DIFFERENCE_PROBE_H_

#include <functional>
#include <vector>

namespace flyki {
namespace probe {

double probePair(
    const std::function<double(const std::vector<double> &)> &f,
    const std::vector<double> &x,
    std::size_t i,
    std::size_t j,
    double delta);

}  // namespace probe
}  // namespace flyki

#endif  // FLYKI_PROBE_FINITE_DIFFERENCE_PROBE_H_
