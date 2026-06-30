#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <sstream>
#include <exception>
#include <filesystem>
#include <memory>
#include "Header.h"
#include <float.h>
#include <cstdlib>
#include "CBOG_CBD.h"
#include "grouping/CBOCCCommandLine.h"
#include "grouping/CBOCCGroupingLoader.h"
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

void printStartupLog(
	const flyki::grouping::CBOCCCommandLine& options,
	const flyki::grouping::LegacyGroupingView& grouping) {
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
		<< "Number Of Groups: " << grouping.number_of_groups << '\n'
		<< "Dimension: " << grouping.dimension << '\n'
		<< "Shared Variables: " << flyki::grouping::extractSharedVariablesFromOo(grouping.overiablesRedandunt).size() << '\n'
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
		int DIM = 905;
		std::unique_ptr<Benchmarks> fp(generateFuncObj(options.func));
		const auto grouping = loadGrouping(options);
		printStartupLog(options, grouping);

		if (options.method == "CBCCO") {
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
