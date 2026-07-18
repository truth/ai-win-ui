#pragma once

// Internal host surface. Embedders should include <ai_win_ui/host.h> instead.
//
// OpenSecondaryUiHost remains for in-tree demos that already include this header.

#include "host_options.h"
#include "renderer.h"

#include <Windows.h>

namespace ai_win_ui {

bool OpenSecondaryUiHost(HINSTANCE instance, const OpenHostOptions& options);

bool OpenSecondaryUiHost(
    HINSTANCE instance,
    const wchar_t* layoutRelPath,
    const wchar_t* chromeMode = nullptr,
    const wchar_t* sizeWh = nullptr,
    RendererBackend renderer = RendererBackend::Direct2D);

void InitializeProcessDpiAwareness();

} // namespace ai_win_ui
