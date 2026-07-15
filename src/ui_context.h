#pragma once

#include <functional>
#include <Windows.h>
#include <string>

class IRenderer;
class ITextMeasurer;
class IResourceProvider;
class ILayoutEngine;
class Theme;

using UIEventHandler = std::function<void()>;
using UIEventResolver = std::function<UIEventHandler(const std::string&)>;

struct UIContext {
    IRenderer* renderer = nullptr;
    ITextMeasurer* textMeasurer = nullptr;
    IResourceProvider* resourceProvider = nullptr;
    ILayoutEngine* layoutEngine = nullptr;
    Theme* theme = nullptr;
    UIEventResolver eventResolver;
    UINT dpi = 96;
    float dpiScale = 1.0f;
    // Client-area size of the host window (DIP-free pixels). Used by overlay popups.
    float viewportWidth = 0.0f;
    float viewportHeight = 0.0f;
    // False while the top-level window is inactive (custom chrome dims caption).
    bool windowActive = true;
};
