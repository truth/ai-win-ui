#pragma once

#include "renderer.h"

#include <string>

// Structured options for opening a UI host window (CLI env still supported).
// Used by multi-window gallery / shaped demos / future embed API (H1/H2).
struct OpenHostOptions {
    std::wstring layout;     // resource path e.g. layouts/ui.xml
    std::wstring chrome;     // empty/system => system frame; custom; layered
    std::wstring size;       // empty => default; e.g. 1100x750
    RendererBackend renderer = RendererBackend::Direct2D;
    bool childProcess = false; // true => CreateProcess instead of in-process
    HWND parent = nullptr;   // Wave3: WS_CHILD host when non-null
};
