# 给 OpenCore 的需求清单

汇总说明见 `AGENTS.md` 第 3、6 节。本文件只记录需要在 **OpenCore 仓库**里新增或修改的事物；本项目内的临时实现见 `local.md` 对应条目。

条目状态取值：`待汇总` / `已并入 vX.Y` / `已接入并移除项目层实现`。

---

### OC-001 日志格式化语义不一致

- 类型：缺陷
- 位置：`modules/OpenCore/src/StageManager.cpp:78`
- 现象：`LOG("Removing stage at index %zu", index)` 原样输出 `%zu`。`LOG` 内部走 `std::format`，占位符应为 `{}`。
- 证据：2026-10-28 运行 `OpenScripter.exe`，stderr 出现 `Removing stage at index %zu`。
- 影响：日志误导；同一宏在两套格式化语义之间摇摆，后续极易复发。
- 期望：统一为 `std::format` 语义，或在规范中明确禁止 printf 风格占位符。
- 阻塞：否
- 状态：待汇总

### OC-002 头文件用全局 using 污染命名空间

- 类型：改进
- 位置：`include/World/Stage/StageManager.hpp:13`、`include/Core/Info/GameInfo.hpp:11`、`include/Core/Math/OpenCore_Wave.hpp:14`、`include/Runtime/Graphics/IDrawableObject/IDrawableObject.hpp:18`、`include/Asset/TextureLoader.hpp:22`、`include/Asset/ResourceManager.hpp:33`、`include/Runtime/Graphics/UI/TextArea.hpp:21`、`include/Runtime/Graphics/UI/TypeWriter.hpp:24`、`include/Runtime/Animation/Concrete/CollectionAnimation.hpp:7`
- 现象：头文件里出现 `using std::unique_ptr;`，被包含方因此可以在不写 `std::` 的情况下使用 `unique_ptr`。`include/OpenCore.hpp:73`、`:110` 正是依赖这种传递可见性。
- 影响：删除任何一个无关头文件都可能让远处的代码编译失败；包含顺序敏感。
- 期望：头文件内一律写全 `std::`，`using` 只出现在 `.cpp` 或 `using` 声明处在类作用域内。
- 阻塞：否
- 状态：待汇总

### OC-003 UI 层缺文本输入控件

- 类型：新能力
- 现状：`include/Runtime/Graphics/UI/` 下只有 `Button`、`TextButton`、`CheckBox`、`FrameCounter`、`ImageBoard`、`Scrollbar`、`Symbol`、`TextArea`、`ToolMenu`、`TypeWriter`，没有可编辑文本框。
- 需求：单行/多行文本输入，支持光标、选区、`SDL_EVENT_TEXT_INPUT` 组合输入（中文 IME 必需）、退格删除、复制粘贴。
- 用途：脚本设计器的代码编辑区。
- 本地过渡：`src/Compat/` 自绘实现。
- 阻塞：是（编辑器核心交互）
- 状态：待汇总

### OC-004 缺裁剪与滚动视口

- 类型：新能力
- 现状：`ImageBoard`、`TextArea` 直接绘制到渲染器，没有封装 `SDL_RenderSetClipRect` 或离屏目标；`Scrollbar` 只有滚动条控件本身，没有配套的可滚动容器。
- 需求：视口容器，内容超出时裁剪并按偏移绘制，与 `Scrollbar` 联动。
- 用途：长脚本列表、行号栏、属性面板。
- 本地过渡：`src/Compat/`。
- 阻塞：是
- 状态：待汇总

### OC-005 舞台结构固定为 3 槽，缺面板与布局管理

- 类型：新能力
- 位置：`include/World/Stage/StageManager.hpp:85-91`（`stageContainer` 固定 base/overlay/top 三槽，每槽一个 `ElementManager`）
- 需求：多面板/停靠布局与焦点管理，或允许注册新的舞台类型。
- 用途：设计器的主编辑区、资源树、属性面板、日志面板需要并存的布局。
- 本地过渡：在单槽内靠 layer 号堆叠，自建焦点轮转。
- 阻塞：否（可先单面板跑通）
- 状态：待汇总

### OC-006 TextArea 无逐段着色能力

- 类型：新能力
- 位置：`include/Runtime/Graphics/UI/TextArea.hpp`
- 现状：整块文本单一颜色（`m_colorR/G/B`），无等宽排版、无按区间着色。
- 需求：按字符区间着色、制表位、等宽字体排版。
- 用途：语法高亮。
- 本地过渡：`src/Compat/` 自绘。
- 阻塞：否（高亮可后置）
- 状态：待汇总

### OC-007 脚本运行时仍是空壳

- 类型：规范/接口定义
- 位置：`include/Gameplay/AVG/Runtime/ScriptEngine/ScriptEngine.hpp`、`include/Gameplay/AVG/Runtime/ScriptEngine/ScriptLoader.hpp`（`origin/main` 上均只有声明，无成员、无实现）
- 需求：与设计器共同定义脚本数据结构与运行时接口（指令集、条件、跳转、标签），两侧共用一份 schema。
- 用途：设计器的产物必须能被引擎直接执行。
- 阻塞：是（决定设计器产物的目标格式）
- 状态：待汇总

### OC-008 资源清单缺失时不上报失败

- 类型：改进
- 位置：`include/Asset/ResourceManager.hpp:119` `LoadResourcesFromJson`、`src/ResourceManager.cpp`
- 现象：`assets/script/STAGE_10001.json` 不存在时只打印 `Error: JSON file ... does not exist`，返回的 `future` 仍正常完成，调用方无法区分成功与失败。
- 影响：`PreloadStage` 把加载失败当成成功继续推进（见 `local.md` L-003）。
- 期望：以异常或状态值把失败传给调用方。
- 阻塞：否
- 状态：待汇总

### OC-009 UI 层缺圆角矩形与圆角按钮

- 类型：新能力
- 现状：`include/Runtime/Graphics/UI/` 下的控件只有矩形底或纹理底，没有按半径自绘的圆角控件；`RoundedPanel::PaintInto()` 这类逐行铺圆角的绘制能力也不在引擎里。
- 需求：可设填充色、描边色、描边宽、半径的圆角矩形元素，以及在其上叠文字并带 Normal/Hovered/Pressed 三态的圆角按钮。
- 用途：启动页的 BETA 标记、主次按钮；后续编辑页的面板与字段底。
- 本地过渡：已提升到引擎：`UIElement` 的 `setCornerRadius/setBorderColor/setBorderWidth`、`OpenUI::Panel`、`OpenUI::RoundedRect`；项目侧 `src/Compat/` 已删除。
- 阻塞：否
- 状态：已接入并移除项目层实现（2026-10-28）

### OC-010 TextArea 无法按内容自适应尺寸

- 类型：缺陷 + 新能力
- 位置：`src/Runtime/IDrawableObject_impl.cpp:169`、`:187`（`applyScale` 的 `w * h == 0` 分支解引用 `texture`）、`src/Runtime/TextArea_impl.cpp:6`（构造时传空纹理）
- 现象：读代码确认：对 `TextArea` 调用 `ScaleR(w, 0)` 或 `Scale(0, h)` 想按宽高比自动计算时，`texture` 为空，`texture->getTextureRatio()` 是空指针解引用。未运行复现。
- 影响：文本元素必须同时给死宽高，版面只能手算相对值；标题、标签做不了内容自适应。
- 期望：文本元素能按 `Text::Measure()` 结果自适应尺寸，或在没有纹理时跳过宽高比分支并给出可用尺寸。
- 用途：启动页标题、后续编辑页的标签与表单文字。
- 阻塞：否
- 状态：待汇总

### OC-011 setRenderTarget 按 SDL2 语义判断返回值

- 类型：缺陷
- 位置：`src/Runtime/GraphicsManager_impl.cpp:136`
- 现象：`if (SDL_SetRenderTarget(renderer, texture) != 0)`。SDL3 的 `SDL_SetRenderTarget()` 返回 `bool`（成功为 true），这句在成功时反而走进失败分支：每次调用都打印 `设置渲染目标失败: `（`SDL_GetError()` 为空）并返回 -1，实际渲染目标已经切换成功。
- 证据：2026-10-28 运行 `OpenScripter.exe`，启动页创建两个 `TextArea` 后 stderr 出现 4 条该日志；SDL3 头文件 `SDL_render.h` 声明为 `extern SDL_DECLSPEC bool SDLCALL SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture);`。
- 影响：返回值不可信；依赖它的调用方（`TextArea::generateTexture`）会误判，启动日志被噪声占满。
- 期望：改为 `if (!SDL_SetRenderTarget(...))`，或统一按 bool 语义处理。
- 阻塞：否
- 状态：已解决（2026-10-28 改为 SDL3 bool 语义）

### OC-012 同一字体按字号重复缓存整份字体数据

- 类型：性能
- 位置：`src/Asset/PackageManager_impl.cpp:718`（逐次 `extractResourceData`）、`:743`（每个字号各自持有 `dataKeepAlive`）
- 现象：`getFont(name, ptsize)` 每个字号都会从资源包里读出一整份字体字节并随 `TTF_Font` 持有；启动页用到 192/48/44 三种字号，17MB 字体在内存里变成三份。
- 影响：启动页工作集约 100MB；按代码推断其中约 51MB 来自三份重复字体数据（未单独测量）。
- 期望：按资源名缓存一份字体字节，多个 `TTF_Font` 共享同一份数据。
- 阻塞：否
- 状态：待汇总
