from pathlib import Path

from experiments.analyze_cbocco_coverage_scaling import (
    analyze_phase_dir,
    summarize_by_dimension,
    summarize_by_seed,
    write_report,
)


def write_result(path: Path, rows: list[tuple[int, float]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(f"{fe},{fitness}" for fe, fitness in rows) + "\n", encoding="utf-8")


def write_counted_groups(path: Path, groups: list[list[int]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    for group in groups:
        lines.append(str(len(group)))
        lines.append(" ".join(str(v) for v in group))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_successful_run(root: Path) -> None:
    seed_dir = root / "seed_1"
    write_result(seed_dir / "legacy" / "legacy.result.txt", [(100, 10.0), (200, 8.0)])
    run_dir = seed_dir / "dim_3"
    write_counted_groups(run_dir / "predicted" / "predicted_groups.txt", [[0, 1], [1, 2]])
    write_counted_groups(run_dir / "completed_groups.txt", [[0, 1], [1, 2], [3], [4]])
    (run_dir / "hard_overlap_before.txt").write_text(
        "Hard-Overlap Shared Variables: 2\nZero Effective Group Count: 1\n",
        encoding="utf-8",
    )
    (run_dir / "sanitize_summary.txt").write_text(
        "\n".join(
            [
                "Repair Policy: demote_min_shared",
                "Before Repair",
                "Hard-Overlap Shared Variables: 2",
                "Zero Effective Group Count: 1",
                "After Repair",
                "Hard-Overlap Shared Variables: 1",
                "Zero Effective Group Count: 0",
                "Hard-Overlap Compatible: true",
                "Repair Actions: 1",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    (run_dir / "sanitizer_report.csv").write_text(
        "policy,group_index,variable,detail\n"
        "demote_min_shared,1,1,\"removed variable from 2 overlap rows\"\n",
        encoding="utf-8",
    )
    (run_dir / "sanitized_loader_summary.txt").write_text("Validation Errors: 0\n", encoding="utf-8")
    (run_dir / "sanitized_cbocco" / "exit_code.txt").parent.mkdir(parents=True, exist_ok=True)
    (run_dir / "sanitized_cbocco" / "exit_code.txt").write_text("0\n", encoding="utf-8")
    write_result(run_dir / "sanitized_cbocco" / "run.result.txt", [(120, 9.0), (240, 6.0)])


def test_analyze_phase_dir_computes_coverage_completion_and_common_fe(tmp_path: Path) -> None:
    write_successful_run(tmp_path)

    rows = analyze_phase_dir(tmp_path, seeds=[1], dimension_limits=[3], expected_dimension=5)

    row = rows[0]
    assert row["cwvig_coverage_ratio"] == 0.6
    assert row["completion_added_variables"] == 2
    assert row["completion_added_ratio"] == 0.4
    assert row["predicted_group_count_before_completion"] == 2
    assert row["group_count_after_completion"] == 4
    assert row["shared_before_sanitize"] == 2
    assert row["shared_after_sanitize"] == 1
    assert row["zero_effective_groups_before"] == 1
    assert row["zero_effective_groups_after"] == 0
    assert row["sanitizer_repair_actions"] == 1
    assert row["loader_validation_errors"] == 0
    assert row["hard_overlap_compatible_after"] == "true"
    assert row["cbocco_exit_code"] == 0
    assert row["result_file_exists"] == "true"
    assert row["common_fe_vs_legacy"] == 200
    assert row["fitness_at_common_fe"] == 9.0
    assert row["legacy_fitness_at_common_fe"] == 8.0
    assert row["relative_fe_difference"] == 0.2


def test_analyze_phase_dir_reports_missing_result_file(tmp_path: Path) -> None:
    write_successful_run(tmp_path)
    result = tmp_path / "seed_1" / "dim_3" / "sanitized_cbocco" / "run.result.txt"
    result.unlink()
    (tmp_path / "seed_1" / "dim_3" / "sanitized_cbocco" / "exit_code.txt").write_text("1\n", encoding="utf-8")

    row = analyze_phase_dir(tmp_path, seeds=[1], dimension_limits=[3], expected_dimension=5)[0]

    assert row["cbocco_exit_code"] == 1
    assert row["result_file_exists"] == "false"
    assert row["final_fe"] == ""
    assert row["common_fe_vs_legacy"] == ""


def test_dimension_and_seed_summaries_aggregate_rows(tmp_path: Path) -> None:
    write_successful_run(tmp_path)
    rows = analyze_phase_dir(tmp_path, seeds=[1], dimension_limits=[3], expected_dimension=5)

    by_dim = summarize_by_dimension(rows)
    by_seed = summarize_by_seed(rows)

    assert by_dim[0]["dimension_limit"] == 3
    assert by_dim[0]["successful_results"] == 1
    assert by_dim[0]["mean_completion_added_ratio"] == 0.4
    assert by_seed[0]["seed"] == 1
    assert by_seed[0]["successful_results"] == 1


def test_report_generation_mentions_smoke_caveats(tmp_path: Path) -> None:
    write_successful_run(tmp_path)
    rows = analyze_phase_dir(tmp_path, seeds=[1], dimension_limits=[3], expected_dimension=5)
    report = tmp_path / "report.md"

    write_report(rows, summarize_by_dimension(rows), summarize_by_seed(rows), report)

    text = report.read_text(encoding="utf-8")
    assert "coverage-scaling smoke" in text
    assert "no final optimization superiority claim" in text
