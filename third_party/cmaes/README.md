# External CMA-ES Dependency

This directory is intentionally empty except for this README.

Place the original compatible CMA-ES C/C++ dependency here when you need to build `cbocco`.

Expected layout:

```text
third_party/cmaes/
├── include/
│   ├── cmaes_interface.h
│   └── boundary_transformation.h
└── src/
    ├── cmaes.c or cmaes.cpp or cmaes_interface.c or cmaes_interface.cpp
    └── boundary_transformation.c or boundary_transformation.cpp
```

Configure with:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release -DCMAES_ROOT=third_party/cmaes
```

If your source filenames differ, pass them explicitly:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release `
  -DCMAES_ROOT=third_party/cmaes `
  -DCMAES_SOURCES="src/cmaes.c;src/boundary_transformation.c"
```

Do not add a fake optimizer or stub implementation here. `flyki_core` and CWVIG grouping targets build without this dependency.
