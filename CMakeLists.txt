cmake_minimum_required(VERSION 3.20)

project(OpenGamesStudio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ──────────────────────────────────────────────
#  产物目录
# ──────────────────────────────────────────────
# OpenGamesStudio 本体直接输出到构建根目录（与其他 OpenCore 工程惯例一致）。
# 三个工具各输出到 <build>/<名字>/ 子目录，这样各自的 assets/ 与 data/
# 天然隔离，不会互相覆盖。
# 产物跟随构建目录，不污染源码树。
set(OPS_TOOLS_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "工具可执行文件的输出根目录")

# ──────────────────────────────────────────────
#  共享 OpenCore
# ──────────────────────────────────────────────
# 全工程只有这一份 OpenCore，四个可执行文件链接同一个目标。
# 子工程里不要再次 add_subdirectory(OpenCore)。
add_subdirectory(modules/OpenCore)

# ──────────────────────────────────────────────
#  OpenGames Studio 本体
# ──────────────────────────────────────────────
file(GLOB_RECURSE STUDIO_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/project/*.cpp"
)

add_executable(OpenGamesStudio ${STUDIO_SOURCES})

target_include_directories(OpenGamesStudio
PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(OpenGamesStudio
PRIVATE
    OpenCore
)

set(STUDIO_BIN_DIR "${CMAKE_BINARY_DIR}")
set_target_properties(OpenGamesStudio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${STUDIO_BIN_DIR}"
)

if(WIN32)

    target_sources(OpenGamesStudio
    PRIVATE
        "${CMAKE_SOURCE_DIR}/resources/resource.rc"
    )

    add_custom_command(
        TARGET OpenGamesStudio
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/assets/SDL3.dll"
            "${CMAKE_SOURCE_DIR}/assets/SDL3_image.dll"
            "${CMAKE_SOURCE_DIR}/assets/SDL3_mixer.dll"
            "${CMAKE_SOURCE_DIR}/assets/SDL3_ttf.dll"
            "$<TARGET_FILE_DIR:OpenGamesStudio>"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets/assets"
            "$<TARGET_FILE_DIR:OpenGamesStudio>/assets"

        COMMENT "拷贝 OpenGamesStudio 的运行库与资源"
    )

endif()

install(TARGETS OpenGamesStudio RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/assets/" DESTINATION assets)

add_custom_target(OpenGamesStudio_export
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}"
        --prefix "${STUDIO_BIN_DIR}/export"
    COMMENT "导出 OpenGamesStudio 的干净目录"
)

# ──────────────────────────────────────────────
#  子工程
# ──────────────────────────────────────────────
add_subdirectory(tools/OpenPackager)
add_subdirectory(tools/OpenScripter)
add_subdirectory(tools/OpenWorlder)

# ──────────────────────────────────────────────
#  中枢依赖
# ──────────────────────────────────────────────
# 构建 OpenGamesStudio 时先把三个工具编出来。
# 反向不成立：单独构建某个工具不会牵连 Studio 与其它工具。
add_dependencies(OpenGamesStudio
    OpenPackager
    OpenScripter
    OpenWorlder
)

message(STATUS "OpenGamesStudio 输出目录: ${STUDIO_BIN_DIR}")
message(STATUS "工具输出根目录: ${OPS_TOOLS_DIR}")
