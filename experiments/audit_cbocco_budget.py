"""Audit CBOCC result FE accounting without changing optimizer behavior."""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from experiments.parse_cbocco_results import parse_result_file, parse_startup_log


AUDIT_COLUMNS = [
    "run_label",
    "maxfes_arg",
    "first_fe",
    "final_fe",
    "logged_rows",
    "fe_increment_pattern",
    "final_fitness",
    "result_file",
    "grouping_source",
    "groups",
    "shared_variables",
    "exit_code",
]


def read_fe_values(path: Path) -> list[int]:
    if not path.exists():
        return []
    values: list[int] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        text = line.strip()
        if not text:
            continue
        values.append(int(text.split(",", maxsplit=1)[0]))
    return values


def fe_increment_pattern(fe_values: list[int]) -> str:
    increments = [right - left for left, right in zip(fe_values, fe_values[1:])]
    if not increments:
        return ""
    if len(set(increments)) == 1:
        return f"{increments[0]}x{len(increments)}"
    return ";".join(str(value) for value in increments)


def read_exit_code(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8").strip()


def summarize_result(
    run_label: str,
    result_file: str | Path,
    *,
    maxfes_arg: int | str = "",
    stdout_file: str | Path | None = None,
    exit_code_file: str | Path | None = None,
) -> dict[str, object]:
    path = Path(result_file)
    parsed = parse_result_file(path)
    startup = parse_startup_log(Path(stdout_file)) if stdout_file else {}
    fe_values = read_fe_values(path)
    return {
        "run_label": run_label,
        "maxfes_arg": maxfes_arg or startup.get("MaxFEs argument", ""),
        "first_fe": parsed.first_fe if parsed.first_fe is not None else "",
        "final_fe": parsed.final_fe if parsed.final_fe is not None else "",
        "logged_rows": parsed.logged_rows,
        "fe_increment_pattern": fe_increment_pattern(fe_values),
        "final_fitness": parsed.final_fitness if parsed.final_fitness is not None else "",
        "result_file": str(path),
        "grouping_source": startup.get("Grouping Source", ""),
        "groups": startup.get("Number Of Groups", ""),
        "shared_variables": startup.get("Shared Variables", ""),
        "exit_code": read_exit_code(Path(exit_code_file)) if exit_code_file else "",
    }


def write_csv(rows: Iterable[dict[str, object]], output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=AUDIT_COLUMNS)
        writer.writeheader()
        writer.writerows(rows)


def write_report(rows: list[dict[str, object]], output_path: Path) -> None:
    lines = [
        "# Phase 7D CBOCC FE Budget Audit",
        "",
        "This audit is diagnostic only. It does not change CBOG_CBD, CMAESO, benchmark functions, or legacy CBCCO behavior.",
        "",
        "## FE Summaries",
        "",
        "| run_label | maxfes_arg | groups | shared_variables | first_fe | final_fe | logged_rows | final_fitness |",
        "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    for row in rows:
        lines.append(
            "| {run_label} | {maxfes_arg} | {groups} | {shared_variables} | {first_fe} | {final_fe} | {logged_rows} | {final_fitness} |".format(
                **row
            )
        )
    lines.extend(
        [
            "",
            "## Interpretation",
            "",
            "- The command-line `maxfes` is passed to `CBOG_CBD` as `MAXFES`.",
            "- `CBOG_CBD::testStage()` runs before `optimizationStage()` checks `usedFEs < MAXFES`.",
            "- Each `CMAESO::calculateFitness()` call increments `usedFEs` once per CMA-ES population member.",
            "- More groups and different group dimensions can change the FE consumed before the maxfes gate is reached.",
            "- Runs with different final FE are not FE-matched; fitness differences are not optimization improvement claims.",
        ]
    )
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="append", nargs=5, metavar=("LABEL", "RESULT", "MAXFES", "STDOUT", "EXIT_CODE"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()

    rows = [
        summarize_result(label, result, maxfes_arg=maxfes, stdout_file=stdout, exit_code_file=exit_code)
        for label, result, maxfes, stdout, exit_code in (args.run or [])
    ]
    write_csv(rows, args.output)
    write_report(rows, args.report)
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
