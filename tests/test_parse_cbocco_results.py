from pathlib import Path

from experiments.parse_cbocco_results import parse_result_file, write_smoke_report


def test_parse_result_file_reads_first_and_final_rows(tmp_path: Path) -> None:
    result = tmp_path / "run.result.txt"
    result.write_text("10,3.5\n20,2.25\n", encoding="utf-8")

    parsed = parse_result_file(result)

    assert parsed.exists
    assert parsed.non_empty
    assert parsed.logged_rows == 2
    assert parsed.first_fe == 10
    assert parsed.first_fitness == 3.5
    assert parsed.final_fe == 20
    assert parsed.final_fitness == 2.25


def test_parse_result_file_reports_missing_file(tmp_path: Path) -> None:
    parsed = parse_result_file(tmp_path / "missing.result.txt")

    assert not parsed.exists
    assert not parsed.non_empty
    assert parsed.logged_rows == 0
    assert parsed.final_fe is None


def test_write_smoke_report_includes_required_caveats(tmp_path: Path) -> None:
    report = tmp_path / "report.md"
    rows = [
        {
            "run_label": "legacy",
            "maxfes_arg": "1000",
            "final_fe": 162000,
            "final_fitness": 1.2,
        },
        {
            "run_label": "completed_predicted",
            "maxfes_arg": "1000",
            "final_fe": 181600,
            "final_fitness": 2.3,
        },
    ]

    write_smoke_report(rows, report)

    text = report.read_text(encoding="utf-8")
    assert "original hard-overlap CBOG_CBD" in text
    assert "not a fair full CWVIG result" in text
    assert "not a performance claim" in text
    assert "162000" in text
