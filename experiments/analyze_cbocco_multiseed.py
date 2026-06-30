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


COMPARISON_COLUMNS = [
    "seed",
    "final_fe_legacy",
    "final_fitness_legacy",
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


def compare_seed(
    seed: int,
    legacy_result: str | Path,
    completed_result: str | Path,
    *,
    fe_tolerance: int,
) -> dict[str, object]:
    legacy_curve = read_curve(legacy_result)
    completed_curve = read_curve(completed_result)
    final_fe_legacy: int | str = legacy_curve[-1][0] if legacy_curve else ""
    final_fitness_legacy: float | str = legacy_curve[-1][1] if legacy_curve else ""
    final_fe_completed: int | str = completed_curve[-1][0] if completed_curve else ""
    final_fitness_completed: float | str = completed_curve[-1][1] if completed_curve else ""
    if not legacy_curve or not completed_curve:
        return {
            "seed": seed,
            "final_fe_legacy": final_fe_legacy,
            "final_fitness_legacy": final_fitness_legacy,
            "final_fe_completed_predicted": final_fe_completed,
            "final_fitness_completed_predicted": final_fitness_completed,
            "common_fe": "",
            "legacy_fitness_at_common_fe": "",
            "completed_predicted_fitness_at_common_fe": "",
            "final_fe_difference": "",
            "relative_fe_difference": "",
            "fe_matched": "false",
            "winner_final_fitness": "unavailable",
            "winner_common_fe_fitness": "unavailable",
            "legacy_result_file": str(legacy_result),
            "completed_predicted_result_file": str(completed_result),
        }

    common_fe = min(final_fe_legacy, final_fe_completed)
    legacy_common = fitness_at_or_before(legacy_curve, common_fe)
    completed_common = fitness_at_or_before(completed_curve, common_fe)
    difference = final_fe_completed - final_fe_legacy
    relative = 0.0 if final_fe_legacy == 0 else difference / final_fe_legacy
    return {
        "seed": seed,
        "final_fe_legacy": final_fe_legacy,
        "final_fitness_legacy": final_fitness_legacy,
        "final_fe_completed_predicted": final_fe_completed,
        "final_fitness_completed_predicted": final_fitness_completed,
        "common_fe": common_fe,
        "legacy_fitness_at_common_fe": legacy_common if legacy_common is not None else "",
        "completed_predicted_fitness_at_common_fe": completed_common if completed_common is not None else "",
        "final_fe_difference": difference,
        "relative_fe_difference": relative,
        "fe_matched": "true" if abs(difference) <= fe_tolerance else "false",
        "winner_final_fitness": winner(
            "legacy",
            final_fitness_legacy,
            "completed_predicted",
            final_fitness_completed,
        ),
        "winner_common_fe_fitness": winner("legacy", legacy_common, "completed_predicted", completed_common),
        "legacy_result_file": str(legacy_result),
        "completed_predicted_result_file": str(completed_result),
    }


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


def summarize_pairs(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    sources = [
        (
            "legacy",
            "final_fitness_legacy",
            "legacy_fitness_at_common_fe",
            "final_fe_legacy",
        ),
        (
            "completed_predicted",
            "final_fitness_completed_predicted",
            "completed_predicted_fitness_at_common_fe",
            "final_fe_completed_predicted",
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


def compare_phase_dir(phase_dir: Path, seeds: list[int], fe_tolerance: int) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for seed in seeds:
        seed_dir = phase_dir / f"seed_{seed}"
        rows.append(
            compare_seed(
                seed,
                first_result_file(seed_dir / "legacy"),
                first_result_file(seed_dir / "completed_predicted"),
                fe_tolerance=fe_tolerance,
            )
        )
    return rows


def write_csv(rows: Iterable[dict[str, object]], columns: list[str], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)


def write_report(rows: list[dict[str, object]], summary: list[dict[str, object]], path: Path) -> None:
    lines = [
        "# Phase 7E Multi-Seed CBOCC Smoke Comparison",
        "",
        "This is not a final performance benchmark.",
        "The completed_predicted grouping uses 10D CWVIG plus singleton completion to 905D.",
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
    for row in rows:
        lines.append(
            "| {seed} | {common_fe} | {final_fe_difference} | {relative_fe_difference} | {winner_final_fitness} | {winner_common_fe_fitness} |".format(
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
    args = parser.parse_args()

    rows = compare_phase_dir(args.phase_dir, args.seeds, args.fe_tolerance)
    summary = summarize_pairs(rows)
    write_csv(rows, COMPARISON_COLUMNS, args.comparison_output)
    write_csv(summary, SUMMARY_COLUMNS, args.summary_output)
    write_report(rows, summary, args.report)
    print(f"wrote {args.comparison_output}")


if __name__ == "__main__":
    main()
