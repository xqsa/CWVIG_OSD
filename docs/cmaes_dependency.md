# CMA-ES Dependency

- Date: 2026-06-30
- Executor: Codex
- Scope: Phase 6A dependency recovery only; no optimizer behavior change.

## Why External

The original Flyki overlapping-optimization code wraps an external CMA-ES implementation through `CMAESO.h` and `CMAESO.cpp`. The repository does not currently contain the required CMA-ES headers or implementation sources, so `cbocco` cannot be built until the dependency is provided.

Do not stub, rewrite, or silently replace CMA-ES. The missing dependency must be supplied explicitly.

## Repository Search Result

Only wrapper files are present:

- `benchmark/flyki_overlap/CMAESO.h`
- `benchmark/flyki_overlap/CMAESO.cpp`

No bundled `cmaes_interface.h`, `boundary_transformation.h`, or CMA-ES implementation source file is present in the repository.

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
├── include/
│   ├── cmaes_interface.h
│   └── boundary_transformation.h
└── src/
    ├── cmaes.c or cmaes.cpp or cmaes_interface.c or cmaes_interface.cpp
    └── boundary_transformation.c or boundary_transformation.cpp
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

As of Phase 6B verification on 2026-06-30, no compatible CMA-ES dependency is present locally. The actual local layout is:

```text
third_party/cmaes/
└── README.md
```

Missing files:

- `third_party/cmaes/include/cmaes_interface.h`
- `third_party/cmaes/include/boundary_transformation.h`
- one compatible CMA-ES implementation source under `third_party/cmaes/src/`
- one compatible boundary transformation implementation source under `third_party/cmaes/src/`

Verified independent targets still build without CMA-ES:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki_phase6b -DCMAKE_BUILD_TYPE=Release -DCMAES_ROOT=third_party/cmaes
cmake --build build/flyki_phase6b --target flyki_core --config Release
cmake --build build/flyki_phase6b --target cwvig_grouping_pipeline_cli --config Release
cmake --build build/flyki_phase6b --target cwvig_grouping_pipeline_tests --config Release
.\build\flyki_phase6b\Release\cwvig_grouping_pipeline_tests.exe
```

Current blocker:

```powershell
cmake --build build/flyki_phase6b --target cmaes_dependency_check --config Release
cmake --build build/flyki_phase6b --target cbocco --config Release
```

Both commands fail with the documented missing-dependency diagnostic until the external files are supplied. The build path is prepared and documented, but `cbocco` remains unavailable.
