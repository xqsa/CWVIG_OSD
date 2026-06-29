#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "$repo_root/benchmark/flyki_overlap" -B "$repo_root/build/flyki" -DCMAKE_BUILD_TYPE=Release
cmake --build "$repo_root/build/flyki" --target cwvig_grouping_pipeline_cli --config Release
cmake --build "$repo_root/build/flyki" --target grouping_source_cli --config Release

python3 "$repo_root/experiments/run_grouping_batch.py" \
  --mode smoke \
  --output-root results/repro_smoke \
  --preset-config-dir configs/cwvig_presets \
  --clean

python3 "$repo_root/experiments/analyze_grouping_batch.py" \
  --output-root results/repro_smoke

python3 "$repo_root/experiments/check_reproducibility.py" --clean
