# CMA-ES Dependency

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 6C dependency recovery; no optimizer behavior change.

## Why External

The original Flyki overlapping-optimization code wraps an external CMA-ES implementation through `CMAESO.h` and `CMAESO.cpp`. `cbocco` needs the matching C API headers and implementation sources.

Do not stub, rewrite, or silently replace CMA-ES. The missing dependency must be supplied explicitly.

## Bundled Dependency

The project now vendors the minimal compatible upstream `c-cmaes` files under `third_party/cmaes`.

- Upstream: https://github.com/CMA-ES/c-cmaes
- Source revision checked when added: `4450d3deccf2aacb6aa955d8216cfc4461699c60`
- License: see `third_party/cmaes/LICENSE`; upstream allows Apache License 2.0 or LGPL 2.1 or later.

Vendored files:

- `third_party/cmaes/include/cmaes_interface.h`
- `third_party/cmaes/include/cmaes.h`
- `third_party/cmaes/include/boundary_transformation.h`
- `third_party/cmaes/src/cmaes.c`
- `third_party/cmaes/src/boundary_transformation.c`

## Required Headers

`CMAESO.h` includes:

- `cmaes_interface.h`
- `boundary_transformation.h`

`boundary_transformation.h` is included inside `extern "C"`.

## Required Types

`CMAESO` stores these external types:

- `cmaes_t evo`
- `cmaes_boundary_transformation_t boundaries`

## Required CMA-ES API

`CMAESO.cpp` calls:

- `cmaes_init`
- `cmaes_Get`
- `cmaes_exit`
- `cmaes_resume_distribution`
- `cmaes_NewDouble`
- `cmaes_SamplePopulation`
- `cmaes_ReSampleSingle`
- `cmaes_UpdateDistribution`

## Required Boundary API

`CMAESO.h` declares and `CMAESO.cpp` calls:

- `cmaes_boundary_transformation_init`
- `cmaes_boundary_transformation_exit`
- `cmaes_boundary_transformation`
- `cmaes_boundary_transformation_inverse`
- `cmaes_boundary_transformation_shift_into_feasible_preimage`

## Expected Layout

Preferred local layout:

```text
third_party/cmaes/
├── LICENSE
├── include/
│   ├── cmaes_interface.h
│   ├── cmaes.h
│   └── boundary_transformation.h
└── src/
    ├── cmaes.c
    └── boundary_transformation.c
```

The default `CMAES_ROOT` points at `third_party/cmaes`.

## CMake Options

Default convention:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release -DCMAES_ROOT=third_party/cmaes
```

If source filenames differ:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release `
  -DCMAES_ROOT=third_party/cmaes `
  -DCMAES_SOURCES="src/cmaes.c;src/boundary_transformation.c"
```

Backward-compatible aliases remain available:

- `FLYKI_CMAES_DIR`
- `FLYKI_CMAES_SOURCES`

Prefer `CMAES_ROOT` and `CMAES_SOURCES` for new runs.

## Verification

With the dependency present:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release -DCMAES_ROOT=third_party/cmaes
cmake --build build/flyki --target cmaes_dependency_check --config Release
cmake --build build/flyki --target cbocco --config Release
```

Without the dependency, the expected result is a clear missing-dependency diagnostic when building either `cmaes_dependency_check` or `cbocco`.

## Targets Requiring CMA-ES

- `cmaes_dependency_check`
- `cbocco`

`cbocco` includes:

- `CBOCC.cpp`
- `CBOG_CBD.cpp`
- `CMAESO.cpp`
- benchmark sources
- external CMA-ES sources

## Targets Independent Of CMA-ES

These targets must remain buildable without CMA-ES:

- `flyki_core`
- `grouping_core`
- `probe_core`
- `soft_overlap_core`
- `cwvig_grouping_pipeline_cli`
- `cwvig_grouping_pipeline_tests`
- grouping metrics/provider/source loader targets

## Current Status

As of Phase 6C verification on 2026-06-30, the compatible CMA-ES dependency is present locally and `cbocco` builds with GCC + Ninja.

```powershell
$env:PATH="$env:USERPROFILE\scoop\apps\gcc\15.2.0\bin;$env:USERPROFILE\scoop\apps\ninja\current;$env:USERPROFILE\scoop\shims;$env:PATH"
& $env:USERPROFILE\scoop\shims\cmake.exe -S benchmark/flyki_overlap -B build/flyki_phase6c -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="$env:USERPROFILE\scoop\apps\gcc\15.2.0\bin\gcc.exe" -DCMAKE_CXX_COMPILER="$env:USERPROFILE\scoop\apps\gcc\15.2.0\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="$env:USERPROFILE\scoop\shims\ninja.exe" -DCMAES_ROOT=third_party/cmaes
& $env:USERPROFILE\scoop\shims\cmake.exe --build build/flyki_phase6c --target flyki_core --config Release
& $env:USERPROFILE\scoop\shims\cmake.exe --build build/flyki_phase6c --target cwvig_grouping_pipeline_cli --config Release
& $env:USERPROFILE\scoop\shims\cmake.exe --build build/flyki_phase6c --target cwvig_grouping_pipeline_tests --config Release
& build\flyki_phase6c\cwvig_grouping_pipeline_tests.exe
& $env:USERPROFILE\scoop\shims\cmake.exe --build build/flyki_phase6c --target cmaes_dependency_check --config Release
& $env:USERPROFILE\scoop\shims\cmake.exe --build build/flyki_phase6c --target cbocco --config Release
```

Smoke command:

```powershell
& ..\..\build\flyki_phase6c\cbocco.exe 1 CBCCO 1 1000
```

Run from `benchmark/flyki_overlap`.

Smoke artifacts were archived locally:

- `results/phase6c_cbocco_smoke/1.1.100.CBOG-CBD.result.txt`
- `results/phase6c_cbocco_smoke/CBCCOtimefile.txt`

The smoke reached the original CBCCO flow and produced the legacy result file. No SharedVariablePolicy, CBOG_CBD optimization logic, CBOCC optimization behavior, CMAESO algorithm logic, or benchmark function logic was changed.
