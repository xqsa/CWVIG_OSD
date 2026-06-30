#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include "Header.h"
#include <float.h>
#include <cstdlib>
#include "CBOG_CBD.h"
#include "grouping/CBOCCCommandLine.h"
#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/GroupingCoverageAudit.h"
#include "grouping/HardOverlapCompatibilityAudit.h"
#include "grouping/GroupingMetrics.h"

using namespace std;

Benchmarks* generateFuncObj(int funcID) {
	Benchmarks* fp;
	if (funcID == 1) {
		fp = new F1();
	}
	else if (funcID == 2) {
		fp = new F2();
	}
	else if (funcID == 3) {
		fp = new F3();
	}
	else if (funcID == 4) {
		fp = new F4();
	}
	else if (funcID == 5) {
		fp = new F5();
	}
	else if (funcID == 6) {
		fp = new F6();
	}
	else if (funcID == 7) {
		fp = new F7();
	}
	else if (funcID == 8) {
		fp = new F8();
	}
	else if (funcID == 9) {
		fp = new F9();
	}
	else if (funcID == 10) {
		fp = new F10();
	}
	else if (funcID == 11) {
		fp = new F11();
	}
	else if (funcID == 12) {
		fp = new F12();
	}
	else {
		cerr << "Fail to locate Specified Function Index" << endl;
		exit(-1);
	}
	return fp;
}

namespace {

bool isHelpRequest(int argc, char* argv[]) {
	if (argc != 2) {
		return false;
	}
	const string arg(argv[1]);
	return arg == "--help" || arg == "-h";
}

std::filesystem::path legacyPath(const flyki::grouping::CBOCCCommandLine& options, const string& suffix) {
	return options.root / (std::to_string(options.func) + suffix);
}

flyki::grouping::LegacyGroupingView loadGrouping(const flyki::grouping::CBOCCCommandLine& options) {
	if (options.grouping_source == "explicit_files") {
		return flyki::grouping::loadLegacyGroupingFromFiles(options.po_path, options.oo_path);
	}
	return flyki::grouping::loadLegacyGroupingForFunction(options.func, options.root);
}

std::string coverageProblemMessage(const flyki::grouping::GroupingCoverageAudit& audit) {
	std::ostringstream output;
	output << "Grouping coverage is partial: missing_variable_count=" << audit.missing_variable_count
		<< ", out_of_range_variables=" << audit.out_of_range_variables
		<< ", coverage_ratio=" << audit.coverage_ratio;
	return output.str();
}

void enforceCoveragePolicy(
	const flyki::grouping::CBOCCCommandLine& options,
	const flyki::grouping::GroupingCoverageAudit& audit) {
	if (audit.out_of_range_variables > 0) {
		throw std::runtime_error("Grouping contains out-of-range variables for the benchmark dimension.");
	}
	if (audit.full_coverage) {
		return;
	}

	const auto problem = coverageProblemMessage(audit);
	if (options.require_full_coverage || !options.allow_partial_grouping) {
		throw std::runtime_error(problem + ". Pass --allow-partial-grouping true for smoke-only partial runs, "
			"or use --completion-policy singletons|tail_group.");
	}
	std::cerr << "WARNING: " << problem
		<< ". The run is allowed only because --allow-partial-grouping true was set.\n";
}

void enforceHardOverlapPolicy(
	const flyki::grouping::CBOCCCommandLine& options,
	const flyki::grouping::HardOverlapCompatibilityAudit& audit) {
	if (audit.compatible) {
		return;
	}

	const auto problem = "Grouping is hard-overlap incompatible: zero_effective_group_count="
		+ std::to_string(audit.zero_effective_group_count);
	if (options.allow_hard_overlap_incompatible) {
		std::cerr << "WARNING: " << problem
			<< ". The run is allowed only because --allow-hard-overlap-incompatible true was set.\n";
		return;
	}
	if (options.require_hard_overlap_compatible) {
		throw std::runtime_error(problem
			+ ". Run hard_overlap_sanitize_cli or pass --allow-hard-overlap-incompatible true for debugging.");
	}
	std::cerr << "WARNING: " << problem
		<< ". The optimizer may abort because --require-hard-overlap-compatible true was not set.\n";
}

void printStartupLog(
	const flyki::grouping::CBOCCCommandLine& options,
	const flyki::grouping::LegacyGroupingView& original_grouping,
	const flyki::grouping::LegacyGroupingView& grouping,
	const int expected_dimension,
	const flyki::grouping::GroupingCoverageAudit& original_audit,
	const flyki::grouping::GroupingCoverageAudit& audit,
	const flyki::grouping::HardOverlapCompatibilityAudit& hard_overlap_audit,
	const flyki::grouping::CompletionPolicy completion_policy) {
	const auto po_path = options.grouping_source == "explicit_files" ? options.po_path : legacyPath(options, "po.txt");
	const auto oo_path = options.grouping_source == "explicit_files" ? options.oo_path : legacyPath(options, "oo.txt");
	const auto validation_errors = flyki::grouping::validateLegacyGroupingView(grouping);
	std::cout << "CBOCC startup" << '\n'
		<< "Function ID: " << options.func << '\n'
		<< "Method: " << options.method << '\n'
		<< "Seed: " << options.seed << '\n'
		<< "MaxFEs argument: " << options.maxfes << '\n'
		<< "Grouping Source: " << options.grouping_source << '\n'
		<< "Po Path: " << po_path.string() << '\n'
		<< "Oo Path: " << oo_path.string() << '\n'
		<< "Expected Benchmark Dimension: " << expected_dimension << '\n'
		<< "Original Grouping Inferred Dimension: " << original_grouping.dimension << '\n'
		<< "Original Covered Unique Variables: " << original_audit.covered_unique_variables << '\n'
		<< "Original Missing Variable Count: " << original_audit.missing_variable_count << '\n'
		<< "Completion Policy: " << flyki::grouping::completionPolicyName(completion_policy) << '\n'
		<< "Partial Grouping Allowed: " << (options.allow_partial_grouping ? "true" : "false") << '\n'
		<< "Require Full Coverage: " << (options.require_full_coverage ? "true" : "false") << '\n'
		<< "Require Hard-Overlap Compatible: " << (options.require_hard_overlap_compatible ? "true" : "false") << '\n'
		<< "Allow Hard-Overlap Incompatible: " << (options.allow_hard_overlap_incompatible ? "true" : "false") << '\n'
		<< "Number Of Groups: " << grouping.number_of_groups << '\n'
		<< "Grouping Inferred Dimension: " << grouping.dimension << '\n'
		<< "Covered Unique Variables: " << audit.covered_unique_variables << '\n'
		<< "Missing Variable Count: " << audit.missing_variable_count << '\n'
		<< "Duplicate Variable Occurrences: " << audit.duplicate_variable_occurrences << '\n'
		<< "Out Of Range Variables: " << audit.out_of_range_variables << '\n'
		<< "Coverage Ratio: " << audit.coverage_ratio << '\n'
		<< "Full Coverage: " << (audit.full_coverage ? "true" : "false") << '\n'
		<< "Shared Variables: " << flyki::grouping::extractSharedVariablesFromOo(grouping.overiablesRedandunt).size() << '\n'
		<< "Zero Effective Group Count: " << hard_overlap_audit.zero_effective_group_count << '\n'
		<< "Hard-Overlap Compatible: " << (hard_overlap_audit.compatible ? "true" : "false") << '\n'
		<< "Validation Errors: " << validation_errors.size() << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
	try {
		if (isHelpRequest(argc, argv)) {
			std::cout << flyki::grouping::cboccoUsage() << '\n';
			return 0;
		}
		const auto options = flyki::grouping::parseCBOCCCommandLine(argc, argv);
		const int expected_dimension = flyki::grouping::expectedFlykiOverlapDimension(options.func);
		int DIM = expected_dimension;
		const auto original_grouping = loadGrouping(options);
		const auto completion_policy = flyki::grouping::parseCompletionPolicy(options.completion_policy);
		const auto original_audit = flyki::grouping::auditGroupingCoverage(original_grouping, expected_dimension);
		const auto grouping = flyki::grouping::completeGroupingCoverage(
			original_grouping,
			expected_dimension,
			completion_policy);
		const auto audit = flyki::grouping::auditGroupingCoverage(grouping, expected_dimension);
		const auto hard_overlap_audit = flyki::grouping::auditHardOverlapCompatibility(grouping, expected_dimension);
		printStartupLog(options, original_grouping, grouping, expected_dimension, original_audit, audit, hard_overlap_audit, completion_policy);
		enforceCoveragePolicy(options, audit);
		enforceHardOverlapPolicy(options, hard_overlap_audit);

		if (options.method == "CBCCO") {
			std::unique_ptr<Benchmarks> fp(generateFuncObj(options.func));
			CBOG_CBD oneSolver(fp.get(), DIM, options.seed, 100, grouping.groups, grouping.overiables, grouping.overiablesRedandunt, grouping.sharedvar_group_pos, options.maxfes);
			oneSolver.testStage();
			oneSolver.optimizationStage();
		}
		else {
			cout << "No such way!!" << endl;
		}
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "cbocco: " << error.what() << '\n'
			<< flyki::grouping::cboccoUsage() << '\n';
		return 1;
	}
}
