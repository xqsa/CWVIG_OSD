#ifndef FLYKI_GROUPING_GROUPING_IO_H_
#define FLYKI_GROUPING_GROUPING_IO_H_

#include "grouping/GroupingData.h"

#include <string>
#include <vector>

namespace flyki {
namespace grouping {

std::vector<std::vector<int>> readPoFile(const std::string &path);
std::vector<std::vector<int>> readOoFile(const std::string &path);

void writePoFile(const std::string &path, const std::vector<std::vector<int>> &groups);
void writeOoFile(const std::string &path, const std::vector<std::vector<int>> &overlap_groups);

std::string summarizeGrouping(const GroupingData &data);

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_GROUPING_IO_H_
