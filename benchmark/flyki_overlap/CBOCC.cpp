#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <sstream>
#include "Header.h"
#include <float.h>
#include <cstdlib>
#include "CBOG_CBD.h"
#include "grouping/CBOCCGroupingLoader.h"

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

int main(int argc, char* argv[]) {
	int func = atoi(argv[1]);
	string method(argv[2]);
	int seed = atoi(argv[3]);
	long int maxfes = atol(argv[4]);
	int taskId = seed;
	int DIM = 905;
	Benchmarks* fp;
	fp = generateFuncObj(func);
	const auto grouping = flyki::grouping::loadLegacyGroupingForFunction(func, ".");

	if (method == "CBCCO") {
		CBOG_CBD oneSolver(fp, DIM, seed, 100, grouping.groups, grouping.overiables, grouping.overiablesRedandunt, grouping.sharedvar_group_pos, maxfes);
		oneSolver.testStage();
		oneSolver.optimizationStage();
	}
	else {
		cout << "No such way!!" << endl;
	}
	delete fp;
	return 0;
}
