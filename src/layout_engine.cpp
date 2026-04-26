#include "layout_engine.h"

#include "ui.h"

#include <algorithm>
#include <cmath>
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

constexpr float kUnboundedMeasure = 4096.0f;

struct LeafMeasureContext {
    UIElement* element = nullptr;
    bool hasCache = false;
    float width = 0.0f;
    float height = 0.0f;
    YGMeasureMode widthMode = YGMeasureModeUndefined;
    YGMeasureMode heightMode = YGMeasureModeUndefined;
    Size measured{};
};

float ResolveMeasureConstraint(float value, YGMeasureMode mode) {
    if (mode == YGMeasureModeUndefined || std::isnan(value)) {
        return kUnboundedMeasure;
    }
    return std::max(0.0f, value);
}

Size MeasureLeafWithContext(LeafMeasureContext& context,
                            float width,
                            YGMeasureMode widthMode,
                            float height,
                            YGMeasureMode heightMode) {
    if (!context.element) {
        return {};
    }

    if (context.hasCache &&
        context.width == width &&
        context.height == height &&
        context.widthMode == widthMode &&
        context.heightMode == heightMode) {
        return context.measured;
    }

    const float availableWidth = ResolveMeasureConstraint(width, widthMode);
    const float availableHeight = ResolveMeasureConstraint(height, heightMode);
    context.measured = context.element->MeasureForLayout(availableWidth, availableHeight);
    context.width = width;
    context.height = height;
    context.widthMode = widthMode;
    context.heightMode = heightMode;
    context.hasCache = true;
    return context.measured;
}

YGSize MeasureLeafNode(YGNodeConstRef node,
                       float width,
                       YGMeasureMode widthMode,
                       float height,
                       YGMeasureMode heightMode) {
    auto* context = static_cast<LeafMeasureContext*>(YGNodeGetContext(node));
    if (!context || !context->element) {
        return YGSize{0.0f, 0.0f};
    }

    Size measured = MeasureLeafWithContext(*context, width, widthMode, height, heightMode);

    if (widthMode == YGMeasureModeExactly && !std::isnan(width)) {
        measured.width = std::max(0.0f, width);
    } else if (widthMode == YGMeasureModeAtMost && !std::isnan(width)) {
        measured.width = std::min(measured.width, std::max(0.0f, width));
    }

    if (heightMode == YGMeasureModeExactly && !std::isnan(height)) {
        measured.height = std::max(0.0f, height);
    } else if (heightMode == YGMeasureModeAtMost && !std::isnan(height)) {
        measured.height = std::min(measured.height, std::max(0.0f, height));
    }

    return YGSize{measured.width, measured.height};
}

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

YGAlign ToYogaSelfAlign(UIElement::SelfAlign align) {
    switch (align) {
        case UIElement::SelfAlign::Stretch:
            return YGAlignStretch;
        case UIElement::SelfAlign::Start:
            return YGAlignFlexStart;
        case UIElement::SelfAlign::Center:
            return YGAlignCenter;
        case UIElement::SelfAlign::End:
            return YGAlignFlexEnd;
        case UIElement::SelfAlign::Auto:
        default:
            return YGAlignAuto;
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

YGWrap ToYogaWrap(StackWrap wrap) {
    switch (wrap) {
        case StackWrap::Wrap:
            return YGWrapWrap;
        case StackWrap::NoWrap:
        default:
            return YGWrapNoWrap;
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

void ApplyBorder(YGNodeRef node, const LayoutSpacing& border) {
    YGNodeStyleSetBorder(node, YGEdgeLeft, border.left);
    YGNodeStyleSetBorder(node, YGEdgeTop, border.top);
    YGNodeStyleSetBorder(node, YGEdgeRight, border.right);
    YGNodeStyleSetBorder(node, YGEdgeBottom, border.bottom);
}

struct BuiltYogaLayout {
    YogaTree root;
    std::vector<YGNodeRef> childNodes;
    std::vector<UIElement*> elements;
    std::vector<std::unique_ptr<LeafMeasureContext>> leafContexts;
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
    YGNodeStyleSetFlexWrap(root, ToYogaWrap(style.wrap));
    YGNodeStyleSetAlignItems(root, ToYogaAlign(style.alignItems));
    YGNodeStyleSetJustifyContent(root, ToYogaJustify(style.justifyContent));
    YGNodeStyleSetGap(root, style.direction == StackDirection::Row ? YGGutterColumn : YGGutterRow, style.spacing);
    ApplyPadding(root, style.padding);
    ApplyBorder(root, style.border);

    const float clampedWidth = std::max(0.0f, availableWidth);
    const float clampedHeight = std::max(0.0f, availableHeight);
    const float contentWidth = std::max(
        0.0f,
        clampedWidth - style.padding.left - style.padding.right - style.border.left - style.border.right);
    const float contentHeight = std::max(
        0.0f,
        clampedHeight - style.padding.top - style.padding.bottom - style.border.top - style.border.bottom);

    if (exactWidth) {
        YGNodeStyleSetWidth(root, clampedWidth);
    }
    if (exactHeight) {
        YGNodeStyleSetHeight(root, clampedHeight);
    }

    layout.childNodes.reserve(children.size());
    layout.elements.reserve(children.size());
    layout.leafContexts.reserve(children.size());

    for (const auto& child : children) {
        if (!child.element) {
            continue;
        }

        YGNodeRef childNode = YGNodeNew();
        if (!childNode) {
            continue;
        }

        ApplyMargin(childNode, child.margin);
        if (child.element->AlignSelf() != UIElement::SelfAlign::Auto) {
            YGNodeStyleSetAlignSelf(childNode, ToYogaSelfAlign(child.element->AlignSelf()));
        }
        if (child.element->HasMinWidth()) {
            YGNodeStyleSetMinWidth(childNode, child.element->MinWidth());
        }
        if (child.element->HasMaxWidth()) {
            YGNodeStyleSetMaxWidth(childNode, child.element->MaxWidth());
        }
        if (child.element->HasMinHeight()) {
            YGNodeStyleSetMinHeight(childNode, child.element->MinHeight());
        }
        if (child.element->HasMaxHeight()) {
            YGNodeStyleSetMaxHeight(childNode, child.element->MaxHeight());
        }

        const float availableChildWidth =
            std::max(0.0f, contentWidth - child.margin.left - child.margin.right);
        const float availableChildHeight =
            std::max(0.0f, contentHeight - child.margin.top - child.margin.bottom);
        const bool stretchCrossAxis = style.alignItems == StackAlignItems::Stretch &&
            ((style.direction == StackDirection::Column && !child.element->HasFixedWidth()) ||
             (style.direction == StackDirection::Row && !child.element->HasFixedHeight()));
        const bool canStretchCrossAxis = stretchCrossAxis &&
            ((style.direction == StackDirection::Column && exactWidth) ||
             (style.direction == StackDirection::Row && exactHeight));
        const bool useMeasureFunc = child.element->IsLayoutLeaf();
        const float flexGrow = child.element->FlexGrow();
        const float flexShrink = child.element->FlexShrink();
        const bool hasFlexBasis = child.element->HasFlexBasis();
        const bool hasFlex = (flexGrow > 0.0f || flexShrink > 0.0f || hasFlexBasis);
        float measuredWidth = 0.0f;
        float measuredHeight = 0.0f;

        if (useMeasureFunc) {
            auto context = std::make_unique<LeafMeasureContext>();
            context->element = child.element;
            LeafMeasureContext* contextPtr = context.get();
            layout.leafContexts.push_back(std::move(context));

            YGNodeSetContext(childNode, contextPtr);
            YGNodeSetMeasureFunc(childNode, MeasureLeafNode);

            if (child.element->HasFixedWidth()) {
                YGNodeStyleSetWidth(childNode, child.element->GetPreferredWidth(availableChildWidth));
            }
            if (child.element->HasFixedHeight()) {
                YGNodeStyleSetHeight(childNode, child.element->GetPreferredHeight(availableChildWidth));
            }

            if (!hasFlexBasis) {
                const Size preview = MeasureLeafWithContext(
                    *contextPtr,
                    availableChildWidth,
                    YGMeasureModeAtMost,
                    availableChildHeight,
                    YGMeasureModeAtMost);
                measuredWidth = preview.width;
                measuredHeight = preview.height;
            }
        } else {
            measuredWidth = style.direction == StackDirection::Column
                ? (canStretchCrossAxis ? availableChildWidth : child.element->GetPreferredWidth(availableChildWidth))
                : child.element->GetPreferredWidth(availableChildWidth);

            child.element->Measure(measuredWidth, availableChildHeight);
            measuredHeight = child.element->DesiredSize().height;
        }

        // Set flex grow / shrink / basis before size-setting so that Yoga
        // knows which children are flexible when it resolves the layout.
        if (flexGrow > 0.0f) {
            YGNodeStyleSetFlexGrow(childNode, flexGrow);
        }
        if (flexShrink > 0.0f) {
            YGNodeStyleSetFlexShrink(childNode, flexShrink);
        } else if (style.direction == StackDirection::Row && !hasFlex) {
            // Row children with no explicit flex attrs default to shrink=1
            // so total width fits the container. Aligns with CSS flexbox.
            YGNodeStyleSetFlexShrink(childNode, 1.0f);
        }
        if (hasFlex) {
            const float intrinsicMainAxis = style.direction == StackDirection::Column
                ? measuredHeight : measuredWidth;
            const float flexBasisVal = hasFlexBasis
                ? child.element->FlexBasis() : intrinsicMainAxis;
            YGNodeStyleSetFlexBasis(childNode, std::max(0.0f, flexBasisVal));
        }

        // Set explicit Yoga node sizes.
        //
        // When flex is active, do NOT set the main-axis size explicitly —
        // Yoga computes it from flexBasis + flexGrow/flexShrink.
        // Cross-axis size is always set when the item is not stretched.
        if (style.direction == StackDirection::Column) {
            // Column: main axis = height, cross axis = width
            if (!useMeasureFunc) {
                if (!canStretchCrossAxis) {
                    YGNodeStyleSetWidth(childNode, measuredWidth);
                }
                if (!hasFlex) {
                    YGNodeStyleSetHeight(childNode, measuredHeight);
                }
            }
        } else {
            // Row: main axis = width, cross axis = height
            if (!useMeasureFunc || child.element->HasFixedWidth()) {
                if (!hasFlex) {
                    YGNodeStyleSetWidth(childNode, measuredWidth);
                }
            }
            if (!useMeasureFunc) {
                if (canStretchCrossAxis) {
                    YGNodeStyleSetHeight(childNode, availableChildHeight);
                } else {
                    YGNodeStyleSetHeight(childNode, measuredHeight);
                }
            }
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