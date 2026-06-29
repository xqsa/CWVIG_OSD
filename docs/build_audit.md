# Phase 0 Build Audit

Date: 2026-06-29  
Executor: Codex  
Scope: `benchmark/flyki_overlap`

## Purpose

Phase 0 creates a safe, reproducible build foundation for the Flyki overlapping
optimization code without implementing CWVIG-OSD and without changing algorithm
logic.

The build is split into:

- `flyki_core`: benchmark/core sources that do not require external CMA-ES.
- `cbocco`: full CBCCO executable, available only when the original external
  CMA-ES dependency is supplied.

## Source File Inventory

Inspection command:

```powershell
rg --files -g '*.cpp' -g '*.h'
```

Files inspected:

```text
Benchmarks.cpp
Benchmarks.h
CBOCC.cpp
CBOG_CBD.cpp
CBOG_CBD.h
CMAESO.cpp
CMAESO.h
F1.cpp
F1.h
F2.cpp
F2.h
F3.cpp
F3.h
F4.cpp
F4.h
F5.cpp
F5.h
F6.cpp
F6.h
F7.cpp
F7.h
F8.cpp
F8.h
F9.cpp
F9.h
F10.cpp
F10.h
F11.cpp
F11.h
F12.cpp
F12.h
Header.cpp
Header.h
```

### Benchmark/Core Sources

These files define benchmark functions and the benchmark factory. They do not
depend on CMA-ES.

| Category | Files | Notes |
|---|---|---|
| Benchmark base | `Benchmarks.h`, `Benchmarks.cpp` | Reads `cdatafiles/Fx-*` and implements shared benchmark math. |
| Benchmark factory | `Header.h`, `Header.cpp` | Creates `F1` to `F12` benchmark objects. |
| Function definitions | `F1.h/.cpp` to `F12.h/.cpp` | Original overlapping benchmark functions. Algorithm logic must remain unchanged. |
| Benchmark data | `cdatafiles/` | Shift vectors, permutations, rotations, sizes, and weights. |

### Full CBCCO Sources

These files are required for the full optimizer executable.

| Category | Files | Notes |
|---|---|---|
| Main entry | `CBOCC.cpp` | Parses `func method seed maxfes`, reads `po/oo`, launches `CBOG_CBD`. |
| Optimizer body | `CBOG_CBD.h`, `CBOG_CBD.cpp` | Owns CBCCO stages and group assignment flow. |
| CMA-ES wrapper | `CMAESO.h`, `CMAESO.cpp` | Wraps the external CMA-ES C/C++ implementation. |
| Benchmark/core | `Benchmarks.*`, `Header.h`, `F1..F12` | Linked into the executable. `Header.cpp` is intentionally not used by `cbocco` because `CBOCC.cpp` defines its own `generateFuncObj`. |
| External CMA-ES | `cmaes_interface.h`, `boundary_transformation.h`, implementation sources | Missing from this repository. |

### Grouping IO And Metrics Later

No dedicated grouping IO or metrics C++ module exists yet in
`benchmark/flyki_overlap`.

Current grouping-related logic is embedded in `CBOCC.cpp`:

- Reads `${func}po.txt` into `groups`.
- Reads `${func}oo.txt` into `overiables` and `overiablesRedandunt`.
- Builds `sharedvar_group_pos`.

Later phases should extract this behavior into an adapter or IO module before
adding CWVIG-OSD. New code should prefer root-level `grouping/`,
`experiments/`, and `metrics/`, with C++ adapter code in `cc/` if needed.

## Target Dependency Graph

```text
flyki_core
├── Benchmarks.cpp
├── Header.cpp
├── F1.cpp ... F12.cpp
└── headers/data under benchmark/flyki_overlap

cbocco
├── CBOCC.cpp
├── CBOG_CBD.cpp/.h
├── CMAESO.cpp/.h
├── Benchmarks.cpp
├── F1.cpp ... F12.cpp
└── external original CMA-ES dependency
    ├── cmaes_interface.h
    ├── boundary_transformation.h
    ├── CMA-ES implementation source, for example cmaes.c/cmaes.cpp
    └── boundary transformation implementation source
```

`flyki_core` is independent of external CMA-ES. `cbocco` is deliberately gated:
if the CMA-ES headers or implementation files are missing, the CMake `cbocco`
target reports the missing dependency instead of stubbing or rewriting CMA-ES.

## Missing Dependencies

Repository inspection found only:

- `CMAESO.h`
- `CMAESO.cpp`

The following external files required by `CMAESO.h` are missing from
`benchmark/flyki_overlap`:

- `cmaes_interface.h`
- `boundary_transformation.h`
- CMA-ES implementation source, for example `cmaes.c`, `cmaes.cpp`,
  `cmaes_interface.c`, or `cmaes_interface.cpp`
- boundary transformation implementation source, for example
  `boundary_transformation.c` or `boundary_transformation.cpp`

Phase 0 does not download, stub, or rewrite these dependencies.

## Build Files Added

- `benchmark/flyki_overlap/CMakeLists.txt`
- `benchmark/flyki_overlap/cmake/report_missing_cmaes.cmake`

The CMake file defines:

- `flyki_core`: static library from benchmark/core sources.
- `cbocco`: full executable if the original CMA-ES dependency is supplied;
  otherwise a custom target that reports missing dependency files.

## Build Commands

From repository root:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target cbocco --config Release
```

If the original CMA-ES source distribution is available:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release -DFLYKI_CMAES_DIR=<path-to-original-cmaes>
cmake --build build/flyki --target cbocco --config Release
```

If the source files are not discoverable automatically:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release -DFLYKI_CMAES_DIR=<path-to-original-cmaes> -DFLYKI_CMAES_SOURCES="cmaes.c;boundary_transformation.c"
cmake --build build/flyki --target cbocco --config Release
```

## Known Blockers

1. Local environment blocker: `cmake` is not available in the current PATH.
2. Local environment blocker: no C++ compiler was found in PATH (`g++`,
   `clang++`, and `cl` were not available).
3. Repository dependency blocker: the original external CMA-ES headers and
   implementation files are missing.
4. Full `cbocco` may need a small portability pass after the original CMA-ES
   dependency is supplied. `CMAESO.cpp` currently uses MSVC-specific
   `strcpy_s`; Phase 0 did not change this because full compilation is already
   blocked by missing external CMA-ES files.

## Verification Status

The build foundation has been added, but local compilation could not be run in
this environment until CMake and a C++ compiler are installed or exposed in
PATH.

Commands attempted from `E:\CWVIG_OSD`:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target cbocco --config Release
```

Observed result:

```text
cmake: The term 'cmake' is not recognized as a name of a cmdlet, function, script file, or executable program.
CMAKE_NOT_FOUND
CPP_COMPILER_NOT_FOUND
```

Expected status:

- `flyki_core`: buildable from benchmark/core sources once CMake and a C++
  compiler are available.
- `cbocco`: intentionally unavailable until the original CMA-ES dependency is
  supplied.

## Recommended Next Goal

Install or expose a local CMake + C++ compiler toolchain, then run:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --target flyki_core --config Release
cmake --build build/flyki --target cbocco --config Release
```

After `flyki_core` is verified, the next implementation goal should be a
grouping IO adapter that preserves the original `po/oo` file behavior before
any CWVIG-OSD estimator or soft decomposition logic is introduced.
