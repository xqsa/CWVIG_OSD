#include "grouping/GroupingIO.h"

#include "grouping/GroupingMetrics.h"

#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace flyki {
namespace grouping {
namespace {

std::string makeFileError(const std::string &action, const std::string &path)
{
    return "Failed to " + action + " grouping file: " + path;
}

int readIntValue(std::istream &input, const std::string &path, const std::string &field)
{
    long long value = 0;
    if (!(input >> value)) {
        throw std::runtime_error("Malformed grouping file " + path + ": missing " + field + ".");
    }
    if (value < 0 || value > std::numeric_limits<int>::max()) {
        throw std::runtime_error("Malformed grouping file " + path + ": invalid " + field + ".");
    }
    return static_cast<int>(value);
}

std::vector<std::vector<int>> readCountedGroups(const std::string &path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error(makeFileError("open", path));
    }

    std::vector<std::vector<int>> groups;
    while (true) {
        input >> std::ws;
        if (input.eof()) {
            break;
        }

        const int count = readIntValue(input, path, "group size");
        std::vector<int> group;
        group.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            group.push_back(readIntValue(input, path, "variable id"));
        }
        groups.push_back(group);
    }
    return groups;
}

void validateGroupsForWrite(const std::vector<std::vector<int>> &groups)
{
    for (const auto &group : groups) {
        for (const int variable : group) {
            if (variable < 0) {
                throw std::invalid_argument("Grouping variable ids must be non-negative.");
            }
        }
    }
}

void writeCountedGroups(const std::string &path, const std::vector<std::vector<int>> &groups)
{
    validateGroupsForWrite(groups);

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error(makeFileError("write", path));
    }

    for (const auto &group : groups) {
        output << group.size() << '\n';
        for (std::size_t i = 0; i < group.size(); ++i) {
            if (i > 0) {
                output << ' ';
            }
            output << group[i];
        }
        output << '\n';
    }
}

void appendSummary(std::ostringstream &output, const std::string &label, const GroupSummary &summary)
{
    output << label << " Groups: " << summary.number_groups << '\n'
           << label << " Total Variable Occurrences: " << summary.total_variable_occurrences << '\n'
           << label << " Unique Variables: " << summary.number_unique_variables << '\n'
           << label << " Group Size Min: " << summary.min_group_size << '\n'
           << label << " Group Size Max: " << summary.max_group_size << '\n'
           << label << " Group Size Mean: " << summary.mean_group_size << '\n'
           << label << " Shared Variables: " << summary.number_shared_variables << '\n';
}

}  // namespace

std::vector<std::vector<int>> readPoFile(const std::string &path)
{
    return readCountedGroups(path);
}

std::vector<std::vector<int>> readOoFile(const std::string &path)
{
    return readCountedGroups(path);
}

void writePoFile(const std::string &path, const std::vector<std::vector<int>> &groups)
{
    writeCountedGroups(path, groups);
}

void writeOoFile(const std::string &path, const std::vector<std::vector<int>> &overlap_groups)
{
    writeCountedGroups(path, overlap_groups);
}

std::string summarizeGrouping(const GroupingData &data)
{
    std::ostringstream output;
    appendSummary(output, "Primary", summarizeGroups(data.groups));
    appendSummary(output, "Overlap", summarizeGroups(data.overlap_groups));
    output << "Dimension: " << data.dimension << '\n'
           << "Unique Variables: " << data.unique_variables.size() << '\n'
           << "Shared Variables: " << data.shared_variables.size() << '\n';
    return output.str();
}

}  // namespace grouping
}  // namespace flyki
