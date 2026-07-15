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

// Uniform multi-column grid (Yoga flex-wrap row under the hood; Yoga 3.x has no CSS Grid).
// Matches legacy GridPanel: N equal columns, fixed rowHeight, cellSpacing as row/column gap.
struct GridLayoutStyle {
    int columns = 3;
    LayoutSpacing padding{};
    LayoutSpacing border{};
    float cellSpacing = 0.0f;
    float rowHeight = 120.0f;
};

using GridLayoutChild = StackLayoutChild;

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

    virtual Size MeasureGrid(const GridLayoutStyle& style,
                             const std::vector<GridLayoutChild>& children,
                             float availableWidth,
                             float availableHeight) = 0;
    virtual void ArrangeGrid(const GridLayoutStyle& style,
                             const std::vector<GridLayoutChild>& children,
                             const Rect& bounds) = 0;
};

std::unique_ptr<ILayoutEngine> CreateDefaultLayoutEngine();
std::unique_ptr<ILayoutEngine> CreateYogaLayoutEngine();
