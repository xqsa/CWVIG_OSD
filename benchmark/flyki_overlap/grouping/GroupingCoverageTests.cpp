#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingCoverageAudit.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void writeFile(const std::filesystem::path &path, const std::string &content)
{
    std::ofstream output(path);
    output << content;
}

}  // namespace

int main()
{
    using flyki::grouping::CompletionPolicy;
    using flyki::grouping::auditGroupingCoverage;
    using flyki::grouping::completeGroupingCoverage;
    using flyki::grouping::expectedFlykiOverlapDimension;
    using flyki::grouping::loadLegacyGroupingForFunction;
    using flyki::grouping::loadLegacyGroupingFromFiles;
    using flyki::grouping::validateLegacyGroupingView;

    require(expectedFlykiOverlapDimension(1) == 905, "func 1 expected dimension");
    require(expectedFlykiOverlapDimension(12) == 905, "func 12 expected dimension");

    const auto legacy = loadLegacyGroupingForFunction(1, "benchmark/flyki_overlap");
    const auto legacy_audit = auditGroupingCoverage(legacy, 905);
    require(legacy_audit.expected_dimension == 905, "legacy expected dimension");
    require(legacy_audit.covered_unique_variables == 905, "legacy covered variables");
    require(legacy_audit.missing_variable_count == 0, "legacy missing variables");
    require(legacy_audit.out_of_range_variables == 0, "legacy out of range variables");
    require(legacy_audit.full_coverage, "legacy full coverage");

    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_grouping_coverage_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);
    const auto po_path = temp_root / "predicted_groups.txt";
    const auto oo_path = temp_root / "predicted_overlap.txt";
    writeFile(po_path, "2\n0 1\n2\n1 2\n2\n8 9\n");
    writeFile(oo_path, "1\n1\n1\n1\n0\n");

    const auto partial = loadLegacyGroupingFromFiles(po_path, oo_path);
    const auto partial_audit = auditGroupingCoverage(partial, 905);
    require(partial_audit.covered_unique_variables == 5, "partial covered variables");
    require(partial_audit.missing_variable_count == 900, "partial missing variables");
    require(partial_audit.coverage_ratio > 0.005 && partial_audit.coverage_ratio < 0.006, "partial coverage ratio");
    require(!partial_audit.full_coverage, "partial should not be full coverage");

    auto bad_overlap = partial;
    bad_overlap.overiablesRedandunt[0] = {905};
    bad_overlap.overlap_groups = bad_overlap.overiablesRedandunt;
    const auto bad_overlap_audit = auditGroupingCoverage(bad_overlap, 905);
    require(bad_overlap_audit.out_of_range_variables == 1, "overlap out-of-range variable");
    require(!bad_overlap_audit.full_coverage, "overlap out-of-range should not be full coverage");

    const auto singleton_completed = completeGroupingCoverage(partial, 905, CompletionPolicy::Singletons);
    const auto singleton_audit = auditGroupingCoverage(singleton_completed, 905);
    require(singleton_audit.full_coverage, "singleton completion full coverage");
    require(validateLegacyGroupingView(singleton_completed).empty(), "singleton completion validation");

    const auto tail_completed = completeGroupingCoverage(partial, 905, CompletionPolicy::TailGroup);
    const auto tail_audit = auditGroupingCoverage(tail_completed, 905);
    require(tail_audit.full_coverage, "tail completion full coverage");
    require(validateLegacyGroupingView(tail_completed).empty(), "tail completion validation");

    std::filesystem::remove_all(temp_root);
    std::cout << "GroupingCoverageTests passed\n";
    return 0;
}
