from pathlib import Path

from experiments.audit_cbocco_budget import fe_increment_pattern, summarize_result


def test_summarize_result_computes_fe_increment_pattern(tmp_path: Path) -> None:
    result = tmp_path / "run.result.txt"
    result.write_text("10,9.0\n20,8.0\n35,7.5\n", encoding="utf-8")

    summary = summarize_result("demo", result, maxfes_arg=1000)

    assert summary["run_label"] == "demo"
    assert summary["maxfes_arg"] == 1000
    assert summary["first_fe"] == 10
    assert summary["final_fe"] == 35
    assert summary["logged_rows"] == 3
    assert summary["fe_increment_pattern"] == "10;15"
    assert summary["final_fitness"] == 7.5


def test_fe_increment_pattern_handles_missing_or_single_row() -> None:
    assert fe_increment_pattern([]) == ""
    assert fe_increment_pattern([10]) == ""
    assert fe_increment_pattern([10, 15, 30]) == "5;15"
