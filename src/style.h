#pragma once

#include "box_decoration.h"
#include "graphics_types.h"
#include "theme.h"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>

enum class StyleState : std::size_t {
    Normal = 0,
    Hover,
    Pressed,
    Focused,
    Disabled,
    Selected,
    ReadOnly,
    Count
};

constexpr std::size_t kStyleStateCount = static_cast<std::size_t>(StyleState::Count);

struct StyleSpec {
    std::optional<BoxDecoration> decoration;
    // Sub-decorations for composite controls (Slider track/thumb, Progress fill, etc.).
    std::optional<BoxDecoration> track;
    std::optional<BoxDecoration> thumb;
    std::optional<BoxDecoration> fill;
    std::optional<Thickness>     padding;
    std::optional<Thickness>     margin;
    std::optional<Thickness>     border;
    std::optional<Color>         foreground;
    std::optional<float>         fontSize;
    std::optional<float>         opacity;
};

class ComponentStyle {
public:
    StyleSpec base;
    std::array<StyleSpec, kStyleStateCount> overrides{};

    // Nested styles for list rows, tab chips, and combo dropdown panels.
    std::unique_ptr<ComponentStyle> itemStyle;
    std::unique_ptr<ComponentStyle> tabStyle;
    std::unique_ptr<ComponentStyle> dropdownStyle;

    ComponentStyle();
    ComponentStyle(const ComponentStyle& other);
    ComponentStyle& operator=(const ComponentStyle& other);
    ComponentStyle(ComponentStyle&&) noexcept;
    ComponentStyle& operator=(ComponentStyle&&) noexcept;
    ~ComponentStyle();

    StyleSpec Resolve(StyleState state) const;

    void SetBaseBackground(Color c);
    void SetBaseBorder(const Thickness& width, Color color);
    void SetBaseRadius(const CornerRadius& r);
    void SetBaseForeground(Color c);
    void SetBaseFontSize(float size);

    // Resolve optional theme color with hard-coded fallback (DefaultStyle lazy theme).
    static Color ThemeColor(const Theme* theme, const char* key, Color fallback);
    static float ThemeNumber(const Theme* theme, Theme::NumberCategory category, const char* key, float fallback);
};
