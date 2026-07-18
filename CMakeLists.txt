cmake_minimum_required(VERSION 3.21)
project(AiWinUiRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(AI_WIN_UI_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resource")

set(AI_WIN_UI_VERSION_MAJOR 0)
set(AI_WIN_UI_VERSION_MINOR 3)
set(AI_WIN_UI_VERSION_PATCH 1)

# Auto-detect bundled Skia SDK (gitignored; present on local dev machines).
set(_ai_win_ui_default_skia_dir "${CMAKE_SOURCE_DIR}/third_party/skia-m124")
set(_ai_win_ui_skia_autodetected OFF)
if(EXISTS "${_ai_win_ui_default_skia_dir}/out/Debug-x64/skia.lib"
   AND EXISTS "${_ai_win_ui_default_skia_dir}/out/Release-x64/skia.lib"
   AND EXISTS "${_ai_win_ui_default_skia_dir}/modules/svg/include/SkSVGDOM.h")
    set(_ai_win_ui_skia_autodetected ON)
    message(STATUS "Auto-detected Skia SDK at ${_ai_win_ui_default_skia_dir}; enabling Skia backend.")
endif()

option(AI_WIN_UI_ENABLE_SKIA "Build with Skia renderer backend (required for SVG)." ${_ai_win_ui_skia_autodetected})
option(AI_WIN_UI_BUILD_SAMPLES "Build samples/embed_host" ON)
set(AI_WIN_UI_SKIA_PROVIDER "none" CACHE STRING "How Skia should be discovered when AI_WIN_UI_ENABLE_SKIA is ON.")
set_property(CACHE AI_WIN_UI_SKIA_PROVIDER PROPERTY STRINGS none vcpkg local-sdk)
set(AI_WIN_UI_SKIA_DIR "" CACHE PATH "Optional root directory of a local Skia SDK or prebuilt package.")
if(_ai_win_ui_skia_autodetected AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "none")
    set(AI_WIN_UI_SKIA_PROVIDER "local-sdk" CACHE STRING "" FORCE)
endif()
if(_ai_win_ui_skia_autodetected AND NOT AI_WIN_UI_SKIA_DIR)
    set(AI_WIN_UI_SKIA_DIR "${_ai_win_ui_default_skia_dir}" CACHE PATH "" FORCE)
endif()
set(AI_WIN_UI_SKIA_INCLUDE_DIR "" CACHE PATH "Optional override for the Skia include root when using AI_WIN_UI_SKIA_PROVIDER=local-sdk.")
set(AI_WIN_UI_SKIA_LIBRARY_DIR "" CACHE PATH "Optional override for the directory containing skia.lib when using AI_WIN_UI_SKIA_PROVIDER=local-sdk.")

set(AI_WIN_UI_YOGA_BUILD_DIR "${CMAKE_SOURCE_DIR}/third_party/yoga-3.2.1/build" CACHE PATH "Default Yoga build directory.")
set(AI_WIN_UI_YOGA_STATICCRT_BUILD_DIR "${CMAKE_SOURCE_DIR}/third_party/yoga-3.2.1/build-staticcrt" CACHE PATH "Yoga /MT build for local Skia.")

set(_ai_win_ui_yoga_build_dir "${AI_WIN_UI_YOGA_BUILD_DIR}")
if(AI_WIN_UI_ENABLE_SKIA AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
    set(_ai_win_ui_yoga_build_dir "${AI_WIN_UI_YOGA_STATICCRT_BUILD_DIR}")
endif()

set(AI_WIN_UI_LIB_SOURCES
    src/ui_host.cpp
    src/application.cpp
    src/box_decoration.cpp
    src/style.cpp
    src/style_catalog.cpp
    src/theme.cpp
    src/layout_engine.cpp
    src/renderer.cpp
    src/renderer_skia.cpp
    src/skia_text_layout.cpp
    src/text_measurer.cpp
    src/text_measurer_dwrite.cpp
    src/text_measurer_skia.cpp
    src/ui_text_metrics.cpp
    src/resource_provider.cpp
    src/zip_resource_provider.cpp
    src/layout_parser.cpp
    src/window_chrome.cpp
)

add_library(ai_win_ui_lib STATIC ${AI_WIN_UI_LIB_SOURCES})
add_library(ai_win_ui::lib ALIAS ai_win_ui_lib)

target_include_directories(ai_win_ui_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party/yoga-3.2.1
)

target_compile_definitions(ai_win_ui_lib PUBLIC
    UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
)

if(MSVC)
    target_compile_options(ai_win_ui_lib PRIVATE /utf-8 /wd4819)
endif()

if(AI_WIN_UI_ENABLE_SKIA)
    target_compile_definitions(ai_win_ui_lib PUBLIC AI_WIN_UI_ENABLE_SKIA=1)

    if(AI_WIN_UI_SKIA_PROVIDER STREQUAL "vcpkg")
        find_package(unofficial-skia CONFIG REQUIRED)
        target_link_libraries(ai_win_ui_lib PUBLIC unofficial::skia::skia)
        target_compile_definitions(ai_win_ui_lib PUBLIC AI_WIN_UI_HAS_VCPKG_SKIA=1)
        message(STATUS "Skia: vcpkg")
    elseif(AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
        if(NOT AI_WIN_UI_SKIA_DIR)
            message(FATAL_ERROR "AI_WIN_UI_SKIA_PROVIDER=local-sdk requires AI_WIN_UI_SKIA_DIR.")
        endif()
        set(_skia_inc "${AI_WIN_UI_SKIA_INCLUDE_DIR}")
        if(NOT _skia_inc)
            set(_skia_inc "${AI_WIN_UI_SKIA_DIR}")
        endif()
        set(_skia_libdir "${AI_WIN_UI_SKIA_LIBRARY_DIR}")
        if(NOT _skia_libdir)
            set(_skia_libdir "${AI_WIN_UI_SKIA_DIR}/out")
        endif()

        target_include_directories(ai_win_ui_lib PRIVATE "${_skia_inc}")
        set_property(TARGET ai_win_ui_lib PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        target_link_options(ai_win_ui_lib INTERFACE
            "/alternatename:inflate=Cr_z_inflate"
            "/alternatename:inflateEnd=Cr_z_inflateEnd"
            "/alternatename:inflateInit2_=Cr_z_inflateInit2_"
            "/alternatename:inflateReset=Cr_z_inflateReset"
        )

        if(EXISTS "${_skia_libdir}/Debug-x64/skia.lib" AND EXISTS "${_skia_libdir}/Release-x64/skia.lib")
            file(GLOB _skia_dbg LIST_DIRECTORIES false "${_skia_libdir}/Debug-x64/*.lib")
            file(GLOB _skia_rel LIST_DIRECTORIES false "${_skia_libdir}/Release-x64/*.lib")
            set(_dbg_names "")
            foreach(_p IN LISTS _skia_dbg)
                get_filename_component(_n "${_p}" NAME_WE)
                list(APPEND _dbg_names "${_n}")
            endforeach()
            list(REMOVE_DUPLICATES _dbg_names)
            set(_rel_names "")
            foreach(_p IN LISTS _skia_rel)
                get_filename_component(_n "${_p}" NAME_WE)
                list(APPEND _rel_names "${_n}")
            endforeach()
            list(REMOVE_DUPLICATES _rel_names)
            target_link_directories(ai_win_ui_lib PUBLIC
                "$<$<CONFIG:Debug>:${_skia_libdir}/Debug-x64>"
                "$<$<NOT:$<CONFIG:Debug>>:${_skia_libdir}/Release-x64>"
            )
            string(REPLACE ";" "$<SEMICOLON>" _dbg_genex "${_dbg_names}")
            string(REPLACE ";" "$<SEMICOLON>" _rel_genex "${_rel_names}")
            target_link_libraries(ai_win_ui_lib PUBLIC
                "$<$<CONFIG:Debug>:${_dbg_genex}>"
                "$<$<NOT:$<CONFIG:Debug>>:${_rel_genex}>"
            )
        elseif(EXISTS "${_skia_libdir}/skia.lib")
            target_link_libraries(ai_win_ui_lib PUBLIC "${_skia_libdir}/skia.lib")
        else()
            message(FATAL_ERROR "Could not find skia.lib under ${_skia_libdir}")
        endif()
        target_compile_definitions(ai_win_ui_lib PUBLIC AI_WIN_UI_HAS_LOCAL_SKIA=1)
        message(STATUS "Skia: local-sdk at ${AI_WIN_UI_SKIA_DIR}")
    else()
        message(STATUS "Skia enabled, provider=${AI_WIN_UI_SKIA_PROVIDER}")
    endif()
else()
    message(STATUS "AI_WIN_UI_ENABLE_SKIA=OFF (Direct2D only — CI path)")
endif()

target_link_libraries(ai_win_ui_lib PUBLIC
    "$<$<CONFIG:Debug>:${_ai_win_ui_yoga_build_dir}/yoga/Debug/yogacore.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${_ai_win_ui_yoga_build_dir}/yoga/Release/yogacore.lib>"
    d2d1
    dwrite
    dwmapi
    imm32
    windowscodecs
)

function(ai_win_ui_add_app target_name source_file)
    add_executable(${target_name} WIN32 ${source_file})
    target_link_libraries(${target_name} PRIVATE ai_win_ui_lib)
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8 /wd4819)
    endif()
    # Match static CRT when Skia local-sdk forces /MT on the library.
    if(AI_WIN_UI_ENABLE_SKIA AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
        set_property(TARGET ${target_name} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    set_target_properties(${target_name} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target_name}>"
    )
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${AI_WIN_UI_RESOURCE_DIR}"
            "$<TARGET_FILE_DIR:${target_name}>/resource"
        COMMENT "Copying runtime resources next to ${target_name}"
    )
endfunction()

ai_win_ui_add_app(ai_win_ui src/main.cpp)

if(AI_WIN_UI_BUILD_SAMPLES)
    ai_win_ui_add_app(embed_host samples/embed_host/main.cpp)
endif()
