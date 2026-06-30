from pathlib import Path

from experiments.analyze_cbocco_multiseed import (
    compare_seed,
    fitness_at_or_before,
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
