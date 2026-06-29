#include "grouping/GroupingData.h"
#include "grouping/GroupingIO.h"
#include "grouping/LegacyGroupingAdapter.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

template <typename T>
void requireEqual(const T &actual, const T &expected, const std::string &message)
{
    if (!(actual == expected)) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using namespace flyki::grouping;

    {
        const auto data = makeGroupingData({{0, 1}, {1, 2}}, {{1}, {1}});
        const auto view = makeLegacyGroupingView(data);
        requireEqual(view.number_of_groups, std::size_t{2}, "tiny group count");
        requireEqual(view.dimension, 3, "tiny dimension");
        requireEqual(view.overiables, std::vector<std::vector<int>>({{1}}), "tiny overiables");
        requireEqual(view.overiablesRedandunt, std::vector<std::vector<int>>({{1}, {1}}), "tiny raw overlap");
        requireEqual(view.sharedvar_group_pos.at(1), std::vector<std::pair<int, int>>({{0, 1}, {1, 0}}), "tiny shared map");
        require(validateLegacyGroupingView(view).empty(), "tiny view should validate");
    }

    {
        const auto data = makeGroupingData({{5}, {5, 7}, {8, 5}}, {{5}, {5}, {5}});
        const auto view = makeLegacyGroupingView(data);
        requireEqual(view.overiables, std::vector<std::vector<int>>({{5}}), "three-way overiables");
        requireEqual(
            view.sharedvar_group_pos.at(5),
            std::vector<std::pair<int, int>>({{0, 0}, {1, 0}, {2, 1}}),
            "three-way shared map");
        require(validateLegacyGroupingView(view).empty(), "three-way view should validate");
    }

    {
        const auto data = makeGroupingData({{2, 3}, {2}}, {{2, 2}, {2}});
        const auto view = makeLegacyGroupingView(data);
        requireEqual(view.overiablesRedandunt[0], std::vector<int>({2, 2}), "duplicate raw overlap kept");
        requireEqual(view.overiables, std::vector<std::vector<int>>({{2}}), "duplicate unique overlap");
        requireEqual(
            view.sharedvar_group_pos.at(2),
            std::vector<std::pair<int, int>>({{0, 0}, {0, 0}, {1, 0}}),
            "duplicate shared map keeps CBOCC duplicate pair");
        requireEqual(formatSharedMap(view), std::string("2: 0,0 0,0 1,0\n"), "deterministic shared map format");
    }

    {
        auto view = makeLegacyGroupingView(makeGroupingData({{0}}, {{1}}));
        require(!validateLegacyGroupingView(view).empty(), "invalid overlap should be reported");
    }

    {
        const auto groups = readPoFile("benchmark/flyki_overlap/1po.txt");
        const auto overlap = readOoFile("benchmark/flyki_overlap/1oo.txt");
        const auto view = makeLegacyGroupingView(makeGroupingData(groups, overlap));
        requireEqual(view.number_of_groups, std::size_t{20}, "real group count");
        requireEqual(view.dimension, 905, "real dimension");
        requireEqual(view.overiablesRedandunt.size(), std::size_t{20}, "real raw overlap count");
        requireEqual(view.sharedvar_group_pos.size(), std::size_t{95}, "real shared map entries");
        require(validateLegacyGroupingView(view).empty(), "real 1po/1oo should validate");
    }

    std::cout << "GroupingProviderTests passed\n";
    return 0;
}
