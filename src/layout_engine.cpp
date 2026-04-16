#include "layout_engine.h"

#include "ui.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <yoga/Yoga.h>

namespace {

class YogaTreeDeleter {
public:
    void operator()(YGNodeRef node) const {
        if (node) {
            YGNodeFreeRecursive(node);
        }
    }
};

using YogaTree = std::unique_ptr<std::remove_pointer_t<YGNodeRef>, YogaTreeDeleter>;

YGAlign ToYogaAlign(StackAlignItems align) {
    switch (align) {
        case StackAlignItems::Start:
            return YGAlignFlexStart;
        case StackAlignItems::Center:
            return YGAlignCenter;
        case StackAlignItems::End:
            return YGAlignFlexEnd;
        case StackAlignItems::Stretch:
        default:
            return YGAlignStretch;
    }
}

YGJustify ToYogaJustify(StackJustifyContent justify) {
    switch (justify) {
        case StackJustifyContent::Center:
            return YGJustifyCenter;
        case StackJustifyContent::End:
            return YGJustifyFlexEnd;
        case StackJustifyContent::SpaceBetween:
            return YGJustifySpaceBetween;
        case StackJustifyContent::Start:
        default:
            return YGJustifyFlexStart;
    }
}

void ApplyPadding(YGNodeRef node, const LayoutSpacing& padding) {
    YGNodeStyleSetPadding(node, YGEdgeLeft, padding.left);
    YGNodeStyleSetPadding(node, YGEdgeTop, padding.top);
    YGNodeStyleSetPadding(node, YGEdgeRight, padding.right);
    YGNodeStyleSetPadding(node, YGEdgeBottom, padding.bottom);
}

void ApplyMargin(YGNodeRef node, const LayoutSpacing& margin) {
    YGNodeStyleSetMargin(node, YGEdgeLeft, margin.left);
    YGNodeStyleSetMargin(node, YGEdgeTop, margin.top);
    YGNodeStyleSetMargin(node, YGEdgeRight, margin.right);
    YGNodeStyleSetMargin(node, YGEdgeBottom, margin.bottom);
}

struct BuiltYogaLayout {
    YogaTree root;
    std::vector<YGNodeRef> childNodes;
    std::vector<UIElement*> elements;
};

BuiltYogaLayout BuildVerticalStackLayout(const StackLayoutStyle& style,
                                         const std::vector<StackLayoutChild>& children,
                                         float availableWidth,
                                         float availableHeight,
                                         bool exactHeight) {
    BuiltYogaLayout layout;
    layout.root = YogaTree(YGNodeNew());
    if (!layout.root) {
        return layout;
    }

    YGNodeRef root = layout.root.get();
    YGNodeStyleSetFlexDirection(root, YGFlexDirectionColumn);
    YGNodeStyleSetAlignItems(root, ToYogaAlign(style.alignItems));
    YGNodeStyleSetJustifyContent(root, ToYogaJustify(style.justifyContent));
    YGNodeStyleSetGap(root, YGGutterRow, style.spacing);
    ApplyPadding(root, style.padding);

    const float clampedWidth = std::max(0.0f, availableWidth);
    const float clampedHeight = std::max(0.0f, availableHeight);
    const float contentWidth =
        std::max(0.0f, clampedWidth - style.padding.left - style.padding.right);

    YGNodeStyleSetWidth(root, clampedWidth);
    if (exactHeight) {
        YGNodeStyleSetHeight(root, clampedHeight);
    }

    layout.childNodes.reserve(children.size());
    layout.elements.reserve(children.size());

    for (const auto& child : children) {
        if (!child.element) {
            continue;
        }

        YGNodeRef childNode = YGNodeNew();
        if (!childNode) {
            continue;
        }

        ApplyMargin(childNode, child.margin);

        const float availableChildWidth =
            std::max(0.0f, contentWidth - child.margin.left - child.margin.right);
        const bool stretchWidth =
            style.alignItems == StackAlignItems::Stretch && !child.element->HasFixedWidth();
        const float measuredWidth = stretchWidth
            ? availableChildWidth
            : child.element->GetPreferredWidth(availableChildWidth);

        child.element->Measure(measuredWidth, clampedHeight);
        const float measuredHeight = child.element->DesiredSize().height;

        if (!stretchWidth) {
            YGNodeStyleSetWidth(childNode, measuredWidth);
        }
        YGNodeStyleSetHeight(childNode, measuredHeight);

        YGNodeInsertChild(root, childNode, YGNodeGetChildCount(root));
        layout.childNodes.push_back(childNode);
        layout.elements.push_back(child.element);
    }

    return layout;
}

class YogaLayoutEngine final : public ILayoutEngine {
public:
    float MeasureVerticalStack(const StackLayoutStyle& style,
                               const std::vector<StackLayoutChild>& children,
                               float availableWidth,
                               float availableHeight) override {
        BuiltYogaLayout layout =
            BuildVerticalStackLayout(style, children, availableWidth, availableHeight, false);
        if (!layout.root) {
            return 0.0f;
        }

        YGNodeCalculateLayout(
            layout.root.get(),
            std::max(0.0f, availableWidth),
            YGUndefined,
            YGDirectionLTR);
        return YGNodeLayoutGetHeight(layout.root.get());
    }

    void ArrangeVerticalStack(const StackLayoutStyle& style,
                              const std::vector<StackLayoutChild>& children,
                              const Rect& bounds) override {
        BuiltYogaLayout layout =
            BuildVerticalStackLayout(style, children, bounds.Width(), bounds.Height(), true);
        if (!layout.root) {
            return;
        }

        YGNodeCalculateLayout(layout.root.get(), bounds.Width(), bounds.Height(), YGDirectionLTR);

        for (size_t i = 0; i < layout.childNodes.size(); ++i) {
            UIElement* element = layout.elements[i];
            YGNodeRef childNode = layout.childNodes[i];
            const float left = bounds.left + YGNodeLayoutGetLeft(childNode);
            const float top = bounds.top + YGNodeLayoutGetTop(childNode);
            const float width = YGNodeLayoutGetWidth(childNode);
            const float height = YGNodeLayoutGetHeight(childNode);
            element->Arrange(Rect::Make(left, top, left + width, top + height));
        }
    }
};

} // namespace

std::unique_ptr<ILayoutEngine> CreateDefaultLayoutEngine() {
    return CreateYogaLayoutEngine();
}

std::unique_ptr<ILayoutEngine> CreateYogaLayoutEngine() {
    return std::make_unique<YogaLayoutEngine>();
}
