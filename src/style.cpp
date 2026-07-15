#include "style.h"
#include "theme.h"

#include <utility>

namespace {

template <typename T>
void MergeOptional(std::optional<T>& dst, const std::optional<T>& src) {
    if (src.has_value()) {
        dst = src;
    }
}

std::unique_ptr<ComponentStyle> CloneNested(const std::unique_ptr<ComponentStyle>& src) {
    if (!src) {
        return nullptr;
    }
    return std::make_unique<ComponentStyle>(*src);
}

} // namespace

ComponentStyle::ComponentStyle() = default;

ComponentStyle::ComponentStyle(const ComponentStyle& other)
    : base(other.base)
    , overrides(other.overrides)
    , itemStyle(CloneNested(other.itemStyle))
    , tabStyle(CloneNested(other.tabStyle))
    , dropdownStyle(CloneNested(other.dropdownStyle)) {
}

ComponentStyle& ComponentStyle::operator=(const ComponentStyle& other) {
    if (this == &other) {
        return *this;
    }
    base = other.base;
    overrides = other.overrides;
    itemStyle = CloneNested(other.itemStyle);
    tabStyle = CloneNested(other.tabStyle);
    dropdownStyle = CloneNested(other.dropdownStyle);
    return *this;
}

ComponentStyle::ComponentStyle(ComponentStyle&&) noexcept = default;
ComponentStyle& ComponentStyle::operator=(ComponentStyle&&) noexcept = default;
ComponentStyle::~ComponentStyle() = default;

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
    MergeOptional(result.track,      override_spec.track);
    MergeOptional(result.thumb,      override_spec.thumb);
    MergeOptional(result.fill,       override_spec.fill);
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

Color ComponentStyle::ThemeColor(const Theme* theme, const char* key, Color fallback) {
    if (!theme || !key) {
        return fallback;
    }
    if (auto c = theme->ResolveColor(key)) {
        return *c;
    }
    return fallback;
}

float ComponentStyle::ThemeNumber(const Theme* theme, Theme::NumberCategory category, const char* key, float fallback) {
    if (!theme || !key) {
        return fallback;
    }
    if (auto n = theme->ResolveNumber(category, key)) {
        return *n;
    }
    return fallback;
}
