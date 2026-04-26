#pragma once

#include "graphics_types.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

class Theme {
public:
    enum class NumberCategory {
        Spacing,
        Radius,
        FontSize,
        BorderWidth
    };

    std::unordered_map<std::string, Color> colors;
    std::unordered_map<std::string, float> spacings;
    std::unordered_map<std::string, float> radii;
    std::unordered_map<std::string, float> fontSizes;
    std::unordered_map<std::string, float> borderWidths;

    std::optional<Color> ResolveColor(const std::string& key) const;
    std::optional<float> ResolveNumber(NumberCategory category, const std::string& key) const;

    static std::unique_ptr<Theme> LoadFromJson(const std::string& jsonText);
};
