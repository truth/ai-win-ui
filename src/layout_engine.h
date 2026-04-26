#pragma once

#include "graphics_types.h"

#include <memory>
#include <vector>

class UIElement;

struct LayoutSpacing {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

enum class StackAlignItems {
    Stretch,
    Start,
    Center,
    End
};

enum class StackDirection {
    Column,
    Row
};

enum class StackWrap {
    NoWrap,
    Wrap
};

enum class StackJustifyContent {
    Start,
    Center,
    End,
    SpaceBetween
};

struct StackLayoutStyle {
    StackDirection direction = StackDirection::Column;
    StackWrap wrap = StackWrap::NoWrap;
    LayoutSpacing padding{};
    LayoutSpacing border{};
    float spacing = 0.0f;
    StackAlignItems alignItems = StackAlignItems::Stretch;
    StackJustifyContent justifyContent = StackJustifyContent::Start;
};

struct StackLayoutChild {
    UIElement* element = nullptr;
    LayoutSpacing margin{};
    LayoutSpacing border{};
};

class ILayoutEngine {
public:
    virtual ~ILayoutEngine() = default;
    virtual Size MeasureStack(const StackLayoutStyle& style,
                              const std::vector<StackLayoutChild>& children,
                              float availableWidth,
                              float availableHeight) = 0;
    virtual void ArrangeStack(const StackLayoutStyle& style,
                              const std::vector<StackLayoutChild>& children,
                              const Rect& bounds) = 0;
};

std::unique_ptr<ILayoutEngine> CreateDefaultLayoutEngine();
std::unique_ptr<ILayoutEngine> CreateYogaLayoutEngine();
