#ifndef FLYKI_PROBE_SYNTHETIC_FUNCTIONS_H_
#define FLYKI_PROBE_SYNTHETIC_FUNCTIONS_H_

#include <vector>

namespace flyki {
namespace probe {

double separableSphere(const std::vector<double> &x);
double twoVariableInteraction(const std::vector<double> &x);
double tinyOverlappingFunction(const std::vector<double> &x);
double tinyCliqueFunction(const std::vector<double> &x);

}  // namespace probe
}  // namespace flyki

#endif  // FLYKI_PROBE_SYNTHETIC_FUNCTIONS_H_
