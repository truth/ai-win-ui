#pragma once

#include <functional>
#include <string>

class IRenderer;
class ITextMeasurer;
class IResourceProvider;
class ILayoutEngine;

using UIEventHandler = std::function<void()>;
using UIEventResolver = std::function<UIEventHandler(const std::string&)>;

struct UIContext {
    IRenderer* renderer = nullptr;
    ITextMeasurer* textMeasurer = nullptr;
    IResourceProvider* resourceProvider = nullptr;
    ILayoutEngine* layoutEngine = nullptr;
    UIEventResolver eventResolver;
};
