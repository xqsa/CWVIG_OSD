import argparse
import csv
from collections import defaultdict
from pathlib import Path
from statistics import mean, median, pstdev


SUMMARY_COLUMNS = [
    "case",
    "func",
    "dimension_limit",
    "seed",
    "contexts",
    "preset",
    "truth_available",
    "delta",
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
    "exact_group_matches",
    "output_dir",
]

AGGREGATE_METRICS = [
    "shared_variables",
    "over_shared_ratio",
    "shared_f1",
    "mean_best_group_jaccard",
    "validation_errors",
    "function_evaluations",
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


def read_first_csv_row(path):
    if not path.exists():
        return {}
    with path.open(newline="", encoding="utf-8") as input_file:
        return next(csv.DictReader(input_file), {})


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
    grouping_metrics = read_first_csv_row(run_dir / "grouping_metrics.csv")
    case = summary.get("case", "")
    row = {
        "case": case,
        "func": config.get("func", case.removeprefix("flyki:") if case.startswith("flyki:") else ""),
        "dimension_limit": summary.get("dimension_limit", config.get("dimension_limit", "")),
        "seed": config.get("seed", ""),
        "contexts": summary.get("contexts", config.get("contexts", "")),
        "preset": summary.get("preset", config.get("preset", "")),
        "truth_available": "true" if truth_available else "false",
        "delta": summary.get("delta", config.get("delta", "")),
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
        "exact_group_matches": grouping_metrics.get("exact_group_matches", "") if truth_available else "",
        "output_dir": str(run_dir),
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


def aggregate_rows(rows, group_keys):
    grouped = defaultdict(list)
    for row in rows:
        grouped[tuple(row[key] for key in group_keys)].append(row)

    columns = list(group_keys) + [
        "runs",
        "truth_runs",
        "shared_zero_runs",
        "over_shared_ge_0_8_runs",
        "over_shared_le_0_05_runs",
        "validation_errors_total",
        "shared_variables_mean",
        "shared_variables_std",
        "over_shared_ratio_mean",
        "over_shared_ratio_std",
        "shared_f1_mean",
        "shared_f1_std",
        "shared_f1_median",
        "mean_best_group_jaccard_mean",
        "mean_best_group_jaccard_std",
        "validation_errors_mean",
        "validation_errors_std",
        "function_evaluations_mean",
        "function_evaluations_std",
    ]
    rows_out = []
    for key_tuple in sorted(grouped):
        group_rows = grouped[key_tuple]
        out = dict(zip(group_keys, key_tuple))
        out["runs"] = len(group_rows)
        out["truth_runs"] = sum(1 for row in group_rows if row["truth_available"] == "true")
        out["shared_zero_runs"] = sum(1 for row in group_rows if as_float(row, "shared_variables") == 0.0)
        out["over_shared_ge_0_8_runs"] = sum(1 for row in group_rows if (as_float(row, "over_shared_ratio") or 0.0) >= 0.8)
        out["over_shared_le_0_05_runs"] = sum(1 for row in group_rows if (as_float(row, "over_shared_ratio") or 0.0) <= 0.05)
        out["validation_errors_total"] = format_number(sum(as_float(row, "validation_errors") or 0.0 for row in group_rows))
        for key in AGGREGATE_METRICS:
            avg, std = mean_std(group_rows, key)
            out[f"{key}_mean"] = format_number(avg)
            out[f"{key}_std"] = format_number(std)
            if key == "shared_f1":
                values = stat_values(group_rows, key)
                out["shared_f1_median"] = format_number(median(values) if values else None)
        rows_out.append(out)
    return columns, rows_out


def write_grouped_csv(output_root, rows, group_keys, filename):
    columns, rows_out = aggregate_rows(rows, group_keys)
    path = output_root / filename
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows_out)
    return rows_out, path


def write_preset_summary(output_root, rows):
    rows_out, path = write_grouped_csv(output_root, rows, ["preset"], "preset_summary.csv")
    return rows_out, path


def preset_score(row):
    validation_total = float(row["validation_errors_total"] or "0")
    over_high = int(row["over_shared_ge_0_8_runs"] or "0")
    runs = int(row["runs"] or "0")
    zero_runs = int(row["shared_zero_runs"] or "0")
    shared_std = float(row["shared_variables_std"] or "0")
    f1 = row["shared_f1_mean"]
    f1_value = float(f1) if f1 != "" else 0.0
    preset_preference = {"balanced": 0, "capped": 1, "conservative": 2}.get(row["preset"], 3)
    truth_runs = int(row["truth_runs"] or "0")
    return (
        validation_total > 0,
        over_high > runs / 2.0,
        truth_runs > 0 and zero_runs == runs,
        -f1_value,
        float(row["shared_f1_std"] or "0"),
        shared_std,
        preset_preference,
    )


def smoke_recommendation(repo_root):
    path = repo_root / "results" / "grouping_batch" / "default_preset_selection.md"
    if not path.exists():
        return "unavailable"
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("Recommended default preset:"):
            return line.split("`", 2)[1]
    return "unavailable"


def write_selection_report(output_root, preset_rows):
    recommended = min(preset_rows, key=preset_score) if preset_rows else None
    path = output_root / "default_preset_selection.md"
    smoke = smoke_recommendation(Path(__file__).resolve().parents[1])
    with path.open("w", encoding="utf-8") as output:
        output.write("# Default Preset Selection\n\n")
        output.write("- Validation errors must be zero.\n")
        output.write("- Avoid over_shared_ratio close to 1.0.\n")
        output.write("- Avoid shared count always zero when truth exists.\n")
        output.write("- Prefer higher SharedVar F1 when truth exists.\n")
        output.write("- Prefer lower SharedVar-F1 std when means are close.\n")
        output.write("- Prefer stable shared count across seeds.\n")
        output.write("- Prefer lower FE only when grouping metrics are comparable.\n\n")
        if recommended is None:
            output.write("No completed runs were found.\n")
            return path
        output.write(f"Recommended default preset: `{recommended['preset']}`.\n\n")
        output.write("| Preset | Runs | Truth Runs | Shared Mean | Shared Std | Over-shared Mean | Over >= 0.8 | Shared F1 Mean | Shared F1 Std | Shared F1 Median | Jaccard Mean | Validation Total | FE Mean |\n")
        output.write("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
        for row in preset_rows:
            output.write(
                f"| {row['preset']} | {row['runs']} | {row['truth_runs']} | "
                f"{row['shared_variables_mean']} | {row['shared_variables_std']} | "
                f"{row['over_shared_ratio_mean']} | {row['over_shared_ge_0_8_runs']} | "
                f"{row['shared_f1_mean']} | {row['shared_f1_std']} | {row['shared_f1_median']} | "
                f"{row['mean_best_group_jaccard_mean']} | {row['validation_errors_total']} | "
                f"{row['function_evaluations_mean']} |\n"
            )
        output.write("\n## Smoke vs Medium\n\n")
        output.write(f"- Phase 5B smoke recommendation: `{smoke}`.\n")
        output.write(f"- Phase 5C medium recommendation: `{recommended['preset']}`.\n")
        if smoke == recommended["preset"]:
            output.write("- The smoke recommendation still holds under the medium batch.\n")
        else:
            output.write("- The medium batch changes the recommendation; use the medium result until broader evidence is available.\n")
        output.write("- `conservative` is treated as too strict when it predicts zero shared variables in every truth-available run.\n")
        output.write("- `balanced` is preferred over `capped` only when their metrics are close because it has less hard-coded shared-cap prior.\n")
        output.write("\nRationale: the deterministic ranking first removes presets with validation failures or frequent near-total over-sharing, then penalizes presets that always predict zero shared variables when truth metrics are available. Remaining order prefers higher mean SharedVar-F1, lower SharedVar-F1 std, lower shared-count variance, and finally the less cap-driven preset.\n")
    return path


def analyze(output_root):
    rows, summary_path = write_summary(output_root)
    preset_rows, preset_path = write_preset_summary(output_root, rows)
    _, by_dimension_path = write_grouped_csv(output_root, rows, ["dimension_limit", "preset"], "by_dimension_preset.csv")
    _, by_context_path = write_grouped_csv(output_root, rows, ["contexts", "preset"], "by_context_preset.csv")
    _, by_seed_path = write_grouped_csv(output_root, rows, ["seed", "preset"], "by_seed_preset.csv")
    report_path = write_selection_report(output_root, preset_rows)
    return summary_path, preset_path, by_dimension_path, by_context_path, by_seed_path, report_path


def main():
    parser = argparse.ArgumentParser(description="Analyze CWVIG grouping batch outputs.")
    parser.add_argument("--output-root", default="results/grouping_batch")
    args = parser.parse_args()
    summary_path, preset_path, by_dimension_path, by_context_path, by_seed_path, report_path = analyze(Path(args.output_root))
    print(f"summary={summary_path}")
    print(f"preset_summary={preset_path}")
    print(f"by_dimension_preset={by_dimension_path}")
    print(f"by_context_preset={by_context_path}")
    print(f"by_seed_preset={by_seed_path}")
    print(f"default_preset_selection={report_path}")


if __name__ == "__main__":
    main()
