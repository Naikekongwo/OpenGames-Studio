# OpenGames Studio

OpenGames Studio 是 OpenCore 引擎配套工具集的中枢工程。

## 工程结构

```
OpenGamesStudio/
├── CMakeLists.txt         # 顶层：Studio 本体 + 子工程 + 共享 OpenCore
├── src/                   # Studio 的场景实现
├── include/OpenGamesStudio/
├── project/               # Studio 的程序入口
├── assets/                # Studio 的运行库与资源
├── resources/             # 图标与版本信息
├── modules/OpenCore/      # 全工程唯一的 OpenCore（git 仓库）
├── tools/
│   ├── OpenPackager/      # 资源包打包工具
│   ├── OpenScripter/      # 脚本设计器
│   └── OpenWorlder/       # 世界编辑器
└── build/                 # 构建目录
    ├── OpenGamesStudio.exe
    └── OpenPackager/ 等工具子目录
```

Studio 本身就是顶级工程，源码直接位于根目录下的 `src/`、`include/`、`project/`，
与 OpenPackager、OpenScripter 等单独工程的目录惯例保持一致。

## 构建

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 8
```

## 构建目标

全工程共享一份 `modules/OpenCore`，只编译一次，四个可执行文件链接同一个
`libOpenCore.a`。

| 命令 | 行为 |
| --- | --- |
| `cmake --build build` | 编译全部四个工程 |
| `cmake --build build --target OpenGamesStudio` | 先编三个工具，再编中枢 |
| `cmake --build build --target OpenPackager` | 只编 OpenPackager，不牵连其他 |

依赖关系是单向的：`OpenGamesStudio` 依赖三个工具，单独构建某个工具不会
触发中枢。

## 产物布局

OpenGamesStudio 本体直接输出到构建根目录（与其他 OpenCore 工程惯例一致），
三个工具各自输出到 `<build>/<工程名>/` 下独立的子目录，这样各自的
`assets/` 与运行期生成的 `data/`（资源包）互不覆盖：

```
build/
├── OpenGamesStudio.exe    # 中枢本体
├── SDL3*.dll
├── assets/
├── OpenPackager/          # 工具各自独立
├── OpenScripter/
└── OpenWorlder/
```

## 添加新的子工程

1. 在 `tools/` 下新建目录，编写 `CMakeLists.txt`：
   - 不要写 `add_subdirectory(modules/OpenCore)`
   - 直接 `target_link_libraries(<target> PRIVATE OpenCore)`
   - 输出目录用 `set_target_properties(<target> PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OPS_BIN_DIR}/<名字>")`
2. 在顶层 `CMakeLists.txt` 中 `add_subdirectory(tools/<名字>)`
3. 若需要让中枢依赖它，追加到顶层 `add_dependencies(OpenGamesStudio ...)` 列表
