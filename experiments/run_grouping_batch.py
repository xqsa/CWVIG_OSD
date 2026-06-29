import argparse
import shutil
import subprocess
from pathlib import Path

from analyze_grouping_batch import analyze


PRESETS = ["conservative", "balanced", "capped"]


def parse_int_list(text):
    return [int(item) for item in text.split(",") if item.strip()]


def mode_defaults(mode):
    if mode == "smoke":
        return {
            "functions": [1],
            "dimension_limits": [10],
            "seeds": [1, 3],
            "contexts": [3],
            "presets": PRESETS,
        }
    if mode == "medium":
        return {
            "functions": [1],
            "dimension_limits": [10, 20, 50],
            "seeds": [1, 3, 5, 11],
            "contexts": [3, 5],
            "presets": PRESETS,
        }
    return {
        "functions": [1],
        "dimension_limits": [10],
        "seeds": [1],
        "contexts": [3],
        "presets": PRESETS,
    }


def truth_paths(benchmark_root, func):
    po = benchmark_root / f"{func}po.txt"
    oo = benchmark_root / f"{func}oo.txt"
    return (po, oo) if po.exists() and oo.exists() else (None, None)


def run_name(func, dimension, seed, contexts, preset):
    return f"f{func}_d{dimension}_s{seed}_c{contexts}_{preset}"


def run_one(args, func, dimension, seed, contexts, preset, output_dir):
    po, oo = truth_paths(args.benchmark_root, func)
    command = [
        str(args.pipeline_exe),
        "--mode",
        "flyki",
        "--func",
        str(func),
        "--dimension-limit",
        str(dimension),
        "--contexts",
        str(contexts),
        "--seed",
        str(seed),
        "--delta",
        str(args.delta),
        "--preset",
        preset,
        "--output-dir",
        str(output_dir),
    ]
    if po is not None and oo is not None:
        command.extend(["--true-po", str(po), "--true-oo", str(oo)])
    command.append("--print-summary")
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "command.txt").write_text(" ".join(command) + "\n", encoding="utf-8")
    completed = subprocess.run(command, cwd=args.repo_root, text=True, capture_output=True)
    (output_dir / "stdout.txt").write_text(completed.stdout, encoding="utf-8")
    (output_dir / "stderr.txt").write_text(completed.stderr, encoding="utf-8")
    if completed.returncode != 0:
        raise RuntimeError(f"pipeline failed for {output_dir}: {completed.stderr.strip()}")


def planned_runs(args):
    runs = []
    for func in args.functions:
        for dimension in args.dimension_limits:
            for seed in args.seeds:
                for contexts in args.contexts:
                    for preset in args.presets:
                        runs.append((func, dimension, seed, contexts, preset))
    return runs


def write_plan(args, runs, selected_runs, dry_run):
    args.output_root.mkdir(parents=True, exist_ok=True)
    path = args.output_root / "batch_plan.txt"
    with path.open("w", encoding="utf-8") as output:
        output.write(f"mode={args.mode}\n")
        output.write(f"functions={','.join(str(item) for item in args.functions)}\n")
        output.write(f"dimension_limits={','.join(str(item) for item in args.dimension_limits)}\n")
        output.write(f"seeds={','.join(str(item) for item in args.seeds)}\n")
        output.write(f"contexts={','.join(str(item) for item in args.contexts)}\n")
        output.write(f"presets={','.join(args.presets)}\n")
        output.write(f"delta={args.delta}\n")
        output.write(f"planned_runs={len(runs)}\n")
        output.write(f"selected_runs={len(selected_runs)}\n")
        output.write(f"dry_run={'true' if dry_run else 'false'}\n")
        for func, dimension, seed, contexts, preset in selected_runs:
            output.write(f"{run_name(func, dimension, seed, contexts, preset)}\n")
    return path


def resolved_args():
    parser = argparse.ArgumentParser(description="Run CWVIG grouping pipeline batch experiments.")
    parser.add_argument("--mode", choices=["smoke", "medium", "custom"], default="smoke")
    parser.add_argument("--functions")
    parser.add_argument("--dimension-limits")
    parser.add_argument("--seeds")
    parser.add_argument("--contexts")
    parser.add_argument("--presets")
    parser.add_argument("--delta", type=float, default=0.0001)
    parser.add_argument("--output-root", default="results/grouping_batch")
    parser.add_argument("--pipeline-exe", default="build/flyki/Release/cwvig_grouping_pipeline_cli.exe")
    parser.add_argument("--benchmark-root", default="benchmark/flyki_overlap")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--max-runs", type=int, default=0)
    args = parser.parse_args()

    args.repo_root = Path(__file__).resolve().parents[1]
    args.output_root = (args.repo_root / args.output_root).resolve()
    args.pipeline_exe = (args.repo_root / args.pipeline_exe).resolve()
    args.benchmark_root = (args.repo_root / args.benchmark_root).resolve()

    defaults = mode_defaults(args.mode)
    args.functions = parse_int_list(args.functions) if args.functions else defaults["functions"]
    args.dimension_limits = parse_int_list(args.dimension_limits) if args.dimension_limits else defaults["dimension_limits"]
    args.seeds = parse_int_list(args.seeds) if args.seeds else defaults["seeds"]
    args.contexts = parse_int_list(args.contexts) if args.contexts else defaults["contexts"]
    args.presets = [item for item in args.presets.split(",") if item.strip()] if args.presets else defaults["presets"]
    if not args.pipeline_exe.exists():
        raise FileNotFoundError(f"pipeline executable not found: {args.pipeline_exe}")
    return args


def main():
    args = resolved_args()
    if args.clean and args.output_root.exists():
        shutil.rmtree(args.output_root)
    runs_root = args.output_root / "runs"
    runs_root.mkdir(parents=True, exist_ok=True)
    runs = planned_runs(args)
    selected_runs = runs[: args.max_runs] if args.max_runs > 0 else runs
    plan_path = write_plan(args, runs, selected_runs, args.dry_run)
    if args.dry_run:
        for func, dimension, seed, contexts, preset in selected_runs:
            print(run_name(func, dimension, seed, contexts, preset))
        print(f"planned_runs={len(runs)}")
        print(f"selected_runs={len(selected_runs)}")
        print(f"batch_plan={plan_path}")
        return

    total = 0
    for func, dimension, seed, contexts, preset in selected_runs:
        output_dir = runs_root / run_name(func, dimension, seed, contexts, preset)
        print(f"running {output_dir.relative_to(args.repo_root)}")
        run_one(args, func, dimension, seed, contexts, preset, output_dir)
        total += 1

    summary_path, preset_path, by_dimension_path, by_context_path, by_seed_path, report_path = analyze(args.output_root)
    print(f"runs={total}")
    print(f"batch_plan={plan_path}")
    print(f"summary={summary_path}")
    print(f"preset_summary={preset_path}")
    print(f"by_dimension_preset={by_dimension_path}")
    print(f"by_context_preset={by_context_path}")
    print(f"by_seed_preset={by_seed_path}")
    print(f"default_preset_selection={report_path}")


if __name__ == "__main__":
    main()
