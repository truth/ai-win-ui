#include "style.h"

namespace {

template <typename T>
void MergeOptional(std::optional<T>& dst, const std::optional<T>& src) {
    if (src.has_value()) {
        dst = src;
    }
}

} // namespace

StyleSpec ComponentStyle::Resolve(StyleState state) const {
    StyleSpec result = base;
    if (state == StyleState::Normal) {
        return result;
    }
    const std::size_t index = static_cast<std::size_t>(state);
    if (index >= kStyleStateCount) {
        return result;
    }
    const StyleSpec& override_spec = overrides[index];
    MergeOptional(result.decoration, override_spec.decoration);
    MergeOptional(result.padding,    override_spec.padding);
    MergeOptional(result.margin,     override_spec.margin);
    MergeOptional(result.border,     override_spec.border);
    MergeOptional(result.foreground, override_spec.foreground);
    MergeOptional(result.fontSize,   override_spec.fontSize);
    MergeOptional(result.opacity,    override_spec.opacity);
    return result;
}

void ComponentStyle::SetBaseBackground(Color c) {
    if (!base.decoration.has_value()) {
        base.decoration = BoxDecoration{};
    }
    base.decoration->background = c;
}

void ComponentStyle::SetBaseBorder(const Thickness& width, Color color) {
    if (!base.decoration.has_value()) {
        base.decoration = BoxDecoration{};
    }
    base.decoration->border.width = width;
    base.decoration->border.color = color;
}

void ComponentStyle::SetBaseRadius(const CornerRadius& r) {
    if (!base.decoration.has_value()) {
        base.decoration = BoxDecoration{};
    }
    base.decoration->radius = r;
}

void ComponentStyle::SetBaseForeground(Color c) {
    base.foreground = c;
}

void ComponentStyle::SetBaseFontSize(float size) {
    base.fontSize = size;
}
