# AGENTS.md - CWVIG-OSD 仓库协作规则

本文件是本仓库的本地工作规则。所有自动化代理和人工协作者在修改本仓库前都应先阅读并遵守这些约束。

## 项目目标

本仓库的目标是在重叠大规模全局优化代码基础上实现 **Confidence-weighted Variable Interaction Graph for Overlapping Soft Decomposition (CWVIG-OSD)**。

研究主线是：

1. 从黑盒函数评价中估计带不确定性的变量交互图 `W, U`。
2. 使用重叠社区或 affiliation model 恢复软分组 `Z`。
3. 将共享变量置信度纳入协同进化更新。
4. 在 Flyki/Large-Scale-Overlapping-Optimization 基准上与原始 CBCCO 流程对照验证。

## Repo Layout

- `benchmark/flyki_overlap/`: 从 `Flyki/Large-Scale-Overlapping-Optimization` 迁移或克隆的原始重叠大规模优化代码。这里包含 `CBOCC.cpp`、`CBOG_CBD.*`、`CMAESO.*`、`F1.cpp` 到 `F12.cpp`、`Benchmarks.*` 和 `cdatafiles/`。
- `benchmark/cec2013lsgo/`: 从 `dmolina/cec2013lsgo` 迁移或克隆的快速验证基准。
- `grouping/`: 新增 CWVIG-OSD 分组逻辑的首选目录，包括概率交互估计、加权变量交互图、软重叠分解、共享变量校准。
- `cc/`: 协同进化适配层和 shared-variable-aware update 的 C++ 实验代码。优先放适配层，不直接重写核心优化器。
- `experiments/`: 可复现实验入口、批量运行脚本、统计检验脚本和最小 smoke test。
- `metrics/`: 新增指标、分组准确率、overlap 变量识别质量、运行预算统计等度量代码。若目录不存在，需要新增指标时再创建。
- `results/`: 实验输出、summary、日志和可复现产物。不要把不可复现的手工结果混入这里。
- `docs/`: 设计说明、阶段计划、接口记录和读仓库笔记。
- `tests/`: Python 或轻量接口测试。新增模块应优先配套最小测试。

## Build And Run Commands

每次修改后，最终回复必须列出实际执行过的编译命令、运行命令和结果文件位置。没有执行的命令不能写成已通过。

当前 Python 侧基础验收命令：

```powershell
python -m pytest -q
python experiments/grouping_accuracy.py
```

如果当前机器的 `python` 不在 PATH，可使用 Codex 工作区运行时或本地虚拟环境中的 Python，但回复必须写出实际使用的完整命令。

Flyki C++ 侧当前没有完整构建脚本，且原仓库未包含 `cmaes_interface.h`、`boundary_transformation.h` 对应的完整 CMA-ES 源文件。补齐依赖并新增构建脚本前，不允许声称 C++ 已编译通过。

建立 C++ 构建后，回复中必须至少包含：

```powershell
cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release
cmake --build build/flyki --config Release
```

原始 CBCCO 基线运行命令应保持兼容：

```powershell
.\build\flyki\CBOCC.exe 1 CBCCO 1 1000
```

新增 CWVIG-OSD 方法后，应提供对应运行命令，例如：

```powershell
.\build\flyki\CBOCC.exe 1 CWVIG_OSD 1 1000
```

结果文件位置必须明确到路径，例如：

```text
results/f1_seed1_cwvig_edges.csv
results/f1_seed1_grouping.json
results/f1_seed1_cwvig_osd.result.csv
benchmark/flyki_overlap/1.1.100.CBOG-CBD.result.txt
```

## Coding Rules

1. 不要破坏原始 benchmark 函数。除非任务明确要求修 benchmark bug，否则不要修改 `benchmark/flyki_overlap/F*.cpp`、`F*.h`、`Benchmarks.*` 和 `cdatafiles/` 的函数语义。
2. 不要删除或覆盖原始 `CBOCC`、`CBOG_CBD`、`CMAESO` 实现。需要扩展时优先新增方法分支、适配层或 wrapper。
3. 新增代码应尽量放在 `grouping/`、`experiments/`、`metrics/`。C++ 协同进化适配代码可放在 `cc/`。
4. 不允许一次性重写整个仓库。每次变更应是小步、可编译、可验证的垂直切片。
5. 遇到不清楚的接口，先写适配层或数据转换层，不直接改核心优化器。
6. 优先复用现有数据格式：`po.txt` 表示 grouping 文件，`oo.txt` 表示 overlapping variable 文件。新格式必须提供转换或兼容读写。
7. 不硬编码绝对路径、随机种子以外的实验参数或本机私有配置。
8. 不引入与当前阶段无关的大型框架。新增依赖必须服务于 CWVIG-OSD 的实现、实验或验证。
9. 修改共享变量更新逻辑时，要保留原始 CBCCO 硬分配流程作为可运行对照。
10. 如果必须改 `CBOG_CBD` 或 `CMAESO`，应优先新增函数或策略参数，避免把 CWVIG-OSD 逻辑散落进原有流程。

## Experiment Rules

1. 所有新实验必须可复现，随机种子必须显式设置，并写入命令行参数、配置文件或结果 summary。
2. 每个实验输出至少记录：函数编号、方法名、seed、最大评价次数、实际评价次数、最优适应度、结果文件路径。
3. 每次实验代码修改后，必须说明：
   - 编译命令：实际执行的 C++ 或 Python 构建命令。
   - 运行命令：实际执行的 benchmark 或实验脚本命令。
   - 结果文件位置：生成的 `results/` 或 benchmark 输出文件路径。
4. 分组实验要保存中间产物：`W`、`U`、`Z`、shared-variable confidence、最终 hard/soft grouping。
5. 对照实验必须保留原始 `CBCCO` 方法，新增 `CWVIG_OSD` 不能替代基线入口。
6. 批量实验脚本必须允许设置 seed 列表和函数列表，不能把实验范围隐式写死在代码深处。
7. 阶段完成后必须更新 `PROGRESS.md`，记录日期、完成内容、关键命令、结果文件和未解决风险。

## Done Criteria

一个阶段只有同时满足以下条件，才可以标记为完成：

1. 相关代码或文档已落到预期目录，没有无关重构。
2. 原始 benchmark、原始 `CBOCC` 和原始 CMA-ES 实现没有被删除。
3. 新增实验显式设置随机种子，并能从命令行复现。
4. 已运行与修改范围匹配的最小验收命令。
5. 最终回复列出实际编译命令、运行命令和结果文件位置。
6. `PROGRESS.md` 已更新阶段状态、验证结果和遗留问题。
7. 如果某项验收无法运行，必须说明原因、影响范围和下一步最小修复动作。

## Recommended Insertion Boundaries

- 在 `CBOCC` 入口附近接入 grouping provider，让 `CBCCO` 继续读取原始 `po/oo` 文件，让 `CWVIG_OSD` 使用新分组来源。
- 在 `CBOG_CBD` 构造或初始化阶段传入软分组和共享变量置信度。
- 在 `optimizationStage` 中替换或旁路 overlap 变量硬删除逻辑，但保留原实现作为 baseline。
- 在 `CMAESO` 上层适配 context update，优先通过 shared-variable policy 控制更新，不直接把核心 CMA-ES 采样和分布更新改成新算法。

## Progress Log Rule

每个阶段结束都必须更新根目录 `PROGRESS.md`。建议格式：

```markdown
## YYYY-MM-DD - 阶段名称

- 状态：done / partial / blocked
- 修改文件：
- 编译命令：
- 运行命令：
- 结果文件：
- 关键观察：
- 遗留风险：
```

如果 `PROGRESS.md` 尚不存在，完成第一个阶段时创建它。不要用临时聊天记录替代 `PROGRESS.md`。
