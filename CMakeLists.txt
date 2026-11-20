cmake_minimum_required(VERSION 3.21)
project(AiWinUiRenderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(AI_WIN_UI_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resource")

set(AI_WIN_UI_VERSION_MAJOR 0)
set(AI_WIN_UI_VERSION_MINOR 3)
set(AI_WIN_UI_VERSION_PATCH 2)

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
option(AI_WIN_UI_ENABLE_REACT "Build embedded QuickJS-ng React JSX runtime" ON)
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
    src/json_parser.cpp
    src/xml_parser.cpp
    src/layout_parser.cpp
    src/layout_parser_panels.cpp
    src/layout_parser_controls.cpp
    src/ui_panels.cpp
    src/ui_host_input.cpp
    src/ui_host_overlay.cpp
    src/animation_controller.cpp
    src/c_api.cpp
    src/window_chrome.cpp
    src/uia_provider.cpp
    src/mock_market_engine.cpp
    src/keyboard_genie.cpp
    ${CMAKE_SOURCE_DIR}/third_party/pugixml/pugixml.cpp
    ${CMAKE_SOURCE_DIR}/third_party/yyjson/yyjson.c
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
        ${CMAKE_SOURCE_DIR}/third_party/pugixml
        ${CMAKE_SOURCE_DIR}/third_party/yyjson
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
    ole32
    shell32
    uiautomationcore
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

if(AI_WIN_UI_ENABLE_REACT)
    set(QUICKJS_NG_DIR "${CMAKE_SOURCE_DIR}/third_party/quickjs-ng")
    add_library(quickjs_ng STATIC
        "${QUICKJS_NG_DIR}/dtoa.c"
        "${QUICKJS_NG_DIR}/libregexp.c"
        "${QUICKJS_NG_DIR}/libunicode.c"
        "${QUICKJS_NG_DIR}/quickjs.c"
    )
    set_target_properties(quickjs_ng PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS ON
    )
    target_include_directories(quickjs_ng PUBLIC "${QUICKJS_NG_DIR}")
    target_compile_definitions(quickjs_ng PRIVATE
        QUICKJS_NG_BUILD
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0601
        _GNU_SOURCE
        _CRT_SECURE_NO_WARNINGS
    )
    if(MSVC)
        target_compile_options(quickjs_ng PRIVATE
            /utf-8
            /experimental:c11atomics
            /wd4018 /wd4061 /wd4098 /wd4100 /wd4200 /wd4242 /wd4244 /wd4245
            /wd4267 /wd4334 /wd4388 /wd4389 /wd4456 /wd4457 /wd4710
            /wd4711 /wd4819 /wd4820 /wd4996 /wd5045
        )
    endif()
    if(AI_WIN_UI_ENABLE_SKIA AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
        set_property(TARGET quickjs_ng PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    add_library(ai_win_ui_react STATIC
        src/react/react_node_registry.cpp
        src/react/react_js_runtime.cpp
        src/react/react_host_bridge.cpp
    )
    target_link_libraries(ai_win_ui_react PUBLIC ai_win_ui_lib quickjs_ng)
    target_include_directories(ai_win_ui_react PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/react
    )
    target_compile_definitions(ai_win_ui_react PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
    if(MSVC)
        target_compile_options(ai_win_ui_react PRIVATE /utf-8 /wd4819)
    endif()
    if(AI_WIN_UI_ENABLE_SKIA AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
        set_property(TARGET ai_win_ui_react PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

set(AI_WIN_UI_EMBED_ASSETS_ZIP "" CACHE FILEPATH "Optional Stored assets.zip to embed into executables as RT_RCDATA (AI_WIN_UI_ASSETS_ZIP)")

function(ai_win_ui_maybe_embed_assets_zip target_name)
    if(WIN32 AND NOT "${AI_WIN_UI_EMBED_ASSETS_ZIP}" STREQUAL "")
        if(EXISTS "${AI_WIN_UI_EMBED_ASSETS_ZIP}")
            file(TO_CMAKE_PATH "${AI_WIN_UI_EMBED_ASSETS_ZIP}" _embed_zip_path)
            set(_rc_file "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_embedded_assets.rc")
            file(WRITE "${_rc_file}" "AI_WIN_UI_ASSETS_ZIP RCDATA \"${_embed_zip_path}\"\n")
            target_sources(${target_name} PRIVATE "${_rc_file}")
            message(STATUS "[${target_name}] Embedding assets.zip into EXE RT_RCDATA: ${_embed_zip_path}")
        else()
            message(WARNING "AI_WIN_UI_EMBED_ASSETS_ZIP not found: ${AI_WIN_UI_EMBED_ASSETS_ZIP}")
        endif()
    endif()
endfunction()

ai_win_ui_add_app(ai_win_ui src/main.cpp)
ai_win_ui_maybe_embed_assets_zip(ai_win_ui)
if(AI_WIN_UI_ENABLE_REACT)
    target_link_libraries(ai_win_ui PRIVATE ai_win_ui_react)
    target_compile_definitions(ai_win_ui PRIVATE AI_WIN_UI_ENABLE_REACT=1)
    if(MSVC)
        target_link_options(ai_win_ui PRIVATE "/STACK:8388608")
    endif()
endif()

if(AI_WIN_UI_BUILD_SAMPLES)
    # 金融看盘终端 — stock_terminal sample
    ai_win_ui_add_app(stock_terminal samples/stock_terminal/main.cpp)

    # XML 60Hz 100-cell benchmark target
    ai_win_ui_add_app(perf_stress_xml samples/perf_stress_xml/main.cpp)

    # DirtyRegion & Skia text UTF-16 / LRU unit tests
    add_executable(skia_unit_test samples/skia_unit_test/main.cpp)
    target_link_libraries(skia_unit_test PRIVATE ai_win_ui_lib)
    target_include_directories(skia_unit_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    if(AI_WIN_UI_ENABLE_SKIA AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
        target_include_directories(skia_unit_test PRIVATE "${_skia_inc}")
        set_property(TARGET skia_unit_test PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    if(MSVC)
        target_compile_options(skia_unit_test PRIVATE /utf-8 /wd4819)
    endif()

    ai_win_ui_add_app(embed_host samples/embed_host/main.cpp)

    # 便签 — sticky-notes desktop app (multi-file).
    ai_win_ui_add_app(sticky_notes samples/sticky_notes/main.cpp)
    target_sources(sticky_notes PRIVATE
        samples/sticky_notes/note_store.cpp
        samples/sticky_notes/layout_gen.cpp
        samples/sticky_notes/app.rc
    )

    if(AI_WIN_UI_ENABLE_REACT)
        ai_win_ui_add_app(react_demo samples/react_demo/main.cpp)
        ai_win_ui_maybe_embed_assets_zip(react_demo)
        target_link_libraries(react_demo PRIVATE ai_win_ui_react)
        if(MSVC)
            target_link_options(react_demo PRIVATE "/STACK:8388608")
        endif()

        add_executable(react_unit_test samples/react_unit_test/main.cpp)
        target_link_libraries(react_unit_test PRIVATE ai_win_ui_react)
        target_include_directories(react_unit_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/react
        )
        if(MSVC)
            target_compile_options(react_unit_test PRIVATE /utf-8 /wd4819)
            target_link_options(react_unit_test PRIVATE "/STACK:8388608")
        endif()
        if(AI_WIN_UI_ENABLE_SKIA AND AI_WIN_UI_SKIA_PROVIDER STREQUAL "local-sdk")
            set_property(TARGET react_unit_test PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
        add_custom_command(TARGET react_unit_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${AI_WIN_UI_RESOURCE_DIR}"
                "$<TARGET_FILE_DIR:react_unit_test>/resource"
            COMMENT "Copying runtime resources next to react_unit_test"
        )
    endif()
endif()
