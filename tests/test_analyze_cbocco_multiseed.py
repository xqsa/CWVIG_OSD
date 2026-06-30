from pathlib import Path

from experiments.analyze_cbocco_multiseed import (
    compare_seed,
    fitness_at_or_before,
    parse_sanitizer_metadata,
    read_curve,
    summarize_pairs,
)


def write_result(path: Path, rows: list[tuple[int, float]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(f"{fe},{fitness}" for fe, fitness in rows) + "\n", encoding="utf-8")


def test_common_fe_lookup_uses_last_logged_row_at_or_before_target(tmp_path: Path) -> None:
    result = tmp_path / "run.result.txt"
    write_result(result, [(10, 5.0), (20, 4.0), (40, 3.0)])

    curve = read_curve(result)

    assert fitness_at_or_before(curve, 25) == 4.0
    assert fitness_at_or_before(curve, 5) is None


def test_compare_seed_when_final_fes_differ(tmp_path: Path) -> None:
    legacy = tmp_path / "legacy.result.txt"
    completed = tmp_path / "completed.result.txt"
    write_result(legacy, [(10, 9.0), (20, 7.0)])
    write_result(completed, [(12, 8.0), (24, 6.0)])

    row = compare_seed(3, legacy, completed, fe_tolerance=0)

    assert row["seed"] == 3
    assert row["common_fe"] == 20
    assert row["legacy_fitness_at_common_fe"] == 7.0
    assert row["completed_predicted_fitness_at_common_fe"] == 8.0
    assert row["final_fe_difference"] == 4
    assert row["relative_fe_difference"] == 0.2
    assert row["fe_matched"] == "false"
    assert row["winner_final_fitness"] == "completed_predicted"
    assert row["winner_common_fe_fitness"] == "legacy"


def test_compare_seed_handles_missing_or_empty_result_file(tmp_path: Path) -> None:
    legacy = tmp_path / "legacy.result.txt"
    completed = tmp_path / "completed.result.txt"
    legacy.write_text("", encoding="utf-8")

    row = compare_seed(1, legacy, completed, fe_tolerance=0)

    assert row["common_fe"] == ""
    assert row["winner_final_fitness"] == "unavailable"
    assert row["winner_common_fe_fitness"] == "unavailable"


def test_summarize_pairs_aggregates_multiple_seeds() -> None:
    rows = [
        {
            "winner_final_fitness": "legacy",
            "winner_common_fe_fitness": "legacy",
            "final_fitness_legacy": 10.0,
            "final_fitness_completed_predicted": 12.0,
            "legacy_fitness_at_common_fe": 10.0,
            "completed_predicted_fitness_at_common_fe": 11.0,
            "final_fe_legacy": 100,
            "final_fe_completed_predicted": 120,
            "relative_fe_difference": 0.2,
        },
        {
            "winner_final_fitness": "completed_predicted",
            "winner_common_fe_fitness": "completed_predicted",
            "final_fitness_legacy": 20.0,
            "final_fitness_completed_predicted": 9.0,
            "legacy_fitness_at_common_fe": 20.0,
            "completed_predicted_fitness_at_common_fe": 8.0,
            "final_fe_legacy": 100,
            "final_fe_completed_predicted": 120,
            "relative_fe_difference": 0.2,
        },
    ]

    summary = summarize_pairs(rows)

    by_source = {row["grouping_source"]: row for row in summary}
    assert by_source["legacy"]["mean_final_fitness"] == 15.0
    assert by_source["completed_predicted"]["mean_common_fe_fitness"] == 9.5
    assert by_source["legacy"]["final_fitness_wins"] == 1
    assert by_source["completed_predicted"]["common_fe_fitness_wins"] == 1
    assert by_source["legacy"]["mean_relative_fe_difference"] == 0.2


def test_parse_sanitizer_metadata_reads_before_after_and_repair_count(tmp_path: Path) -> None:
    seed_dir = tmp_path / "seed_3"
    audit = seed_dir / "audit"
    grouping = seed_dir / "grouping"
    audit.mkdir(parents=True)
    grouping.mkdir()
    (audit / "sanitize_summary.txt").write_text(
        "\n".join(
            [
                "Repair Policy: demote_min_shared",
                "Before Repair",
                "Hard-Overlap Shared Variables: 5",
                "Zero Effective Group Count: 3",
                "After Repair",
                "Hard-Overlap Shared Variables: 3",
                "Zero Effective Group Count: 0",
                "Hard-Overlap Compatible: true",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    (audit / "sanitizer_report.csv").write_text(
        "policy,group_index,variable,detail\n"
        "demote_min_shared,2,0,\"removed variable from 5 overlap rows\"\n",
        encoding="utf-8",
    )
    (grouping / "sanitized_loader_summary.txt").write_text("Validation Errors: 0\n", encoding="utf-8")

    metadata = parse_sanitizer_metadata(seed_dir)

    assert metadata["repair_policy"] == "demote_min_shared"
    assert metadata["shared_before_sanitize"] == 5
    assert metadata["shared_after_sanitize"] == 3
    assert metadata["zero_effective_groups_before"] == 3
    assert metadata["zero_effective_groups_after"] == 0
    assert metadata["repaired_variables_or_groups_count"] == 1
    assert metadata["loader_validation_errors"] == 0
    assert metadata["hard_overlap_compatible_after"] == "true"


def test_compare_seed_supports_sanitized_candidate_and_metadata(tmp_path: Path) -> None:
    legacy = tmp_path / "legacy.result.txt"
    sanitized = tmp_path / "sanitized.result.txt"
    write_result(legacy, [(100, 5.0), (200, 4.0)])
    write_result(sanitized, [(120, 6.0), (240, 3.0)])

    row = compare_seed(
        5,
        legacy,
        sanitized,
        fe_tolerance=0,
        candidate_name="sanitized_completed_predicted",
        candidate_prefix="sanitized",
        metadata={
            "shared_before_sanitize": 5,
            "shared_after_sanitize": 3,
            "zero_effective_groups_before": 2,
            "zero_effective_groups_after": 0,
            "repair_policy": "demote_min_shared",
            "repaired_variables_or_groups_count": 2,
            "loader_validation_errors": 0,
            "hard_overlap_compatible_after": "true",
            "legacy_hard_overlap_compatible": "true",
            "sanitized_hard_overlap_compatible": "true",
        },
    )

    assert row["final_fe_sanitized"] == 240
    assert row["final_fitness_sanitized"] == 3.0
    assert row["sanitized_fitness_at_common_fe"] == 6.0
    assert row["winner_final_fitness"] == "sanitized_completed_predicted"
    assert row["both_result_files_present"] == "true"
    assert row["both_hard_overlap_compatible"] == "true"
    assert row["zero_effective_groups_after"] == 0


def test_summarize_pairs_accepts_sanitized_source() -> None:
    rows = [
        {
            "winner_final_fitness": "sanitized_completed_predicted",
            "winner_common_fe_fitness": "legacy",
            "final_fitness_legacy": 8.0,
            "final_fitness_sanitized": 6.0,
            "legacy_fitness_at_common_fe": 8.0,
            "sanitized_fitness_at_common_fe": 9.0,
            "final_fe_legacy": 100,
            "final_fe_sanitized": 120,
            "relative_fe_difference": 0.2,
        }
    ]

    summary = summarize_pairs(rows, candidate_name="sanitized_completed_predicted", candidate_prefix="sanitized")

    by_source = {row["grouping_source"]: row for row in summary}
    assert by_source["sanitized_completed_predicted"]["mean_final_fitness"] == 6.0
    assert by_source["sanitized_completed_predicted"]["final_fitness_wins"] == 1
