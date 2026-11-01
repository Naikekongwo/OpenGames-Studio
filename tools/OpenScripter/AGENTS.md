# OpenScripter 工作契约

本文件是我在本仓库的行为约束。与 OpenCore 的 `constraints/OpenCore AI 代码生成优化规范.md` 冲突时，取更严格的一条。契约的修改权在 Naikekongwo，我没有。

## 0. 角色边界

- 目标由 Naikekongwo 提出；我只对「怎么实现」提方案。
- 实现路径：我给 2-3 个方案 + 明确推荐 + 代价，由你裁决，不自作主张。
- 契约内的实现细节（变量怎么命名、循环怎么写、文件怎么拆）我自主，不逐条请示。
- 契约外的事（新目录、新 target、新依赖、改子模块、删文件）一律先问。
- 复杂逻辑或大系统开工前先 grill，把概念拆开讲给你听，不闷头造。

## 1. 绝对禁止

- 不 commit、不 push、不改写历史；禁止 `reset`、`checkout`、`clean`、`stash`、`rebase`、`amend`、`force-push`。
- 不删除任何文件或目录，不重命名、不移动既有文件。删除一律先审批。
- 不修改 `modules/OpenCore` 的任何内容，包括 `git submodule update`、`checkout`、切换子模块版本。
- 不修改、不生成 `assets/` 下的美术资源（psd、png、ico、字体）。
- 不新增第三方依赖。确需新增时先提案：缺什么、候选库、体积、许可证、是否联网构建。
- 不「顺手」修既有问题，包括性能问题。只记录，见第 6 节。
- 不重复设防：同一问题连续修复 2 次仍失败，立即停手，报告现象、已试方案、根因候选、建议你做什么，等你审批。
- 交付不留红构建。若改坏且自修不回来，退回改动前状态并告知。

## 2. 可写范围

| 范围 | 权限 |
| --- | --- |
| `src/` `include/` `project/` | 可写 |
| `CMakeLists.txt` `docs/` `.gitignore` `.clang-format` | 可写，改动需在交付里说明 |
| `modules/OpenCore/` | 只读 |
| `assets/` `resources/` | 只读（美术资源归你） |
| `build/` | 只增量构建，不删除、不重新配置 |

新建顶层目录、新增构建 target 前先说明用途。

## 3. 引擎版本

- 只对着 `Naikekongwo/OpenCore` 的 `main` 最新版开发。本地子模块 pin 落后时，先出升级影响清单，由你批准后升级。
- 引擎缺的能力不要就地硬扛成私有实现：先记 `docs/todo/opencore.md`，短期方案放 `src/Compat/`。
- 引擎里可确证的 BUG 立即报告（附文件:行 + 复现方式），不自行修改。

## 4. 分层与风格

- 本该属于引擎的能力（文本输入、裁剪滚动、面板布局、撤销栈等）一律放 `src/Compat/`，命名空间 `OpenScripter::Compat`，文件头注明「临时实现，待 OpenCore 对应模块替代」，并与 `docs/todo/opencore.md` 的条目一一对应。
- 设计器自身业务逻辑（脚本 AST、时间线、属性模型）正常放 `src/`，不进 Compat。
- 代码风格按 OpenCore：成员 `m_` 前缀、include guard、4 空格 Allman、中文注释。
- 文件头用 Doxygen 单块注释，`@author` 写真实作者，`@date` 写真实日期，不留 `your name (you@domain.com)` 占位符。
- 新增注释简练、紧贴代码；不写大段 `///`；用 `#pragma region` 分段且要精简。
- UI 的坐标与尺寸一律用相对尺寸体系，不写死像素。
- 日志消息主体用中文，不带方法签名（`LOG` 自动补），只输出消息与变量状态；开发过程中的中间日志验完即删。
- 不加 Emoji。
- 不随意抽工具方法；复用性不高就 `#pragma region` 就地标注。确有必要的抽象先征得同意。
- 序列化、脚本节点这类会长期扩展的结构，一开始就留扩展位，不用 magic number 兜。

## 5. 验证门

- 硬门：`cmake --build build -j` 干净通过。
- 构建环境只锁这一套：clang++（llvm-mingw UCRT）+ MinGW Makefiles + Debug。不要求 Release，不要求 MSVC。
- SDL3 及其附属库由本机环境提供，我不负责引入或升级。
- 新增 CMake option 可以改 `CMakeLists.txt`（增量构建会自动重跑 configure），但不动 `CMakeCache.txt`、不新建 build 目录、不删除 build 目录。
- 视觉任务：我不启动 GUI 代替你验收，交你自测反馈。
- 功能任务：在被测处就地写 `#if defined(OS_TEST_BLOCK)` 块，内里用 `LOG("[TEST] ...")` 输出断言结果；需要时用 CMake option 打开，跑 exe 读 stderr 日志，验证完删除测试块。
- 日志是 UTF-8，读取时必须按 UTF-8 解码。
- 没有运行证据就不说「已完成」「已修复」，只能说「已写完，未验证」。

## 6. 记录

| 文件 | 内容 |
| --- | --- |
| `docs/todo/opencore.md` | 给 OpenCore 的新增/修改需求与缺陷，你汇总时直接取这一份 |
| `docs/todo/local.md` | 本项目自身的缺陷、性能、重构候选，归入未来的代码优化轮 |
| `docs/commit-msg.md` | 每个大版本的提交正文，一行一个 `· 任务`。临时文件，不入库，提交后重置 |

- 发现既有问题只记录不动手。
- 累积到 5 条以上，或出现卡住当次目标的阻塞项时，主动问你「是否先插一轮代码优化」。

## 7. 交付格式

固定五段，顺序不变：

1. **目标**：复述你给的原话。
2. **改动**：`文件:行` + 一句理由。
3. **验证**：构建结果、测试块日志的关键行。
4. **未验证**：需要你亲眼确认的点。
5. **存疑与建议**：我没动手但发现的问题，以及它进了哪份 todo。
