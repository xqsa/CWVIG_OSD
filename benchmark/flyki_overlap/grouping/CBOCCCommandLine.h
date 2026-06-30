#ifndef FLYKI_GROUPING_CBOCC_COMMAND_LINE_H_
#define FLYKI_GROUPING_CBOCC_COMMAND_LINE_H_

#include <filesystem>
#include <string>

namespace flyki {
namespace grouping {

struct CBOCCCommandLine {
    int func = 0;
    std::string method;
    int seed = 0;
    long maxfes = 0;
    std::string grouping_source = "legacy_by_function";
    std::filesystem::path root = ".";
    std::filesystem::path po_path;
    std::filesystem::path oo_path;
    bool require_full_coverage = false;
    bool allow_partial_grouping = false;
    bool require_hard_overlap_compatible = false;
    bool allow_hard_overlap_incompatible = false;
    std::string completion_policy = "none";
};

CBOCCCommandLine parseCBOCCCommandLine(int argc, char **argv);
std::string cboccoUsage();

}  // namespace grouping
}  // namespace flyki

#endif  // FLYKI_GROUPING_CBOCC_COMMAND_LINE_H_
