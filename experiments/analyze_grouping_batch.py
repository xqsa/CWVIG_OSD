import argparse
import csv
from collections import defaultdict
from pathlib import Path
from statistics import mean, pstdev


SUMMARY_COLUMNS = [
    "case",
    "func",
    "dimension_limit",
    "seed",
    "contexts",
    "preset",
    "truth_available",
    "function_evaluations",
    "graph_edges_before_pruning",
    "graph_edges_after_pruning",
    "groups",
    "shared_variables",
    "over_shared_ratio",
    "singleton_count",
    "mean_group_size",
    "max_group_size",
    "validation_errors",
    "shared_precision",
    "shared_recall",
    "shared_f1",
    "mean_best_group_jaccard",
    "run_dir",
]


def read_key_values(path):
    values = {}
    if not path.exists():
        return values
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return values


def as_float(row, key):
    value = row.get(key, "")
    return None if value == "" else float(value)


def format_number(value):
    if value is None:
        return ""
    return f"{value:.12g}"


def summarize_run(run_dir):
    config = read_key_values(run_dir / "pipeline_config.txt")
    summary = read_key_values(run_dir / "pipeline_summary.txt")
    if not summary:
        return None
    truth_available = (run_dir / "grouping_metrics.csv").exists()
    case = summary.get("case", "")
    row = {
        "case": case,
        "func": config.get("func", case.removeprefix("flyki:") if case.startswith("flyki:") else ""),
        "dimension_limit": summary.get("dimension_limit", config.get("dimension_limit", "")),
        "seed": config.get("seed", ""),
        "contexts": summary.get("contexts", config.get("contexts", "")),
        "preset": summary.get("preset", config.get("preset", "")),
        "truth_available": "true" if truth_available else "false",
        "function_evaluations": summary.get("function_evaluations", ""),
        "graph_edges_before_pruning": summary.get("graph_edges_before_pruning", ""),
        "graph_edges_after_pruning": summary.get("graph_edges_after_pruning", ""),
        "groups": summary.get("groups", ""),
        "shared_variables": summary.get("shared_variables", ""),
        "over_shared_ratio": summary.get("over_shared_ratio", ""),
        "singleton_count": summary.get("singleton_count", ""),
        "mean_group_size": summary.get("mean_group_size", ""),
        "max_group_size": summary.get("max_group_size", ""),
        "validation_errors": summary.get("validation_errors", ""),
        "shared_precision": summary.get("shared_precision", "") if truth_available else "",
        "shared_recall": summary.get("shared_recall", "") if truth_available else "",
        "shared_f1": summary.get("shared_f1", "") if truth_available else "",
        "mean_best_group_jaccard": summary.get("mean_best_group_jaccard", "") if truth_available else "",
        "run_dir": str(run_dir),
    }
    return row


def write_summary(output_root):
    runs_root = output_root / "runs"
    rows = []
    for summary_path in sorted(runs_root.glob("*/pipeline_summary.txt")):
        row = summarize_run(summary_path.parent)
        if row is not None:
            rows.append(row)
    output_root.mkdir(parents=True, exist_ok=True)
    summary_path = output_root / "summary.csv"
    with summary_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=SUMMARY_COLUMNS)
        writer.writeheader()
        writer.writerows(rows)
    return rows, summary_path


def stat_values(rows, key):
    values = [as_float(row, key) for row in rows]
    return [value for value in values if value is not None]


def mean_std(rows, key):
    values = stat_values(rows, key)
    if not values:
        return None, None
    return mean(values), pstdev(values)


def write_preset_summary(output_root, rows):
    grouped = defaultdict(list)
    for row in rows:
        grouped[row["preset"]].append(row)
    columns = [
        "preset",
        "runs",
        "truth_runs",
        "shared_variables_mean",
        "shared_variables_std",
        "over_shared_ratio_mean",
        "over_shared_ratio_std",
        "shared_f1_mean",
        "shared_f1_std",
        "mean_best_group_jaccard_mean",
        "mean_best_group_jaccard_std",
        "validation_errors_mean",
        "validation_errors_std",
        "function_evaluations_mean",
        "function_evaluations_std",
    ]
    rows_out = []
    for preset in sorted(grouped):
        preset_rows = grouped[preset]
        out = {
            "preset": preset,
            "runs": len(preset_rows),
            "truth_runs": sum(1 for row in preset_rows if row["truth_available"] == "true"),
        }
        for key in [
            "shared_variables",
            "over_shared_ratio",
            "shared_f1",
            "mean_best_group_jaccard",
            "validation_errors",
            "function_evaluations",
        ]:
            avg, std = mean_std(preset_rows, key)
            out[f"{key}_mean"] = format_number(avg)
            out[f"{key}_std"] = format_number(std)
        rows_out.append(out)
    path = output_root / "preset_summary.csv"
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows_out)
    return rows_out, path


def preset_score(row):
    validation = float(row["validation_errors_mean"] or "0")
    over = float(row["over_shared_ratio_mean"] or "1")
    shared = float(row["shared_variables_mean"] or "0")
    shared_std = float(row["shared_variables_std"] or "0")
    f1 = row["shared_f1_mean"]
    f1_value = float(f1) if f1 != "" else 0.0
    jaccard = row["mean_best_group_jaccard_mean"]
    jaccard_value = float(jaccard) if jaccard != "" else 0.0
    preset_preference = {"balanced": 0, "capped": 1, "conservative": 2}.get(row["preset"], 3)
    always_zero_penalty = 1 if shared == 0 else 0
    over_one_penalty = 1 if over >= 0.9 else 0
    return (
        validation > 0,
        over_one_penalty,
        always_zero_penalty,
        -f1_value,
        shared_std,
        -jaccard_value,
        preset_preference,
    )


def write_selection_report(output_root, preset_rows):
    recommended = min(preset_rows, key=preset_score) if preset_rows else None
    path = output_root / "default_preset_selection.md"
    with path.open("w", encoding="utf-8") as output:
        output.write("# Default Preset Selection\n\n")
        output.write("- Validation errors must be zero.\n")
        output.write("- Avoid over_shared_ratio close to 1.0.\n")
        output.write("- Avoid shared count always zero when truth exists.\n")
        output.write("- Prefer higher SharedVar F1 when truth exists.\n")
        output.write("- Prefer stable shared count across seeds.\n")
        output.write("- Prefer lower FE only when grouping metrics are comparable.\n\n")
        if recommended is None:
            output.write("No completed runs were found.\n")
            return path
        output.write(f"Recommended default preset: `{recommended['preset']}`.\n\n")
        output.write("| Preset | Runs | Truth Runs | Shared Mean | Over-shared Mean | Shared F1 Mean | Jaccard Mean | Validation Mean | FE Mean |\n")
        output.write("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
        for row in preset_rows:
            output.write(
                f"| {row['preset']} | {row['runs']} | {row['truth_runs']} | "
                f"{row['shared_variables_mean']} | {row['over_shared_ratio_mean']} | "
                f"{row['shared_f1_mean']} | {row['mean_best_group_jaccard_mean']} | "
                f"{row['validation_errors_mean']} | {row['function_evaluations_mean']} |\n"
            )
        output.write("\nRationale: the deterministic ranking first removes presets with validation failures or near-total over-sharing, then penalizes presets that always predict zero shared variables when truth metrics are available. Remaining ties prefer higher SharedVar F1, lower shared-count variance, higher group Jaccard, and finally the less cap-driven `balanced` preset.\n")
    return path


def analyze(output_root):
    rows, summary_path = write_summary(output_root)
    preset_rows, preset_path = write_preset_summary(output_root, rows)
    report_path = write_selection_report(output_root, preset_rows)
    return summary_path, preset_path, report_path


def main():
    parser = argparse.ArgumentParser(description="Analyze CWVIG grouping batch outputs.")
    parser.add_argument("--output-root", default="results/grouping_batch")
    args = parser.parse_args()
    summary_path, preset_path, report_path = analyze(Path(args.output_root))
    print(f"summary={summary_path}")
    print(f"preset_summary={preset_path}")
    print(f"default_preset_selection={report_path}")


if __name__ == "__main__":
    main()
