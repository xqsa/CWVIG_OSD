# Progress

## 2026-06-29 - Phase 0 Build Audit And Minimal Build Targets

- 状态：done with documented blockers
- 修改文件：
  - `benchmark/flyki_overlap/CMakeLists.txt`
  - `benchmark/flyki_overlap/cmake/report_missing_cmaes.cmake`
  - `docs/build_audit.md`
  - `PROGRESS.md`
- 编译命令：
  - `cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build/flyki --target flyki_core --config Release`
  - `cmake --build build/flyki --target cbocco --config Release`
- 运行命令：
  - Phase 0 未运行优化实验；本阶段只做构建审计和最小构建目标。
- 结果文件：
  - `docs/build_audit.md`
  - `benchmark/flyki_overlap/CMakeLists.txt`
- 验证结果：
  - `cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release` failed because `cmake` is not in PATH.
  - `cmake --build build/flyki --target flyki_core --config Release` failed because `cmake` is not in PATH.
  - `cmake --build build/flyki --target cbocco --config Release` failed because `cmake` is not in PATH.
  - Toolchain probe returned `CMAKE_NOT_FOUND` and `CPP_COMPILER_NOT_FOUND`.
- 关键观察：
  - `flyki_core` 已在 CMake 中定义为不依赖外部 CMA-ES 的 benchmark/core 静态库。
  - `cbocco` 已在 CMake 中定义为完整 CBCCO 目标；缺少 CMA-ES 时会通过报告脚本显式失败。
  - `CMAESO.h` 引用的 `cmaes_interface.h` 和 `boundary_transformation.h` 不在当前 Flyki 仓库中。
- 遗留风险：
  - 当前机器 PATH 中未发现 `cmake`。
  - 当前机器 PATH 中未发现 C++ 编译器 `g++`、`clang++` 或 `cl`。
  - 未提供原始 CMA-ES C/C++ 依赖前，完整 `cbocco` 不能编译。
  - 原始 `CMAESO.cpp` 中存在 MSVC-specific `strcpy_s`，完整依赖补齐后可能需要做无算法行为变化的兼容性修正。
