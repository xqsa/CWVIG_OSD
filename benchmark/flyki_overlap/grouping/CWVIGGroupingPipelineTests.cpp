#include "grouping/CBOCCGroupingLoader.h"
#include "grouping/CWVIGGroupingPipeline.h"
#include "grouping/GroupingIO.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

namespace {

void require(const bool condition, const std::string &message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void writeTruth(
    const std::filesystem::path &po,
    const std::filesystem::path &oo,
    const std::vector<std::vector<int>> &groups,
    const std::vector<std::vector<int>> &overlap_groups)
{
    flyki::grouping::writePoFile(po.string(), groups);
    flyki::grouping::writeOoFile(oo.string(), overlap_groups);
}

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void writeText(const std::filesystem::path &path, const std::string &text)
{
    std::ofstream output(path);
    output << text;
}

flyki::grouping::CWVIGGroupingPipelineConfig baseSyntheticConfig(
    const std::filesystem::path &output_dir,
    const std::string &synthetic_function)
{
    auto config = flyki::grouping::pipelineConfigForPreset(flyki::grouping::PipelinePreset::Balanced);
    config.mode = flyki::grouping::PipelineMode::Synthetic;
    config.synthetic_function = synthetic_function;
    config.dimension_limit = 3;
    config.contexts = 3;
    config.seed = 11;
    config.delta = 0.0001;
    config.output_dir = output_dir;
    config.preset = flyki::grouping::PipelinePreset::Balanced;
    return config;
}

void requirePipelineFiles(const flyki::grouping::CWVIGGroupingPipelineOutputs &outputs, const bool has_truth)
{
    require(std::filesystem::exists(outputs.edges_csv), "edges.csv exists");
    require(std::filesystem::exists(outputs.z_csv), "z.csv exists");
    require(std::filesystem::exists(outputs.predicted_groups), "predicted_groups.txt exists");
    require(std::filesystem::exists(outputs.predicted_overlap), "predicted_overlap.txt exists");
    require(std::filesystem::exists(outputs.pipeline_config), "pipeline_config.txt exists");
    require(std::filesystem::exists(outputs.pipeline_summary), "pipeline_summary.txt exists");
    require(std::filesystem::exists(outputs.edge_metrics_csv) == has_truth, "edge metrics presence follows truth");
    require(std::filesystem::exists(outputs.grouping_metrics_csv) == has_truth, "grouping metrics presence follows truth");
}

}  // namespace

int main()
{
    const auto temp_root = std::filesystem::temp_directory_path() / "cwvig_grouping_pipeline_tests";
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    const auto true_po = temp_root / "path_true_po.txt";
    const auto true_oo = temp_root / "path_true_oo.txt";
    writeTruth(true_po, true_oo, {{0, 1}, {1, 2}}, {{1}, {1}});

    const auto capped_config_path = temp_root / "capped_default.json";
    writeText(capped_config_path, R"({
  "preset": "capped",
  "mode": "flyki",
  "func": 1,
  "dimension_limit": 10,
  "contexts": 3,
  "seed": 1,
  "delta": 0.0001,
  "edge_score_column": "mean_abs_normalized",
  "edge_weight_mode": "log1p",
  "sparsification_mode": "score_threshold",
  "membership_transform": "sigmoid_centered",
  "shared_rule": "capped_shared",
  "max_shared_ratio": 0.5,
  "target_avg_degree": 1.5
})");
    const auto loaded_config = flyki::grouping::loadPipelineConfigFile(capped_config_path);
    require(loaded_config.preset == flyki::grouping::PipelinePreset::Capped, "config loader reads capped preset");
    require(loaded_config.mode == flyki::grouping::PipelineMode::Flyki, "config loader reads mode");
    require(loaded_config.contexts == 3, "config loader reads contexts");
    require(loaded_config.seed == 1, "config loader reads seed");
    require(loaded_config.max_shared_ratio == 0.5, "config loader reads max shared ratio");
    require(loaded_config.edge_weight_mode == flyki::grouping::EdgeWeightMode::Log1p, "config loader reads edge weight mode");

    auto overlap_config = baseSyntheticConfig(temp_root / "overlap", "overlap");
    overlap_config.true_po_path = true_po;
    overlap_config.true_oo_path = true_oo;
    const auto overlap = flyki::grouping::runCWVIGGroupingPipeline(overlap_config);
    require(overlap.summary.groups == 2, "overlap pipeline emits two groups");
    require(overlap.summary.shared_variables == 1, "overlap pipeline emits one shared variable");
    require(overlap.summary.shared_set == std::set<int>({1}), "overlap pipeline marks variable 1 shared");
    require(overlap.summary.validation_errors == 0, "overlap po/oo validates");
    require(overlap.summary.shared_metrics.f1 == 1.0, "overlap shared F1 is perfect with truth");
    requirePipelineFiles(overlap.outputs, true);

    const auto loaded_overlap = flyki::grouping::loadLegacyGroupingFromFiles(
        overlap.outputs.predicted_groups,
        overlap.outputs.predicted_overlap);
    require(loaded_overlap.number_of_groups == 2, "overlap output is readable by explicit loader");

    auto clique_config = baseSyntheticConfig(temp_root / "clique", "clique");
    clique_config = flyki::grouping::pipelineConfigForPreset(flyki::grouping::PipelinePreset::Conservative);
    clique_config.mode = flyki::grouping::PipelineMode::Synthetic;
    clique_config.synthetic_function = "clique";
    clique_config.dimension_limit = 3;
    clique_config.contexts = 3;
    clique_config.seed = 11;
    clique_config.delta = 0.0001;
    clique_config.output_dir = temp_root / "clique";
    const auto clique = flyki::grouping::runCWVIGGroupingPipeline(clique_config);
    require(clique.summary.shared_variables == 0, "clique conservative pipeline has no shared variables");
    require(clique.summary.validation_errors == 0, "clique po/oo validates");

    const auto noedge = flyki::grouping::runCWVIGGroupingPipeline(
        baseSyntheticConfig(temp_root / "noedge", "sphere"));
    require(noedge.summary.groups == 3, "no-edge pipeline emits singleton groups");
    require(noedge.summary.shared_variables == 0, "no-edge pipeline has zero shared variables");
    require(noedge.summary.validation_errors == 0, "no-edge po/oo validates");
    requirePipelineFiles(noedge.outputs, false);

    const auto first = flyki::grouping::runCWVIGGroupingPipeline(
        baseSyntheticConfig(temp_root / "deterministic_a", "overlap"));
    const auto second = flyki::grouping::runCWVIGGroupingPipeline(
        baseSyntheticConfig(temp_root / "deterministic_b", "overlap"));
    require(readFile(first.outputs.pipeline_summary) == readFile(second.outputs.pipeline_summary),
        "pipeline summary is deterministic for fixed inputs");
    require(readFile(first.outputs.predicted_groups) == readFile(second.outputs.predicted_groups),
        "predicted groups are deterministic for fixed inputs");
    require(readFile(first.outputs.predicted_overlap) == readFile(second.outputs.predicted_overlap),
        "predicted overlap is deterministic for fixed inputs");

    auto flyki_config = flyki::grouping::pipelineConfigForPreset(flyki::grouping::PipelinePreset::Balanced);
    flyki_config.mode = flyki::grouping::PipelineMode::Flyki;
    flyki_config.func = 1;
    flyki_config.dimension_limit = 10;
    flyki_config.contexts = 3;
    flyki_config.seed = 11;
    flyki_config.delta = 0.0001;
    flyki_config.output_dir = temp_root / "flyki_balanced";
    flyki_config.preset = flyki::grouping::PipelinePreset::Balanced;
    const auto flyki = flyki::grouping::runCWVIGGroupingPipeline(flyki_config);
    require(flyki.summary.over_shared_ratio < 1.0, "balanced preset reduces Flyki over-sharing below raw baseline");
    require(flyki.summary.validation_errors == 0, "Flyki po/oo validates");

    std::filesystem::remove_all(temp_root);
    std::cout << "CWVIGGroupingPipelineTests passed\n";
    return 0;
}
