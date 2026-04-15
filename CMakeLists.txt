cmake_minimum_required(VERSION 3.21)
project(AiWinUiRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ai_win_ui WIN32
    src/main.cpp
    src/renderer.cpp
    src/resource_provider.cpp
    src/layout_parser.cpp
)

target_include_directories(ai_win_ui PRIVATE src)

target_compile_definitions(ai_win_ui PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(ai_win_ui
    d2d1
    dwrite
    windowscodecs
)
