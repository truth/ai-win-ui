#pragma once

#include "box_decoration.h"
#include "graphics_types.h"

#include <array>
#include <cstddef>
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

    StyleSpec Resolve(StyleState state) const;

    void SetBaseBackground(Color c);
    void SetBaseBorder(const Thickness& width, Color color);
    void SetBaseRadius(const CornerRadius& r);
    void SetBaseForeground(Color c);
    void SetBaseFontSize(float size);
};
