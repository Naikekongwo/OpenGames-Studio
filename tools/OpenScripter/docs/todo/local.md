# 本项目自身的待办

只记录，不动手（见 `AGENTS.md` 第 1 节）。累积到 5 条以上或出现阻塞项时，会先问是否插一轮代码优化。

归轮：`性能轮` / `功能轮` / `待批准`。

---

## 需求条目（来自 `docs/direction.md`）

R 条目是方向确定后拆出的开发任务，与下方 L 条目来源不同。

### R-001 M0 UI 骨架

- 类型：功能
- 内容：启动页（新建/选择/最近工程）与编辑页四区布局，使用假工程与假台本；全屏预览占位。
- 依赖：无
- 归轮：功能轮（M0）
- 状态：进行中（启动页首版已落地；编辑页与最近工程待做）

### R-002 M1 工程体系

- 类型：功能
- 内容：工程 manifest 格式、目录扫描、创建/打开/最近工程、空台本加载。
- 依赖：`direction.md` 6.1
- 归轮：功能轮（M1）
- 状态：待做

### R-003 M2 Compat 基础设施

- 类型：功能
- 内容：文本输入（IME、光标、选区）、滚动视口、面板布局。
- 依赖：OC-003、OC-004、OC-005
- 归轮：功能轮（M2）
- 状态：待做

### R-004 M3 台本视图与单步编辑

- 类型：功能
- 内容：只读节点列表、选中定位、由指令元数据生成表单、改动回写文本。
- 依赖：R-003；先用占位指令表
- 归轮：功能轮（M3）
- 状态：待做

### R-005 M4 语言与解析

- 类型：功能
- 内容：语法、指令集、解析与序列化、节点到文本区间的映射、校验。
- 依赖：反编译原型 ScriptEngine
- 归轮：功能轮（M4）
- 状态：阻塞，等反编译

### R-006 M5 预览

- 类型：功能
- 内容：从当前步起播、暂停/继续/单步/停止、全屏、按资源注册清单加载。
- 依赖：R-005
- 归轮：功能轮（M5）
- 状态：待做

### R-007 M6 旧脚本迁移

- 类型：功能
- 内容：旧指令集到新脚本的无损转换，一次性迁移。
- 依赖：R-005、旧脚本格式已摸清
- 归轮：功能轮（M6）
- 状态：待做

### R-008 脚本规范文档

- 类型：规范
- 内容：语法、指令表、节点映射规则、未知指令行为，与解析器同步更新。
- 依赖：R-005
- 归轮：功能轮（M4）
- 状态：待做

### R-009 资源注册清单

- 类型：规范 + 功能
- 内容：确定工程内资源注册清单的格式与加载方式，与引擎的注册方式一致；预览按清单取资源。
- 依赖：L-006、资源接口定稿
- 归轮：功能轮（M1 起草，M5 接入）
- 状态：待做

---

## 缺陷与优化条目

### L-001 异常按值捕获

- 类型：性能 + 规范
- 位置：`project/OpenScripter.cpp:14`（`catch (std::exception e)`）；`src/OpenScripter.cpp` 已在重写 StartUp 时改为 `const std::exception &`。
- 现象：按值捕获触发对象切片与一次拷贝。
- 期望：`catch (const std::exception &e)`。
- 归轮：性能轮
- 状态：部分解决（src 侧已改；project 侧仍按值）

### L-002 依赖隐式传递包含

- 类型：可维护性
- 位置：`project/OpenScripter.cpp:16` 使用 `std::cout`，但本文件未 `#include <iostream>`；`src/OpenScripter.cpp` 的 `std::cout` 已改用 `LOG`。
- 现象：原先靠 `modules/OpenCore/include/Asset/ResourceManager.hpp:16` 的传递包含才能编译；子模块升级后 `ResourceManager` 移除，构建直接报 `no member named 'cout'`。
- 期望：自己用到的头文件自己包含。
- 归轮：性能轮（与 OC-002 一起做更省事）
- 状态：已解决（2026-10-28 补 `<iostream>`，`src` 侧改 `LOG`）

### L-003 预加载清单不存在，加载失败被当成成功

- 类型：功能缺陷
- 位置：`src/Stage/PreloadStage.cpp`（原 `LoadResourcesFromJson(10001)`，现已移除）、`modules/OpenCore/include/Core/Macros.hpp`（`JSON_PATH`，现不再使用）
- 现象：运行日志报 `JSON file assets/script/STAGE_10001.json does not exist`，但 `LoadingState` 仍返回 `ready`，流程继续推进。
- 期望：先确定清单文件是否该存在；若存在则补文件，若不该存在则删掉这次调用。根因之一在 OC-008。
- 归轮：功能轮
- 状态：已解决（2026-10-28 重写 PreloadStage，不再调用 LoadResourcesFromJson）

### L-004 字体 9001 未加载，FrameCounter 画不出字

- 类型：功能缺陷
- 位置：`src/Stage/PreloadStage.cpp`（原 `FrameCounter` 创建，现已移除）
- 现象：运行日志两次报 `GetFont failed to get font id 9001`。
- 期望：补字体资源与清单，或显式指定已加载的字体 id。
- 归轮：功能轮（等资源清单方案确定后再动）
- 状态：已解决（2026-10-28 注册 ChineseFont，启动页统一使用该字体）

### L-005 预加载结束后没有任何舞台接手

- 类型：功能缺陷
- 位置：`src/Stage/PreloadStage.cpp`（原 `removeStage` 之后无舞台接手；现切到 `StartupStage`）
- 现象：`PreloadStage` 是唯一的 overlay stage，`removeStage(StageType::overlayStage)` 之后 base/top 槽均为空，程序进入空白窗口空转（2026-10-28 实测：进程 5 秒后仍存活，画面无内容）。
- 期望：预加载完成后切入真正的设计器主舞台。
- 归轮：功能轮（会被主界面实现自然取代）
- 状态：已解决（2026-10-28 新增 StartupStage 接手）

### L-006 子模块升级适配

- 类型：版本 + 适配
- 位置：`modules/OpenCore`
- 现象：本条目记录时子模块在 `970a9f6`，落后 `origin/main` 55 个提交；提交 `b8b45f5` 已把它升到 `df9968c`（与 PhotoOrganizer 及 `origin/main` 一致）。
- 影响：原记录的四处断点已随启动页重写一并处理：`Stage` 改为默认构造、`UI<>` 改走 `PackageManager` 纹理名、`ResourceManager` 换成 `PackageManager`、不再使用 `JSON_PATH`。
- 期望：保留此条作为升级记录；下次升级前先对照 `df9968c` 之后的 API 变更。
- 归轮：已完成
- 状态：已解决（2026-10-28）

### L-007 格式化规则不完整

- 类型：规范
- 位置：`.clang-format`（仅 `BreakBeforeBraces: Allman`、`IndentWidth: 4`）
- 现象：`ColumnLimit`、指针绑定、`SortIncludes` 等均为 LLVM 默认值，未与本项目实际写法对齐。
- 期望：补全规则后再统一格式化，避免格式化产生大面积无关 diff。
- 归轮：待批准（会触发全仓库重排）
- 状态：待汇总

### L-008 资源目录约定不清

- 类型：规范
- 位置：`CMakeLists.txt:40-45` 把 `assets/` 整目录复制到 exe 旁；实际产物是 `build/assets/assets/ui/icon.png` 与 `build/assets/Raw/`，而清单路径是 `build/assets/script/`。
- 现象：资源根下同时存在 `assets/`（游戏资源）、`Raw/`（源文件）与未来的 `script/` 三种命名，层级含义不明。
- 期望：明确资源根与各子目录职责，写入文档后再建目录。
- 归轮：功能轮（先等脚本格式定下来）
- 状态：待汇总
