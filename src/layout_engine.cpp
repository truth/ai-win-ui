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

YGFlexDirection ToYogaDirection(StackDirection direction) {
    switch (direction) {
        case StackDirection::Row:
            return YGFlexDirectionRow;
        case StackDirection::Column:
        default:
            return YGFlexDirectionColumn;
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

BuiltYogaLayout BuildStackLayout(const StackLayoutStyle& style,
                                 const std::vector<StackLayoutChild>& children,
                                 float availableWidth,
                                 float availableHeight,
                                 bool exactWidth,
                                 bool exactHeight) {
    BuiltYogaLayout layout;
    layout.root = YogaTree(YGNodeNew());
    if (!layout.root) {
        return layout;
    }

    YGNodeRef root = layout.root.get();
    YGNodeStyleSetFlexDirection(root, ToYogaDirection(style.direction));
    YGNodeStyleSetAlignItems(root, ToYogaAlign(style.alignItems));
    YGNodeStyleSetJustifyContent(root, ToYogaJustify(style.justifyContent));
    YGNodeStyleSetGap(root, style.direction == StackDirection::Row ? YGGutterColumn : YGGutterRow, style.spacing);
    ApplyPadding(root, style.padding);

    const float clampedWidth = std::max(0.0f, availableWidth);
    const float clampedHeight = std::max(0.0f, availableHeight);
    const float contentWidth =
        std::max(0.0f, clampedWidth - style.padding.left - style.padding.right);
    const float contentHeight =
        std::max(0.0f, clampedHeight - style.padding.top - style.padding.bottom);

    if (exactWidth) {
        YGNodeStyleSetWidth(root, clampedWidth);
    }
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
        const float availableChildHeight =
            std::max(0.0f, contentHeight - child.margin.top - child.margin.bottom);
        const bool stretchCrossAxis = style.alignItems == StackAlignItems::Stretch &&
            ((style.direction == StackDirection::Column && !child.element->HasFixedWidth()) ||
             (style.direction == StackDirection::Row && !child.element->HasFixedHeight()));
        const float measuredWidth = style.direction == StackDirection::Column
            ? (stretchCrossAxis ? availableChildWidth : child.element->GetPreferredWidth(availableChildWidth))
            : child.element->GetPreferredWidth(availableChildWidth);
        const float flexGrow = child.element->FlexGrow();
        const float flexShrink = child.element->FlexShrink();
        const bool hasFlexBasis = child.element->HasFlexBasis();

        child.element->Measure(measuredWidth, availableChildHeight);
        const float measuredHeight = child.element->DesiredSize().height;

        if (style.direction == StackDirection::Column) {
            if (!stretchCrossAxis) {
                YGNodeStyleSetWidth(childNode, measuredWidth);
            }
        } else {
            YGNodeStyleSetWidth(childNode, measuredWidth);
            if (stretchCrossAxis) {
                YGNodeStyleSetHeight(childNode, availableChildHeight);
            } else {
                YGNodeStyleSetHeight(childNode, measuredHeight);
            }
        }
        if (flexGrow > 0.0f) {
            YGNodeStyleSetFlexGrow(childNode, flexGrow);
        }
        if (flexShrink > 0.0f) {
            YGNodeStyleSetFlexShrink(childNode, flexShrink);
        }

        if (flexGrow > 0.0f || flexShrink > 0.0f || hasFlexBasis) {
            const float intrinsicMainAxis = style.direction == StackDirection::Column ? measuredHeight : measuredWidth;
            const float flexBasis = hasFlexBasis ? child.element->FlexBasis() : intrinsicMainAxis;
            YGNodeStyleSetFlexBasis(childNode, std::max(0.0f, flexBasis));
            if (style.direction == StackDirection::Column) {
                YGNodeStyleSetHeight(childNode, measuredHeight);
            }
        } else if (style.direction == StackDirection::Column) {
            YGNodeStyleSetHeight(childNode, measuredHeight);
        }

        YGNodeInsertChild(root, childNode, YGNodeGetChildCount(root));
        layout.childNodes.push_back(childNode);
        layout.elements.push_back(child.element);
    }

    return layout;
}

class YogaLayoutEngine final : public ILayoutEngine {
public:
    Size MeasureStack(const StackLayoutStyle& style,
                      const std::vector<StackLayoutChild>& children,
                      float availableWidth,
                      float availableHeight) override {
        BuiltYogaLayout layout =
            BuildStackLayout(style, children, availableWidth, availableHeight, true, false);
        if (!layout.root) {
            return {};
        }

        YGNodeCalculateLayout(
            layout.root.get(),
            std::max(0.0f, availableWidth),
            YGUndefined,
            YGDirectionLTR);
        return Size{
            YGNodeLayoutGetWidth(layout.root.get()),
            YGNodeLayoutGetHeight(layout.root.get())
        };
    }

    void ArrangeStack(const StackLayoutStyle& style,
                      const std::vector<StackLayoutChild>& children,
                      const Rect& bounds) override {
        BuiltYogaLayout layout =
            BuildStackLayout(style, children, bounds.Width(), bounds.Height(), true, true);
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
