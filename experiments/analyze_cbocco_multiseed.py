"""Analyze Phase 7E CBOCC multi-seed smoke results with common-FE matching."""

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


DEFAULT_CANDIDATE_NAME = "completed_predicted"
DEFAULT_CANDIDATE_PREFIX = "completed_predicted"

SANITIZER_COLUMNS = [
    "shared_before_sanitize",
    "shared_after_sanitize",
    "zero_effective_groups_before",
    "zero_effective_groups_after",
    "repair_policy",
    "repaired_variables_or_groups_count",
    "loader_validation_errors",
    "hard_overlap_compatible_after",
    "legacy_hard_overlap_compatible",
    "sanitized_hard_overlap_compatible",
    "both_result_files_present",
    "both_hard_overlap_compatible",
]

BASE_COMPARISON_COLUMNS = [
    "seed",
    "final_fe_legacy",
    "final_fitness_legacy",
]

COMMON_COMPARISON_COLUMNS = [
    "common_fe",
    "legacy_fitness_at_common_fe",
    "final_fe_difference",
    "relative_fe_difference",
    "fe_matched",
    "winner_final_fitness",
    "winner_common_fe_fitness",
    "legacy_result_file",
]

COMPARISON_COLUMNS = [
    *BASE_COMPARISON_COLUMNS,
    "final_fe_completed_predicted",
    "final_fitness_completed_predicted",
    "common_fe",
    "legacy_fitness_at_common_fe",
    "completed_predicted_fitness_at_common_fe",
    "final_fe_difference",
    "relative_fe_difference",
    "fe_matched",
    "winner_final_fitness",
    "winner_common_fe_fitness",
    "legacy_result_file",
    "completed_predicted_result_file",
]

SUMMARY_COLUMNS = [
    "grouping_source",
    "seeds",
    "mean_final_fitness",
    "std_final_fitness",
    "mean_common_fe_fitness",
    "std_common_fe_fitness",
    "mean_final_fe",
    "final_fitness_wins",
    "common_fe_fitness_wins",
    "mean_relative_fe_difference",
]


def read_curve(path: str | Path) -> list[tuple[int, float]]:
    result_path = Path(path)
    if not result_path.exists():
        return []
    rows: list[tuple[int, float]] = []
    for line in result_path.read_text(encoding="utf-8").splitlines():
        text = line.strip()
        if not text:
            continue
        fe_text, fitness_text = text.split(",", maxsplit=1)
        rows.append((int(fe_text), float(fitness_text)))
    return rows


def fitness_at_or_before(curve: list[tuple[int, float]], target_fe: int) -> float | None:
    candidate: float | None = None
    for fe, fitness in curve:
        if fe <= target_fe:
            candidate = fitness
        else:
            break
    return candidate


def winner(left_name: str, left_value: object, right_name: str, right_value: object) -> str:
    if left_value == "" or right_value == "" or left_value is None or right_value is None:
        return "unavailable"
    left = float(left_value)
    right = float(right_value)
    if left < right:
        return left_name
    if right < left:
        return right_name
    return "tie"


def comparison_columns(candidate_prefix: str, include_sanitizer_metadata: bool) -> list[str]:
    candidate_columns = [
        f"final_fe_{candidate_prefix}",
        f"final_fitness_{candidate_prefix}",
    ]
    common_columns = [
        "common_fe",
        "legacy_fitness_at_common_fe",
        f"{candidate_prefix}_fitness_at_common_fe",
        "final_fe_difference",
        "relative_fe_difference",
        "fe_matched",
        "winner_final_fitness",
        "winner_common_fe_fitness",
        "legacy_result_file",
        f"{candidate_prefix}_result_file",
    ]
    columns = [*BASE_COMPARISON_COLUMNS, *candidate_columns, *common_columns]
    if include_sanitizer_metadata:
        columns.extend(SANITIZER_COLUMNS)
    return columns


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


def int_or_blank(value: str | None) -> int | str:
    if value is None or value == "":
        return ""
    return int(value)


def parse_sanitizer_summary(path: Path) -> dict[str, object]:
    if not path.exists():
        return {}
    metadata: dict[str, object] = {}
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
        if key == "Repair Policy":
            metadata["repair_policy"] = value
        elif section == "before" and key == "Hard-Overlap Shared Variables":
            metadata["shared_before_sanitize"] = int(value)
        elif section == "after" and key == "Hard-Overlap Shared Variables":
            metadata["shared_after_sanitize"] = int(value)
        elif section == "before" and key == "Zero Effective Group Count":
            metadata["zero_effective_groups_before"] = int(value)
        elif section == "after" and key == "Zero Effective Group Count":
            metadata["zero_effective_groups_after"] = int(value)
        elif section == "after" and key == "Hard-Overlap Compatible":
            metadata["hard_overlap_compatible_after"] = value.lower()
    return metadata


def count_csv_data_rows(path: Path) -> int | str:
    if not path.exists():
        return ""
    with path.open(newline="", encoding="utf-8") as source:
        return sum(1 for _ in csv.DictReader(source))


def parse_sanitizer_metadata(seed_dir: Path) -> dict[str, object]:
    metadata = parse_sanitizer_summary(seed_dir / "audit" / "sanitize_summary.txt")
    loader = parse_summary_text(seed_dir / "grouping" / "sanitized_loader_summary.txt")
    legacy_log = parse_summary_text(seed_dir / "legacy" / "cbocco_stdout.txt")
    sanitized_log = parse_summary_text(seed_dir / "sanitized_completed_predicted" / "cbocco_stdout.txt")
    metadata["repaired_variables_or_groups_count"] = count_csv_data_rows(seed_dir / "audit" / "sanitizer_report.csv")
    metadata["loader_validation_errors"] = int_or_blank(loader.get("Validation Errors"))
    metadata["legacy_hard_overlap_compatible"] = legacy_log.get("Hard-Overlap Compatible", "").lower()
    metadata["sanitized_hard_overlap_compatible"] = sanitized_log.get("Hard-Overlap Compatible", "").lower()
    return metadata


def compare_seed(
    seed: int,
    legacy_result: str | Path,
    completed_result: str | Path,
    *,
    fe_tolerance: int,
    candidate_name: str = DEFAULT_CANDIDATE_NAME,
    candidate_prefix: str = DEFAULT_CANDIDATE_PREFIX,
    metadata: dict[str, object] | None = None,
) -> dict[str, object]:
    legacy_curve = read_curve(legacy_result)
    completed_curve = read_curve(completed_result)
    final_fe_legacy: int | str = legacy_curve[-1][0] if legacy_curve else ""
    final_fitness_legacy: float | str = legacy_curve[-1][1] if legacy_curve else ""
    final_fe_completed: int | str = completed_curve[-1][0] if completed_curve else ""
    final_fitness_completed: float | str = completed_curve[-1][1] if completed_curve else ""
    row: dict[str, object]
    if not legacy_curve or not completed_curve:
        row = {
            "seed": seed,
            "final_fe_legacy": final_fe_legacy,
            "final_fitness_legacy": final_fitness_legacy,
            f"final_fe_{candidate_prefix}": final_fe_completed,
            f"final_fitness_{candidate_prefix}": final_fitness_completed,
            "common_fe": "",
            "legacy_fitness_at_common_fe": "",
            f"{candidate_prefix}_fitness_at_common_fe": "",
            "final_fe_difference": "",
            "relative_fe_difference": "",
            "fe_matched": "false",
            "winner_final_fitness": "unavailable",
            "winner_common_fe_fitness": "unavailable",
            "legacy_result_file": str(legacy_result),
            f"{candidate_prefix}_result_file": str(completed_result),
        }
        if metadata is not None:
            row.update(metadata)
            row["both_result_files_present"] = "false"
            row["both_hard_overlap_compatible"] = "false"
        return row

    common_fe = min(final_fe_legacy, final_fe_completed)
    legacy_common = fitness_at_or_before(legacy_curve, common_fe)
    completed_common = fitness_at_or_before(completed_curve, common_fe)
    difference = final_fe_completed - final_fe_legacy
    relative = 0.0 if final_fe_legacy == 0 else difference / final_fe_legacy
    row = {
        "seed": seed,
        "final_fe_legacy": final_fe_legacy,
        "final_fitness_legacy": final_fitness_legacy,
        f"final_fe_{candidate_prefix}": final_fe_completed,
        f"final_fitness_{candidate_prefix}": final_fitness_completed,
        "common_fe": common_fe,
        "legacy_fitness_at_common_fe": legacy_common if legacy_common is not None else "",
        f"{candidate_prefix}_fitness_at_common_fe": completed_common if completed_common is not None else "",
        "final_fe_difference": difference,
        "relative_fe_difference": relative,
        "fe_matched": "true" if abs(difference) <= fe_tolerance else "false",
        "winner_final_fitness": winner(
            "legacy",
            final_fitness_legacy,
            candidate_name,
            final_fitness_completed,
        ),
        "winner_common_fe_fitness": winner("legacy", legacy_common, candidate_name, completed_common),
        "legacy_result_file": str(legacy_result),
        f"{candidate_prefix}_result_file": str(completed_result),
    }
    if metadata is not None:
        row.update(metadata)
        row["both_result_files_present"] = "true"
        legacy_ok = str(metadata.get("legacy_hard_overlap_compatible", "")).lower() == "true"
        candidate_ok = str(metadata.get(f"{candidate_prefix}_hard_overlap_compatible", "")).lower() == "true"
        row["both_hard_overlap_compatible"] = "true" if legacy_ok and candidate_ok else "false"
    return row


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


def stdev(values: list[float]) -> float | str:
    return statistics.stdev(values) if len(values) > 1 else (0.0 if values else "")


def summarize_pairs(
    rows: list[dict[str, object]],
    candidate_name: str = DEFAULT_CANDIDATE_NAME,
    candidate_prefix: str = DEFAULT_CANDIDATE_PREFIX,
) -> list[dict[str, object]]:
    sources = [
        (
            "legacy",
            "final_fitness_legacy",
            "legacy_fitness_at_common_fe",
            "final_fe_legacy",
        ),
        (
            candidate_name,
            f"final_fitness_{candidate_prefix}",
            f"{candidate_prefix}_fitness_at_common_fe",
            f"final_fe_{candidate_prefix}",
        ),
    ]
    summary: list[dict[str, object]] = []
    relative_values = number_values(rows, "relative_fe_difference")
    for source, final_key, common_key, fe_key in sources:
        final_values = number_values(rows, final_key)
        common_values = number_values(rows, common_key)
        fe_values = number_values(rows, fe_key)
        summary.append(
            {
                "grouping_source": source,
                "seeds": len(final_values),
                "mean_final_fitness": mean(final_values),
                "std_final_fitness": stdev(final_values),
                "mean_common_fe_fitness": mean(common_values),
                "std_common_fe_fitness": stdev(common_values),
                "mean_final_fe": mean(fe_values),
                "final_fitness_wins": sum(1 for row in rows if row.get("winner_final_fitness") == source),
                "common_fe_fitness_wins": sum(1 for row in rows if row.get("winner_common_fe_fitness") == source),
                "mean_relative_fe_difference": mean(relative_values),
            }
        )
    return summary


def first_result_file(run_dir: Path) -> Path:
    matches = sorted(run_dir.glob("*.result.txt"))
    if matches:
        return matches[0]
    return run_dir / "missing.result.txt"


def compare_phase_dir(
    phase_dir: Path,
    seeds: list[int],
    fe_tolerance: int,
    candidate_dir: str = DEFAULT_CANDIDATE_NAME,
    candidate_name: str = DEFAULT_CANDIDATE_NAME,
    candidate_prefix: str = DEFAULT_CANDIDATE_PREFIX,
    include_sanitizer_metadata: bool = False,
) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for seed in seeds:
        seed_dir = phase_dir / f"seed_{seed}"
        metadata = parse_sanitizer_metadata(seed_dir) if include_sanitizer_metadata else None
        rows.append(
            compare_seed(
                seed,
                first_result_file(seed_dir / "legacy"),
                first_result_file(seed_dir / candidate_dir),
                fe_tolerance=fe_tolerance,
                candidate_name=candidate_name,
                candidate_prefix=candidate_prefix,
                metadata=metadata,
            )
        )
    return rows


def write_csv(rows: Iterable[dict[str, object]], columns: list[str], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)


def write_report(
    rows: list[dict[str, object]],
    summary: list[dict[str, object]],
    path: Path,
    *,
    title: str = "Phase 7E Multi-Seed CBOCC Smoke Comparison",
    candidate_name: str = DEFAULT_CANDIDATE_NAME,
    candidate_prefix: str = DEFAULT_CANDIDATE_PREFIX,
    include_sanitizer_metadata: bool = False,
) -> None:
    lines = [
        f"# {title}",
        "",
        "This is not a final performance benchmark.",
        f"The {candidate_name} grouping uses 10D CWVIG plus singleton completion to 905D.",
        "The original hard-overlap CBOG_CBD behavior is unchanged.",
        "Final-FE comparisons are not FE-matched when final FE differs.",
        "Common-FE comparison is more conservative because it uses the last logged row with FE <= common_fe.",
        "No optimization superiority claim should be made from this phase.",
        "",
        "## Per-Seed Results",
        "",
        "| seed | common_fe | final_fe_difference | relative_fe_difference | winner_final_fitness | winner_common_fe_fitness |",
        "| ---: | ---: | ---: | ---: | --- | --- |",
    ]
    if include_sanitizer_metadata:
        lines[4:4] = [
            "The sanitized completed-predicted grouping also uses explicit demote_min_shared repair.",
            "The sanitizer changes predicted overlap membership only to avoid zero-dimensional CMA-ES groups.",
        ]
    for row in rows:
        lines.append(
            "| {seed} | {common_fe} | {final_fe_difference} | {relative_fe_difference} | {winner_final_fitness} | {winner_common_fe_fitness} |".format(
                **row
            )
        )
    if include_sanitizer_metadata:
        lines.extend(
            [
                "",
                "## Sanitizer",
                "",
                "| seed | zero_before | zero_after | shared_before | shared_after | repairs | loader_errors | compatible_after |",
                "| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |",
            ]
        )
        for row in rows:
            lines.append(
                "| {seed} | {zero_effective_groups_before} | {zero_effective_groups_after} | {shared_before_sanitize} | {shared_after_sanitize} | {repaired_variables_or_groups_count} | {loader_validation_errors} | {hard_overlap_compatible_after} |".format(
                    **row
                )
            )
    failed = [str(row["seed"]) for row in rows if row.get("winner_final_fitness") == "unavailable"]
    if failed:
        lines.extend(["", f"Unavailable/failed seed pairs: {', '.join(failed)}."])
    lines.extend(
        [
            "",
            "## Summary",
            "",
            "| grouping_source | mean_final_fitness | mean_common_fe_fitness | mean_final_fe | final_wins | common_fe_wins |",
            "| --- | ---: | ---: | ---: | ---: | ---: |",
        ]
    )
    for row in summary:
        lines.append(
            "| {grouping_source} | {mean_final_fitness} | {mean_common_fe_fitness} | {mean_final_fe} | {final_fitness_wins} | {common_fe_fitness_wins} |".format(
                **row
            )
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase-dir", type=Path, default=Path("results/phase7e"))
    parser.add_argument("--seeds", nargs="+", type=int, default=[1, 3, 5])
    parser.add_argument("--fe-tolerance", type=int, default=0)
    parser.add_argument("--comparison-output", type=Path, default=Path("results/phase7e/multiseed_comparison.csv"))
    parser.add_argument("--summary-output", type=Path, default=Path("results/phase7e/multiseed_summary.csv"))
    parser.add_argument("--report", type=Path, default=Path("results/phase7e/phase7e_report.md"))
    parser.add_argument("--candidate-dir", default=DEFAULT_CANDIDATE_NAME)
    parser.add_argument("--candidate-name", default=DEFAULT_CANDIDATE_NAME)
    parser.add_argument("--candidate-prefix", default=DEFAULT_CANDIDATE_PREFIX)
    parser.add_argument("--include-sanitizer-metadata", action="store_true")
    parser.add_argument("--report-title", default="Phase 7E Multi-Seed CBOCC Smoke Comparison")
    args = parser.parse_args()

    rows = compare_phase_dir(
        args.phase_dir,
        args.seeds,
        args.fe_tolerance,
        candidate_dir=args.candidate_dir,
        candidate_name=args.candidate_name,
        candidate_prefix=args.candidate_prefix,
        include_sanitizer_metadata=args.include_sanitizer_metadata,
    )
    summary = summarize_pairs(rows, candidate_name=args.candidate_name, candidate_prefix=args.candidate_prefix)
    columns = comparison_columns(args.candidate_prefix, args.include_sanitizer_metadata)
    write_csv(rows, columns, args.comparison_output)
    write_csv(summary, SUMMARY_COLUMNS, args.summary_output)
    write_report(
        rows,
        summary,
        args.report,
        title=args.report_title,
        candidate_name=args.candidate_name,
        candidate_prefix=args.candidate_prefix,
        include_sanitizer_metadata=args.include_sanitizer_metadata,
    )
    print(f"wrote {args.comparison_output}")


if __name__ == "__main__":
    main()
