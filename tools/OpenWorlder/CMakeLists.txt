cmake_minimum_required(VERSION 3.20)

project(OpenWorlder LANGUAGES CXX)

# 注意：本工程是 OpenGamesStudio 的子工程，不自己声明 OpenCore。
# 共享的 OpenCore 目标由 Studio 顶层的 add_subdirectory(modules/OpenCore) 提供。

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/project/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    OpenCore
)

set(TARGET_BIN_DIR "${OPS_TOOLS_DIR}/${PROJECT_NAME}")
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TARGET_BIN_DIR}"
)

if(WIN32)

    target_sources(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.rc"
    )

    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/SDL3.dll"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/SDL3_image.dll"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/SDL3_mixer.dll"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/SDL3_ttf.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"

        COMMENT "拷贝 ${PROJECT_NAME} 的运行库与资源"
    )

endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/assets/" DESTINATION assets)

add_custom_target(${PROJECT_NAME}_export
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}"
        --prefix "${TARGET_BIN_DIR}/export"
    COMMENT "导出 ${PROJECT_NAME} 的干净目录"
)
