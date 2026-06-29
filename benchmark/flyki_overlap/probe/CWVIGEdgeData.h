#ifndef FLYKI_PROBE_CWVIG_EDGE_DATA_H_
#define FLYKI_PROBE_CWVIG_EDGE_DATA_H_

#include "probe/CWVIGEstimator.h"

#include <string>
#include <vector>

namespace flyki {
namespace probe {

std::vector<CWVIGEdge> readCWVIGEdgesCsv(const std::string &path);

}  // namespace probe
}  // namespace flyki

#endif  // FLYKI_PROBE_CWVIG_EDGE_DATA_H_
