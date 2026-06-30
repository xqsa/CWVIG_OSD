# External CMA-ES Dependency

This directory vendors the minimal `c-cmaes` files needed by the Flyki `CMAESO`
wrapper.

- Upstream: https://github.com/CMA-ES/c-cmaes
- Source revision checked when added: `4450d3deccf2aacb6aa955d8216cfc4461699c60`
- License: see `LICENSE` from upstream; the library is available under Apache
  License 2.0 or LGPL 2.1 or later.

Bundled layout:

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

Configure with:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release -DCMAES_ROOT=third_party/cmaes
```

If using a different external CMA-ES checkout, pass source filenames explicitly:

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release `
  -DCMAES_ROOT=third_party/cmaes `
  -DCMAES_SOURCES="src/cmaes.c;src/boundary_transformation.c"
```

Do not replace these files with a fake optimizer or stub implementation.
`flyki_core` and CWVIG grouping targets build without this dependency.
