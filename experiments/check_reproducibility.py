import argparse
import shutil
import subprocess
from pathlib import Path


def run(command, cwd):
    completed = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed: {' '.join(str(part) for part in command)}\n{completed.stderr}"
        )
    return completed.stdout


def read_bytes(path):
    return Path(path).read_bytes()


def main():
    parser = argparse.ArgumentParser(description="Check frozen CWVIG preset reproducibility.")
    parser.add_argument("--output-root", default="results/repro_checks")
    parser.add_argument("--config", default="configs/cwvig_presets/capped_default.json")
    parser.add_argument("--pipeline-exe", default="build/flyki/Release/cwvig_grouping_pipeline_cli.exe")
    parser.add_argument("--loader-exe", default="build/flyki/Release/grouping_source_cli.exe")
    parser.add_argument("--clean", action="store_true")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    output_root = (repo_root / args.output_root).resolve()
    config = (repo_root / args.config).resolve()
    pipeline_exe = (repo_root / args.pipeline_exe).resolve()
    loader_exe = (repo_root / args.loader_exe).resolve()

    if args.clean and output_root.exists():
        shutil.rmtree(output_root)
    output_a = output_root / "fixed_a"
    output_b = output_root / "fixed_b"
    output_a.mkdir(parents=True, exist_ok=True)
    output_b.mkdir(parents=True, exist_ok=True)

    base_command = [
        str(pipeline_exe),
        "--config",
        str(config),
        "--mode",
        "flyki",
        "--func",
        "1",
        "--dimension-limit",
        "10",
        "--seed",
        "1",
        "--contexts",
        "3",
        "--output-dir",
    ]
    run(base_command + [str(output_a), "--print-summary"], repo_root)
    run(base_command + [str(output_b), "--print-summary"], repo_root)

    for name in ["predicted_groups.txt", "predicted_overlap.txt"]:
        if read_bytes(output_a / name) != read_bytes(output_b / name):
            raise AssertionError(f"non-deterministic output: {name}")

    loader_out = run([
        str(loader_exe),
        "--source",
        "explicit_files",
        "--po",
        str(output_a / "predicted_groups.txt"),
        "--oo",
        str(output_a / "predicted_overlap.txt"),
        "--print-summary",
    ], repo_root)
    if "Validation Errors: 0" not in loader_out:
        raise AssertionError(loader_out)

    for truth_only_file in ["edge_metrics.csv", "grouping_metrics.csv"]:
        if (output_a / truth_only_file).exists():
            raise AssertionError(f"truth-only output exists without true po/oo: {truth_only_file}")

    print(f"deterministic_groups={output_a / 'predicted_groups.txt'}")
    print(f"deterministic_overlap={output_a / 'predicted_overlap.txt'}")
    print("loader_validation=0")
    print("truth_files_used_in_unsupervised_path=false")


if __name__ == "__main__":
    main()
