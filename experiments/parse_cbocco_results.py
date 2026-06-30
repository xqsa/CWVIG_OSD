"""Parse CBOCC result files and build the Phase 7C smoke comparison CSV."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


COMPARISON_COLUMNS = [
    "run_label",
    "func",
    "method",
    "seed",
    "maxfes_arg",
    "grouping_source",
    "completion_policy",
    "expected_dimension",
    "grouping_dimension",
    "covered_variables",
    "shared_variables",
    "groups",
    "result_file",
    "first_fe",
    "first_fitness",
    "final_fe",
    "final_fitness",
    "logged_rows",
    "final_fe_difference",
    "relative_fe_difference",
    "exit_code",
]


@dataclass(frozen=True)
class ParsedResult:
    exists: bool
    non_empty: bool
    first_fe: int | None
    first_fitness: float | None
    final_fe: int | None
    final_fitness: float | None
    logged_rows: int


def parse_result_file(path: str | Path) -> ParsedResult:
    result_path = Path(path)
    if not result_path.exists():
        return ParsedResult(False, False, None, None, None, None, 0)

    rows: list[tuple[int, float]] = []
    for line_number, line in enumerate(result_path.read_text(encoding="utf-8").splitlines(), start=1):
        text = line.strip()
        if not text:
            continue
        parts = text.split(",", maxsplit=1)
        if len(parts) != 2:
            raise ValueError(f"Malformed result row in {result_path} line {line_number}: {line}")
        rows.append((int(parts[0]), float(parts[1])))

    if not rows:
        return ParsedResult(True, False, None, None, None, None, 0)

    first_fe, first_fitness = rows[0]
    final_fe, final_fitness = rows[-1]
    return ParsedResult(True, True, first_fe, first_fitness, final_fe, final_fitness, len(rows))


def parse_startup_log(path: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    if not path.exists():
        return fields
    for line in path.read_text(encoding="utf-8").splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", maxsplit=1)
        fields[key.strip()] = value.strip()
    return fields


def read_exit_code(path: Path) -> int | None:
    if not path.exists():
        return None
    text = path.read_text(encoding="utf-8").strip()
    return int(text) if text else None


def first_result_file(run_dir: Path) -> Path:
    matches = sorted(run_dir.glob("*.result.txt"))
    if matches:
        return matches[0]
    return run_dir / "missing.result.txt"


def phase7c_rows(phase_dir: Path) -> list[dict[str, object]]:
    runs = [
        ("legacy", phase_dir / "legacy", "none"),
        ("completed_predicted", phase_dir / "completed_predicted", "singletons"),
    ]
    rows: list[dict[str, object]] = []
    for label, run_dir, completion_policy in runs:
        startup = parse_startup_log(run_dir / "cbocco_stdout.txt")
        result_file = first_result_file(run_dir)
        parsed = parse_result_file(result_file)
        rows.append(
            {
                "run_label": label,
                "func": startup.get("Function ID", ""),
                "method": startup.get("Method", ""),
                "seed": startup.get("Seed", ""),
                "maxfes_arg": startup.get("MaxFEs argument", ""),
                "grouping_source": startup.get("Grouping Source", ""),
                "completion_policy": completion_policy,
                "expected_dimension": startup.get("Expected Benchmark Dimension", ""),
                "grouping_dimension": startup.get("Grouping Inferred Dimension", ""),
                "covered_variables": startup.get("Covered Unique Variables", ""),
                "shared_variables": startup.get("Shared Variables", ""),
                "groups": startup.get("Number Of Groups", ""),
                "result_file": str(result_file),
                "first_fe": parsed.first_fe if parsed.first_fe is not None else "",
                "first_fitness": parsed.first_fitness if parsed.first_fitness is not None else "",
                "final_fe": parsed.final_fe if parsed.final_fe is not None else "",
                "final_fitness": parsed.final_fitness if parsed.final_fitness is not None else "",
                "logged_rows": parsed.logged_rows,
                "exit_code": read_exit_code(run_dir / "exit_code.txt"),
            }
        )
    return rows


def add_fe_difference_columns(
    rows: Iterable[dict[str, object]],
    *,
    tolerance: int,
) -> tuple[list[dict[str, object]], str]:
    materialized = [dict(row) for row in rows]
    if not materialized:
        return materialized, ""

    try:
        baseline = int(materialized[0].get("final_fe"))
    except (TypeError, ValueError):
        baseline = 0

    max_difference = 0
    for row in materialized:
        try:
            final_fe = int(row.get("final_fe"))
        except (TypeError, ValueError):
            difference: int | str = ""
            relative: float | str = ""
        else:
            difference = final_fe - baseline
            relative = 0.0 if baseline == 0 else difference / baseline
            max_difference = max(max_difference, abs(difference))
        row["final_fe_difference"] = difference
        row["relative_fe_difference"] = relative

    if max_difference > tolerance:
        return materialized, f"WARNING: comparison is not FE-matched; max final FE difference is {max_difference}."
    return materialized, ""


def write_comparison_csv(rows: Iterable[dict[str, object]], output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=COMPARISON_COLUMNS)
        writer.writeheader()
        writer.writerows(rows)


def write_smoke_report(rows: Iterable[dict[str, object]], output_path: Path, warning: str = "") -> None:
    materialized = list(rows)
    lines = [
        "# Phase 7C CBOCC Grouping Smoke Comparison",
        "",
        "Both runs use the original hard-overlap CBOG_CBD behavior.",
        "The completed-predicted grouping is not a fair full CWVIG result.",
        "This is only a grouping-source integration smoke comparison.",
        "Any fitness difference is not a performance claim.",
        "",
    ]
    if warning:
        lines.extend([warning, ""])
    lines.extend(
        [
            "## Results",
            "",
            "| run_label | maxfes_arg | final_fe | final_fitness | logged_rows | exit_code |",
            "| --- | ---: | ---: | ---: | ---: | ---: |",
        ]
    )
    for row in materialized:
        lines.append(
            "| {run_label} | {maxfes_arg} | {final_fe} | {final_fitness} | {logged_rows} | {exit_code} |".format(
                run_label=row.get("run_label", ""),
                maxfes_arg=row.get("maxfes_arg", ""),
                final_fe=row.get("final_fe", ""),
                final_fitness=row.get("final_fitness", ""),
                logged_rows=row.get("logged_rows", ""),
                exit_code=row.get("exit_code", ""),
            )
        )

    lines.extend(["", "## MaxFEs Observation", ""])
    for row in materialized:
        lines.append(
            "- `{run_label}` used maxfes argument `{maxfes_arg}` and ended at final FE `{final_fe}`.".format(
                run_label=row.get("run_label", ""),
                maxfes_arg=row.get("maxfes_arg", ""),
                final_fe=row.get("final_fe", ""),
            )
        )
    lines.append(
        "- The observed final FE can exceed the command-line maxfes argument because the original CBOG_CBD `testStage()` consumes evaluations before the optimization-stage limit check."
    )
    lines.append("- This phase records that behavior and does not change it.")
    if warning:
        lines.append("- Because this comparison is not FE-matched, interpret fitness differences only as smoke output.")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase-dir", type=Path, default=Path("results/phase7c"))
    parser.add_argument("--output", type=Path, default=Path("results/phase7c/comparison.csv"))
    parser.add_argument("--report", type=Path)
    parser.add_argument("--fe-tolerance", type=int, default=0)
    args = parser.parse_args()

    rows, warning = add_fe_difference_columns(phase7c_rows(args.phase_dir), tolerance=args.fe_tolerance)
    write_comparison_csv(rows, args.output)
    if args.report:
        write_smoke_report(rows, args.report, warning)
    if warning:
        print(warning)
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
