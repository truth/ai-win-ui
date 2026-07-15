#pragma once

#include "application.h"
#include "host_options.h"
#include "renderer.h"

#include <Windows.h>

// Wave1 H1 public host surface.
//
// One top-level UI window = one UiHost (HWND + retained tree + chrome + backends).
// Implementation currently lives in main.cpp as class UiHost (single TU to keep
// MSVC incomplete-type / WndProc coupling simple). Wave2–3 H5/H6 will move the
// body into ui_host.cpp + ai_win_ui_lib.
//
// Process-wide multi-host registry: ai_win_ui::Application.
// Deferred open message id: ai_win_ui::kMsgOpenDemoHost.

namespace ai_win_ui {

// Open another host from structured options (prefer over mutating AI_WIN_UI_*).
// Default: in-process secondary window; set options.childProcess for CreateProcess.
bool OpenSecondaryUiHost(HINSTANCE instance, const OpenHostOptions& options);

bool OpenSecondaryUiHost(
    HINSTANCE instance,
    const wchar_t* layoutRelPath,
    const wchar_t* chromeMode = nullptr,
    const wchar_t* sizeWh = nullptr,
    RendererBackend renderer = RendererBackend::Direct2D);

} // namespace ai_win_ui
