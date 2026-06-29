#include "probe/CWVIGEdgeData.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace flyki {
namespace probe {
namespace {

std::string trim(const std::string &value)
{
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return value.substr(first, last - first);
}

std::vector<std::string> splitCsvLine(const std::string &line)
{
    std::vector<std::string> fields;
    std::stringstream input(line);
    std::string field;
    while (std::getline(input, field, ',')) {
        fields.push_back(trim(field));
    }
    return fields;
}

std::size_t parseSize(const std::string &text, const std::string &field)
{
    std::size_t parsed = 0;
    std::size_t pos = 0;
    try {
        parsed = static_cast<std::size_t>(std::stoull(text, &pos));
    } catch (const std::exception &) {
        throw std::runtime_error("Invalid CWVIG edge field " + field + ": " + text);
    }
    if (pos != text.size()) {
        throw std::runtime_error("Invalid CWVIG edge field " + field + ": " + text);
    }
    return parsed;
}

double parseDouble(const std::string &text, const std::string &field)
{
    double parsed = 0.0;
    std::size_t pos = 0;
    try {
        parsed = std::stod(text, &pos);
    } catch (const std::exception &) {
        throw std::runtime_error("Invalid CWVIG edge field " + field + ": " + text);
    }
    if (pos != text.size()) {
        throw std::runtime_error("Invalid CWVIG edge field " + field + ": " + text);
    }
    return parsed;
}

CWVIGEdge parseEdge(const std::vector<std::string> &fields, const std::string &path, const std::size_t line_number)
{
    if (fields.size() != 9) {
        throw std::runtime_error("Malformed CWVIG edge CSV " + path + " at line " + std::to_string(line_number) + ".");
    }

    CWVIGEdge edge;
    edge.i = parseSize(fields[0], "i");
    edge.j = parseSize(fields[1], "j");
    if (edge.i == edge.j) {
        throw std::runtime_error("Malformed CWVIG edge CSV " + path + ": self edge at line " + std::to_string(line_number) + ".");
    }
    edge.mean_abs_raw = parseDouble(fields[2], "mean_abs_raw");
    edge.mean_abs_normalized = parseDouble(fields[3], "mean_abs_normalized");
    edge.std_normalized = parseDouble(fields[4], "std_normalized");
    edge.threshold = parseDouble(fields[5], "threshold");
    edge.probability = parseDouble(fields[6], "probability");
    edge.uncertainty = parseDouble(fields[7], "uncertainty");
    edge.contexts = parseSize(fields[8], "contexts");
    return edge;
}

}  // namespace

std::vector<CWVIGEdge> readCWVIGEdgesCsv(const std::string &path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Failed to open CWVIG edge CSV: " + path);
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("Empty CWVIG edge CSV: " + path);
    }

    const std::string expected_header =
        "i,j,mean_abs_raw,mean_abs_normalized,std_normalized,threshold,probability,uncertainty,contexts";
    if (trim(line) != expected_header) {
        throw std::runtime_error("Unexpected CWVIG edge CSV header: " + path);
    }

    std::vector<CWVIGEdge> edges;
    std::size_t line_number = 1;
    while (std::getline(input, line)) {
        ++line_number;
        if (trim(line).empty()) {
            continue;
        }
        edges.push_back(parseEdge(splitCsvLine(line), path, line_number));
    }
    return edges;
}

}  // namespace probe
}  // namespace flyki
