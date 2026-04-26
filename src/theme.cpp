#include "theme.h"

#include "layout_parser.h"

namespace {

void LoadColorTable(const JsonValue& obj, std::unordered_map<std::string, Color>& out) {
    if (!obj.IsObject()) return;
    for (const auto& [key, value] : obj.objectValue) {
        if (value.IsString()) {
            out[key] = LayoutParser::ColorFromString(value.stringValue);
        }
    }
}

void LoadNumberTable(const JsonValue& obj, std::unordered_map<std::string, float>& out) {
    if (!obj.IsObject()) return;
    for (const auto& [key, value] : obj.objectValue) {
        if (value.IsNumber()) {
            out[key] = static_cast<float>(value.numberValue);
        }
    }
}

} // namespace

std::optional<Color> Theme::ResolveColor(const std::string& key) const {
    auto it = colors.find(key);
    if (it == colors.end()) return std::nullopt;
    return it->second;
}

std::optional<float> Theme::ResolveNumber(NumberCategory category, const std::string& key) const {
    const std::unordered_map<std::string, float>* table = nullptr;
    switch (category) {
        case NumberCategory::Spacing:     table = &spacings;     break;
        case NumberCategory::Radius:      table = &radii;        break;
        case NumberCategory::FontSize:    table = &fontSizes;    break;
        case NumberCategory::BorderWidth: table = &borderWidths; break;
    }
    if (!table) return std::nullopt;
    auto it = table->find(key);
    if (it == table->end()) return std::nullopt;
    return it->second;
}

std::unique_ptr<Theme> Theme::LoadFromJson(const std::string& jsonText) {
    JsonValue root = LayoutParser::ParseJson(jsonText);
    if (!root.IsObject()) return nullptr;

    auto theme = std::make_unique<Theme>();
    LoadColorTable(root["colors"],         theme->colors);
    LoadNumberTable(root["spacings"],      theme->spacings);
    LoadNumberTable(root["radii"],         theme->radii);
    LoadNumberTable(root["fontSizes"],     theme->fontSizes);
    LoadNumberTable(root["borderWidths"],  theme->borderWidths);
    return theme;
}
