"""Analyze Phase 7H CWVIG coverage-scaling smoke outputs."""

from __future__ import annotations

import argparse
import csv
import statistics
import sys
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from experiments.analyze_cbocco_multiseed import fitness_at_or_before, read_curve  # noqa: E402


SUMMARY_COLUMNS = [
    "seed",
    "dimension_limit",
    "cwvig_coverage_ratio",
    "completion_added_variables",
    "completion_added_ratio",
    "predicted_group_count_before_completion",
    "group_count_after_completion",
    "shared_before_sanitize",
    "shared_after_sanitize",
    "zero_effective_groups_before",
    "zero_effective_groups_after",
    "sanitizer_repair_actions",
    "loader_validation_errors",
    "hard_overlap_compatible_after",
    "cbocco_exit_code",
    "result_file_exists",
    "final_fe",
    "final_fitness",
    "common_fe_vs_legacy",
    "fitness_at_common_fe",
    "legacy_fitness_at_common_fe",
    "relative_fe_difference",
    "result_file",
    "legacy_result_file",
]

AGGREGATE_COLUMNS = [
    "dimension_limit",
    "seed",
    "runs",
    "successful_results",
    "mean_cwvig_coverage_ratio",
    "mean_completion_added_ratio",
    "mean_zero_effective_groups_before",
    "mean_zero_effective_groups_after",
    "mean_sanitizer_repair_actions",
    "mean_final_fe",
    "mean_final_fitness",
    "mean_relative_fe_difference",
]


def read_counted_groups(path: Path) -> list[list[int]]:
    if not path.exists():
        return []
    tokens = path.read_text(encoding="utf-8").split()
    groups: list[list[int]] = []
    index = 0
    while index < len(tokens):
        count = int(tokens[index])
        index += 1
        group = [int(value) for value in tokens[index : index + count]]
        index += count
        groups.append(group)
    return groups


def unique_variables(groups: Iterable[Iterable[int]]) -> set[int]:
    return {variable for group in groups for variable in group}


def parse_summary_text(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", maxsplit=1)
        values[key.strip()] = value.strip()
    return values


def parse_sanitize_summary(path: Path) -> dict[str, object]:
    if not path.exists():
        return {}
    values: dict[str, object] = {}
    section = ""
    for line in path.read_text(encoding="utf-8").splitlines():
        text = line.strip()
        if text == "Before Repair":
            section = "before"
            continue
        if text == "After Repair":
            section = "after"
            continue
        if ":" not in text:
            continue
        key, value = [part.strip() for part in text.split(":", maxsplit=1)]
        if section == "before" and key == "Hard-Overlap Shared Variables":
            values["shared_before_sanitize"] = int(value)
        elif section == "after" and key == "Hard-Overlap Shared Variables":
            values["shared_after_sanitize"] = int(value)
        elif section == "before" and key == "Zero Effective Group Count":
            values["zero_effective_groups_before"] = int(value)
        elif section == "after" and key == "Zero Effective Group Count":
            values["zero_effective_groups_after"] = int(value)
        elif section == "after" and key == "Hard-Overlap Compatible":
            values["hard_overlap_compatible_after"] = value.lower()
        elif key == "Repair Actions":
            values["sanitizer_repair_actions"] = int(value)
    return values


def parse_int_or_blank(value: str | None) -> int | str:
    if value is None or value.strip() == "":
        return ""
    return int(value.strip())


def first_result_file(run_dir: Path) -> Path:
    matches = sorted(run_dir.glob("*.result.txt"))
    if matches:
        return matches[0]
    return run_dir / "missing.result.txt"


def analyze_run(
    phase_dir: Path,
    seed: int,
    dimension_limit: int,
    expected_dimension: int,
) -> dict[str, object]:
    seed_dir = phase_dir / f"seed_{seed}"
    run_dir = seed_dir / f"dim_{dimension_limit}"
    predicted_groups = read_counted_groups(run_dir / "predicted" / "predicted_groups.txt")
    completed_groups = read_counted_groups(run_dir / "completed_groups.txt")
    predicted_covered = len(unique_variables(predicted_groups))
    completion_added = max(0, expected_dimension - predicted_covered)
    sanitize = parse_sanitize_summary(run_dir / "sanitize_summary.txt")
    loader = parse_summary_text(run_dir / "sanitized_loader_summary.txt")
    cbocco_dir = run_dir / "sanitized_cbocco"
    result_file = first_result_file(cbocco_dir)
    legacy_result_file = first_result_file(seed_dir / "legacy")
    curve = read_curve(result_file)
    legacy_curve = read_curve(legacy_result_file)
    final_fe: int | str = curve[-1][0] if curve else ""
    final_fitness: float | str = curve[-1][1] if curve else ""
    common_fe: int | str = ""
    fitness_common: float | str = ""
    legacy_common: float | str = ""
    relative_fe_difference: float | str = ""
    if curve and legacy_curve:
        legacy_final_fe = legacy_curve[-1][0]
        common_fe = min(legacy_final_fe, curve[-1][0])
        fitness_common = fitness_at_or_before(curve, common_fe)
        legacy_common = fitness_at_or_before(legacy_curve, common_fe)
        relative_fe_difference = 0.0 if legacy_final_fe == 0 else (curve[-1][0] - legacy_final_fe) / legacy_final_fe

    row: dict[str, object] = {
        "seed": seed,
        "dimension_limit": dimension_limit,
        "cwvig_coverage_ratio": 0.0 if expected_dimension == 0 else dimension_limit / expected_dimension,
        "completion_added_variables": completion_added,
        "completion_added_ratio": 0.0 if expected_dimension == 0 else completion_added / expected_dimension,
        "predicted_group_count_before_completion": len(predicted_groups),
        "group_count_after_completion": len(completed_groups),
        "shared_before_sanitize": sanitize.get("shared_before_sanitize", ""),
        "shared_after_sanitize": sanitize.get("shared_after_sanitize", ""),
        "zero_effective_groups_before": sanitize.get("zero_effective_groups_before", ""),
        "zero_effective_groups_after": sanitize.get("zero_effective_groups_after", ""),
        "sanitizer_repair_actions": sanitize.get("sanitizer_repair_actions", ""),
        "loader_validation_errors": parse_int_or_blank(loader.get("Validation Errors")),
        "hard_overlap_compatible_after": sanitize.get("hard_overlap_compatible_after", ""),
        "cbocco_exit_code": parse_int_or_blank((cbocco_dir / "exit_code.txt").read_text(encoding="utf-8") if (cbocco_dir / "exit_code.txt").exists() else None),
        "result_file_exists": "true" if curve else "false",
        "final_fe": final_fe,
        "final_fitness": final_fitness,
        "common_fe_vs_legacy": common_fe,
        "fitness_at_common_fe": fitness_common if fitness_common is not None else "",
        "legacy_fitness_at_common_fe": legacy_common if legacy_common is not None else "",
        "relative_fe_difference": relative_fe_difference,
        "result_file": str(result_file),
        "legacy_result_file": str(legacy_result_file),
    }
    return row


def analyze_phase_dir(
    phase_dir: Path,
    seeds: list[int],
    dimension_limits: list[int],
    expected_dimension: int,
) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for seed in seeds:
        for dimension_limit in dimension_limits:
            rows.append(analyze_run(phase_dir, seed, dimension_limit, expected_dimension))
    return rows


def number_values(rows: Iterable[dict[str, object]], key: str) -> list[float]:
    values: list[float] = []
    for row in rows:
        value = row.get(key)
        if value == "" or value is None:
            continue
        values.append(float(value))
    return values


def mean(values: list[float]) -> float | str:
    return statistics.fmean(values) if values else ""


def aggregate_rows(rows: list[dict[str, object]], key: str) -> list[dict[str, object]]:
    groups: dict[object, list[dict[str, object]]] = {}
    for row in rows:
        groups.setdefault(row[key], []).append(row)
    summary: list[dict[str, object]] = []
    for value in sorted(groups):
        subset = groups[value]
        out = {
            "dimension_limit": value if key == "dimension_limit" else "",
            "seed": value if key == "seed" else "",
            "runs": len(subset),
            "successful_results": sum(1 for row in subset if row.get("result_file_exists") == "true"),
            "mean_cwvig_coverage_ratio": mean(number_values(subset, "cwvig_coverage_ratio")),
            "mean_completion_added_ratio": mean(number_values(subset, "completion_added_ratio")),
            "mean_zero_effective_groups_before": mean(number_values(subset, "zero_effective_groups_before")),
            "mean_zero_effective_groups_after": mean(number_values(subset, "zero_effective_groups_after")),
            "mean_sanitizer_repair_actions": mean(number_values(subset, "sanitizer_repair_actions")),
            "mean_final_fe": mean(number_values(subset, "final_fe")),
            "mean_final_fitness": mean(number_values(subset, "final_fitness")),
            "mean_relative_fe_difference": mean(number_values(subset, "relative_fe_difference")),
        }
        summary.append(out)
    return summary


def summarize_by_dimension(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    return aggregate_rows(rows, "dimension_limit")


def summarize_by_seed(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    return aggregate_rows(rows, "seed")


def write_csv(rows: Iterable[dict[str, object]], columns: list[str], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)


def write_report(
    rows: list[dict[str, object]],
    by_dimension: list[dict[str, object]],
    by_seed: list[dict[str, object]],
    path: Path,
) -> None:
    lines = [
        "# Phase 7H CWVIG Coverage-Scaling Smoke",
        "",
        "This is a coverage-scaling smoke/integration study, not a final benchmark.",
        "Larger dimension_limit reduces singleton completion but increases CWVIG probing cost.",
        "Original hard-overlap CBOG_CBD behavior is unchanged.",
        "Final FE may differ; common-FE analysis should be preferred when comparing curves.",
        "no final optimization superiority claim is made.",
        "",
        "## Runs",
        "",
        "| seed | dim | coverage | completion_added_ratio | zero_after | loader_errors | result | final_fe |",
        "| ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |",
    ]
    for row in rows:
        lines.append(
            "| {seed} | {dimension_limit} | {cwvig_coverage_ratio} | {completion_added_ratio} | {zero_effective_groups_after} | {loader_validation_errors} | {result_file_exists} | {final_fe} |".format(
                **row
            )
        )
    lines.extend(["", "## By Dimension", ""])
    lines.append("| dim | runs | successful | mean_completion_added_ratio | mean_zero_after | mean_final_fe |")
    lines.append("| ---: | ---: | ---: | ---: | ---: | ---: |")
    for row in by_dimension:
        lines.append(
            "| {dimension_limit} | {runs} | {successful_results} | {mean_completion_added_ratio} | {mean_zero_effective_groups_after} | {mean_final_fe} |".format(
                **row
            )
        )
    lines.extend(["", "## By Seed", ""])
    lines.append("| seed | runs | successful | mean_completion_added_ratio | mean_zero_after | mean_final_fe |")
    lines.append("| ---: | ---: | ---: | ---: | ---: | ---: |")
    for row in by_seed:
        lines.append(
            "| {seed} | {runs} | {successful_results} | {mean_completion_added_ratio} | {mean_zero_effective_groups_after} | {mean_final_fe} |".format(
                **row
            )
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase-dir", type=Path, default=Path("results/phase7h_coverage_scaling"))
    parser.add_argument("--seeds", nargs="+", type=int, default=[1, 3])
    parser.add_argument("--dimension-limits", nargs="+", type=int, default=[10, 20, 50])
    parser.add_argument("--expected-dimension", type=int, default=905)
    parser.add_argument("--summary-output", type=Path, default=Path("results/phase7h_coverage_scaling/coverage_scaling_summary.csv"))
    parser.add_argument("--by-dimension-output", type=Path, default=Path("results/phase7h_coverage_scaling/by_dimension_summary.csv"))
    parser.add_argument("--by-seed-output", type=Path, default=Path("results/phase7h_coverage_scaling/by_seed_summary.csv"))
    parser.add_argument("--report", type=Path, default=Path("results/phase7h_coverage_scaling/phase7h_report.md"))
    args = parser.parse_args()

    rows = analyze_phase_dir(args.phase_dir, args.seeds, args.dimension_limits, args.expected_dimension)
    by_dimension = summarize_by_dimension(rows)
    by_seed = summarize_by_seed(rows)
    write_csv(rows, SUMMARY_COLUMNS, args.summary_output)
    write_csv(by_dimension, AGGREGATE_COLUMNS, args.by_dimension_output)
    write_csv(by_seed, AGGREGATE_COLUMNS, args.by_seed_output)
    write_report(rows, by_dimension, by_seed, args.report)
    print(f"wrote {args.summary_output}")


if __name__ == "__main__":
    main()
