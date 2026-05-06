cmake_minimum_required(VERSION 3.21)
project(AiWinUiRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(AI_WIN_UI_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resource")

# Auto-detect bundled Skia SDK so SVG support works zero-config when
# third_party/skia-m124 is present with prebuilt libs.
set(_ai_win_ui_default_skia_dir "${CMAKE_SOURCE_DIR}/third_party/skia-m124")
set(_ai_win_ui_skia_autodetected OFF)
if(EXISTS "${_ai_win_ui_default_skia_dir}/out/Debug-x64/skia.lib"
   AND EXISTS "${_ai_win_ui_default_skia_dir}/out/Release-x64/skia.lib"
   AND EXISTS "${_ai_win_ui_default_skia_dir}/modules/svg/include/SkSVGDOM.h")
    set(_ai_win_ui_skia_autodetected ON)
    message(STATUS "Auto-detected Skia SDK at ${_ai_win_ui_default_skia_dir}; enabling Skia backend.")
endif()

option(AI_WIN_UI_ENABLE_SKIA "Build with Skia renderer backend (required for SVG)." ${_ai_win_ui_skia_autodetected})
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

add_executable(ai_win_ui WIN32
    src/main.cpp
    src/box_decoration.cpp
    src/style.cpp
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
)

set(AI_WIN_UI_YOGA_BUILD_DIR "${CMAKE_SOURCE_DIR}/third_party/yoga-3.2.1/build" CACHE PATH "Default Yoga build directory used by the app target.")
set(AI_WIN_UI_YOGA_STATICCRT_BUILD_DIR "${CMAKE_SOURCE_DIR}/third_party/yoga-3.2.1/build-staticcrt" CACHE PATH "Yoga build directory compiled with static MSVC runtime for local Skia SDK builds.")

target_include_directories(ai_win_ui PRIVATE
    src
    third_party/yoga-3.2.1
)

if(AI_WIN_UI_ENABLE_SKIA)
    target_compile_definitions(ai_win_ui PRIVATE AI_WIN_UI_ENABLE_SKIA=1)

    if(AI_WIN_UI_SKIA_PROVIDER STREQUAL "vcpkg")
        find_package(unofficial-skia CONFIG REQUIRED)
        target_link_libraries(ai_win_ui PRIVATE unofficial::skia::skia)
        target_compile_definitions(ai_win_ui PRIVATE AI_WIN_UI_HAS_VCPKG_SKIA=1)
        message(STATUS "AI_WIN_UI_ENABLE_SKIA is ON and Skia will be resolved from vcpkg via unofficial::skia::skia.")
    elseif(AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
        if(NOT AI_WIN_UI_SKIA_DIR)
            message(FATAL_ERROR "AI_WIN_UI_SKIA_PROVIDER=local-sdk requires AI_WIN_UI_SKIA_DIR to point at an unpacked Skia package root.")
        endif()

        set(_ai_win_ui_skia_include_dir "${AI_WIN_UI_SKIA_INCLUDE_DIR}")
        if(NOT _ai_win_ui_skia_include_dir)
            set(_ai_win_ui_skia_include_dir "${AI_WIN_UI_SKIA_DIR}")
        endif()

        set(_ai_win_ui_skia_library_dir "${AI_WIN_UI_SKIA_LIBRARY_DIR}")
        if(NOT _ai_win_ui_skia_library_dir)
            set(_ai_win_ui_skia_library_dir "${AI_WIN_UI_SKIA_DIR}/out")
        endif()

        target_include_directories(ai_win_ui PRIVATE
            "${_ai_win_ui_skia_include_dir}"
        )

        # The published aseprite/skia Windows packages are built with /MT and /MTd.
        set_property(TARGET ai_win_ui PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        target_link_options(ai_win_ui PRIVATE
            "/alternatename:inflate=Cr_z_inflate"
            "/alternatename:inflateEnd=Cr_z_inflateEnd"
            "/alternatename:inflateInit2_=Cr_z_inflateInit2_"
            "/alternatename:inflateReset=Cr_z_inflateReset"
        )

        if(EXISTS "${_ai_win_ui_skia_library_dir}/Debug-x64/skia.lib" AND EXISTS "${_ai_win_ui_skia_library_dir}/Release-x64/skia.lib")
            file(GLOB _ai_win_ui_skia_debug_libs LIST_DIRECTORIES false "${_ai_win_ui_skia_library_dir}/Debug-x64/*.lib")
            if(NOT _ai_win_ui_skia_debug_libs)
                message(FATAL_ERROR "No .lib files found in ${_ai_win_ui_skia_library_dir}/Debug-x64 for AI_WIN_UI_SKIA_PROVIDER=local-sdk.")
            endif()

            set(_ai_win_ui_skia_lib_names "")
            foreach(_ai_win_ui_skia_lib_path IN LISTS _ai_win_ui_skia_debug_libs)
                get_filename_component(_ai_win_ui_skia_lib_name "${_ai_win_ui_skia_lib_path}" NAME_WE)
                list(APPEND _ai_win_ui_skia_lib_names "${_ai_win_ui_skia_lib_name}")
            endforeach()
            list(REMOVE_DUPLICATES _ai_win_ui_skia_lib_names)

            target_link_directories(ai_win_ui PRIVATE
                "$<$<CONFIG:Debug>:${_ai_win_ui_skia_library_dir}/Debug-x64>"
                "$<$<NOT:$<CONFIG:Debug>>:${_ai_win_ui_skia_library_dir}/Release-x64>"
            )
            target_link_libraries(ai_win_ui PRIVATE ${_ai_win_ui_skia_lib_names})
        elseif(EXISTS "${_ai_win_ui_skia_library_dir}/skia.lib")
            target_link_libraries(ai_win_ui PRIVATE
                "${_ai_win_ui_skia_library_dir}/skia.lib"
            )
        else()
            message(FATAL_ERROR
                "Could not find skia.lib for AI_WIN_UI_SKIA_PROVIDER=local-sdk. "
                "Checked '${_ai_win_ui_skia_library_dir}/Debug-x64/skia.lib', "
                "'${_ai_win_ui_skia_library_dir}/Release-x64/skia.lib', and "
                "'${_ai_win_ui_skia_library_dir}/skia.lib'.")
        endif()

        target_compile_definitions(ai_win_ui PRIVATE AI_WIN_UI_HAS_LOCAL_SKIA=1)
        message(STATUS "AI_WIN_UI_ENABLE_SKIA is ON and Skia will be resolved from a local SDK rooted at ${AI_WIN_UI_SKIA_DIR}.")
    elseif(AI_WIN_UI_SKIA_PROVIDER STREQUAL "none")
        message(STATUS "AI_WIN_UI_ENABLE_SKIA is ON. Runtime backend selection is ready, but no Skia package provider is configured yet.")
    else()
        message(FATAL_ERROR "Unsupported AI_WIN_UI_SKIA_PROVIDER='${AI_WIN_UI_SKIA_PROVIDER}'. Expected one of: none, vcpkg, local-sdk.")
    endif()
endif()

set(_ai_win_ui_yoga_build_dir "${AI_WIN_UI_YOGA_BUILD_DIR}")
if(AI_WIN_UI_ENABLE_SKIA AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
    set(_ai_win_ui_yoga_build_dir "${AI_WIN_UI_YOGA_STATICCRT_BUILD_DIR}")
endif()

target_compile_definitions(ai_win_ui PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(ai_win_ui PRIVATE
    "$<$<CONFIG:Debug>:${_ai_win_ui_yoga_build_dir}/yoga/Debug/yogacore.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${_ai_win_ui_yoga_build_dir}/yoga/Release/yogacore.lib>"
    d2d1
    dwrite
    imm32
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
