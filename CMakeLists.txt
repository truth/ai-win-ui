cmake_minimum_required(VERSION 3.21)
project(AiWinUiRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(AI_WIN_UI_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resource")

add_executable(ai_win_ui WIN32
    src/main.cpp
    src/layout_engine.cpp
    src/renderer.cpp
    src/text_measurer_dwrite.cpp
    src/ui_text_metrics.cpp
    src/resource_provider.cpp
    src/zip_resource_provider.cpp
    src/layout_parser.cpp
)

target_include_directories(ai_win_ui PRIVATE
    src
    third_party/yoga-3.2.1
)

target_compile_definitions(ai_win_ui PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(ai_win_ui
    "$<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/third_party/yoga-3.2.1/build/yoga/Debug/yogacore.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${CMAKE_SOURCE_DIR}/third_party/yoga-3.2.1/build/yoga/Release/yogacore.lib>"
    d2d1
    dwrite
    windowscodecs
)

set_target_properties(ai_win_ui PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:ai_win_ui>"
)

add_custom_command(TARGET ai_win_ui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${AI_WIN_UI_RESOURCE_DIR}"
        "$<TARGET_FILE_DIR:ai_win_ui>/resource"
    COMMENT "Copying runtime resources next to ai_win_ui.exe"
)
