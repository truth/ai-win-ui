#include "layout_parser.h"
#include "resource_provider.h"
#include "theme.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <windows.h>

namespace {

const Theme* g_activeTheme = nullptr;

bool StartsWith(const std::string& s, const char* prefix) {
    const size_t n = std::char_traits<char>::length(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

void WarnUnknownToken(const std::string& token) {
    OutputDebugStringA(("[Theme] Unknown token: " + token + "\n").c_str());
}

std::optional<Color> TryResolveColorToken(const std::string& s) {
    if (!StartsWith(s, "$color.")) return std::nullopt;
    const std::string key = s.substr(7);
    if (g_activeTheme) {
        if (auto v = g_activeTheme->ResolveColor(key)) return v;
    }
    WarnUnknownToken(s);
    return ColorFromHex(0xFF00FF);  // magenta sentinel
}

std::optional<float> TryResolveNumberToken(const std::string& s, Theme::NumberCategory expected) {
    auto matchPrefix = [&](const char* prefix, Theme::NumberCategory cat) -> std::optional<float> {
        if (!StartsWith(s, prefix)) return std::nullopt;
        const std::string key = s.substr(std::char_traits<char>::length(prefix));
        if (g_activeTheme) {
            if (auto v = g_activeTheme->ResolveNumber(cat, key)) return v;
        }
        WarnUnknownToken(s);
        return 0.0f;
    };
    if (auto v = matchPrefix("$spacing.",     Theme::NumberCategory::Spacing))     return v;
    if (auto v = matchPrefix("$radius.",      Theme::NumberCategory::Radius))      return v;
    if (auto v = matchPrefix("$fontSize.",    Theme::NumberCategory::FontSize))    return v;
    if (auto v = matchPrefix("$borderWidth.", Theme::NumberCategory::BorderWidth)) return v;
    (void)expected;
    return std::nullopt;
}

// Assign a float field from a JSON value that may be either a literal number
// or a "$category.key" string token. Returns true if target was modified.
bool TryAssignNumber(const JsonValue& value,
                     Theme::NumberCategory category,
                     float& target) {
    if (value.IsNumber()) {
        target = static_cast<float>(value.numberValue);
        return true;
    }
    if (value.IsString()) {
        if (auto resolved = TryResolveNumberToken(value.stringValue, category)) {
            target = *resolved;
            return true;
        }
    }
    return false;
}

void SkipWhitespace(const std::string& text, size_t& pos) {
    while (pos < text.size() && isspace(static_cast<unsigned char>(text[pos]))) {
        pos++;
    }
}

bool Matches(const std::string& text, size_t pos, const std::string& token) {
    return text.compare(pos, token.size(), token) == 0;
}

std::string ParseIdentifier(const std::string& text, size_t& pos) {
    size_t start = pos;
    while (pos < text.size() && (isalnum(static_cast<unsigned char>(text[pos])) || text[pos] == '_' || text[pos] == '-')) {
        pos++;
    }
    return text.substr(start, pos - start);
}

std::string ParseStringLiteral(const std::string& text, size_t& pos) {
    std::string result;
    if (pos >= text.size() || text[pos] != '"') {
        return result;
    }
    pos++;
    while (pos < text.size()) {
        char ch = text[pos++];
        if (ch == '"') {
            return result;
        }
        if (ch == '\\' && pos < text.size()) {
            char next = text[pos++];
            switch (next) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            default: result.push_back(next); break;
            }
            continue;
        }
        result.push_back(ch);
    }
    return result;
}

JsonValue ParseJsonValue(const std::string& text, size_t& pos);

JsonValue ParseJsonObject(const std::string& text, size_t& pos) {
    JsonValue value;
    value.type = JsonValue::Type::Object;
    pos++;
    SkipWhitespace(text, pos);
    if (pos < text.size() && text[pos] == '}') {
        pos++;
        return value;
    }

    while (pos < text.size()) {
        SkipWhitespace(text, pos);
        std::string key = ParseStringLiteral(text, pos);
        SkipWhitespace(text, pos);
        if (pos >= text.size() || text[pos] != ':') {
            throw std::runtime_error("Invalid JSON object: missing ':'");
        }
        pos++;
        SkipWhitespace(text, pos);
        JsonValue item = ParseJsonValue(text, pos);
        value.objectValue.emplace(std::move(key), std::move(item));
        SkipWhitespace(text, pos);
        if (pos >= text.size()) {
            break;
        }
        if (text[pos] == ',') {
            pos++;
            continue;
        }
        if (text[pos] == '}') {
            pos++;
            break;
        }
        throw std::runtime_error("Invalid JSON object: expected ',' or '}'");
    }
    return value;
}

JsonValue ParseJsonArray(const std::string& text, size_t& pos) {
    JsonValue value;
    value.type = JsonValue::Type::Array;
    pos++;
    SkipWhitespace(text, pos);
    if (pos < text.size() && text[pos] == ']') {
        pos++;
        return value;
    }

    while (pos < text.size()) {
        SkipWhitespace(text, pos);
        JsonValue item = ParseJsonValue(text, pos);
        value.arrayValue.emplace_back(std::move(item));
        SkipWhitespace(text, pos);
        if (pos >= text.size()) {
            break;
        }
        if (text[pos] == ',') {
            pos++;
            continue;
        }
        if (text[pos] == ']') {
            pos++;
            break;
        }
        throw std::runtime_error("Invalid JSON array: expected ',' or ']'");
    }
    return value;
}

JsonValue ParseJsonNumber(const std::string& text, size_t& pos) {
    size_t start = pos;
    if (pos < text.size() && (text[pos] == '-' || text[pos] == '+')) {
        pos++;
    }
    while (pos < text.size() && isdigit(static_cast<unsigned char>(text[pos]))) {
        pos++;
    }
    if (pos < text.size() && text[pos] == '.') {
        pos++;
        while (pos < text.size() && isdigit(static_cast<unsigned char>(text[pos]))) {
            pos++;
        }
    }
    if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E')) {
        pos++;
        if (pos < text.size() && (text[pos] == '+' || text[pos] == '-')) {
            pos++;
        }
        while (pos < text.size() && isdigit(static_cast<unsigned char>(text[pos]))) {
            pos++;
        }
    }

    const std::string raw = text.substr(start, pos - start);
    JsonValue value;
    value.type = JsonValue::Type::Number;
    value.numberValue = std::stod(raw);
    return value;
}

JsonValue ParseJsonValue(const std::string& text, size_t& pos) {
    SkipWhitespace(text, pos);
    if (pos >= text.size()) {
        return {};
    }

    char ch = text[pos];
    if (ch == '{') {
        return ParseJsonObject(text, pos);
    }
    if (ch == '[') {
        return ParseJsonArray(text, pos);
    }
    if (ch == '"') {
        JsonValue value;
        value.type = JsonValue::Type::String;
        value.stringValue = ParseStringLiteral(text, pos);
        return value;
    }
    if (ch == 't' && Matches(text, pos, "true")) {
        pos += 4;
        JsonValue value;
        value.type = JsonValue::Type::Bool;
        value.boolValue = true;
        return value;
    }
    if (ch == 'f' && Matches(text, pos, "false")) {
        pos += 5;
        JsonValue value;
        value.type = JsonValue::Type::Bool;
        value.boolValue = false;
        return value;
    }
    if (ch == 'n' && Matches(text, pos, "null")) {
        pos += 4;
        return JsonValue{};
    }
    return ParseJsonNumber(text, pos);
}

std::string ReadAttributeValue(const std::string& text, size_t& pos) {
    SkipWhitespace(text, pos);
    if (pos >= text.size() || text[pos] != '"') {
        return {};
    }
    return ParseStringLiteral(text, pos);
}

XmlNode ParseXmlNode(const std::string& text, size_t& pos);

XmlNode ParseXml(const std::string& text, size_t& pos) {
    XmlNode node;
    while (pos < text.size()) {
        SkipWhitespace(text, pos);
        if (pos >= text.size() || text[pos] != '<') {
            pos++;
            continue;
        }
        if (Matches(text, pos, "<!--")) {
            pos += 4;
            size_t end = text.find("-->", pos);
            pos = (end == std::string::npos) ? text.size() : end + 3;
            continue;
        }
        pos++;
        if (pos < text.size() && text[pos] == '/') {
            return node;
        }

        size_t startName = pos;
        while (pos < text.size() && !isspace(static_cast<unsigned char>(text[pos])) && text[pos] != '/' && text[pos] != '>') {
            pos++;
        }
        node.name = text.substr(startName, pos - startName);

        while (pos < text.size() && text[pos] != '>' && text[pos] != '/') {
            SkipWhitespace(text, pos);
            if (pos >= text.size() || text[pos] == '>' || text[pos] == '/') {
                break;
            }
            std::string key = ParseIdentifier(text, pos);
            SkipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == '=') {
                pos++;
                SkipWhitespace(text, pos);
                std::string value = ReadAttributeValue(text, pos);
                node.attributes.emplace(std::move(key), std::move(value));
            }
        }

        bool selfClosing = false;
        if (pos < text.size() && text[pos] == '/') {
            selfClosing = true;
            pos++;
        }
        if (pos < text.size() && text[pos] == '>') {
            pos++;
        }

        if (!selfClosing) {
            while (true) {
                SkipWhitespace(text, pos);
                if (pos < text.size() && text[pos] == '<' && pos + 1 < text.size() && text[pos + 1] == '/') {
                    pos += 2;
                    while (pos < text.size() && text[pos] != '>') {
                        pos++;
                    }
                    if (pos < text.size() && text[pos] == '>') {
                        pos++;
                    }
                    break;
                }
                if (pos >= text.size()) {
                    break;
                }
                if (pos < text.size() && text[pos] == '<') {
                    XmlNode child = ParseXmlNode(text, pos);
                    if (!child.name.empty()) {
                        node.children.emplace_back(std::move(child));
                    }
                } else {
                    pos++;
                }
            }
        }
        break;
    }
    return node;
}

XmlNode ParseXmlNode(const std::string& text, size_t& pos) {
    return ParseXml(text, pos);
}

std::vector<std::string> SplitString(const std::string& value, char separator) {
    std::vector<std::string> pieces;
    std::string current;
    for (char ch : value) {
        if (ch == separator) {
            if (!current.empty()) {
                pieces.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        pieces.push_back(current);
    }
    return pieces;
}

std::string TrimString(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && isspace(static_cast<unsigned char>(value[start]))) {
        start++;
    }

    size_t end = value.size();
    while (end > start && isspace(static_cast<unsigned char>(value[end - 1]))) {
        end--;
    }

    return value.substr(start, end - start);
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return value;
}

Thickness ParseThickness(const std::vector<std::string>& values) {
    Thickness thickness;
    if (values.size() == 4) {
        thickness.left = std::stof(TrimString(values[0]));
        thickness.top = std::stof(TrimString(values[1]));
        thickness.right = std::stof(TrimString(values[2]));
        thickness.bottom = std::stof(TrimString(values[3]));
    }
    return thickness;
}

Thickness ParseThickness(const JsonValue& value) {
    Thickness thickness;
    if (value.IsString()) {
        if (auto v = TryResolveNumberToken(value.stringValue, Theme::NumberCategory::Spacing)) {
            return Thickness{*v, *v, *v, *v};
        }
    }
    if (value.IsArray() && value.arrayValue.size() == 4) {
        // Each element may itself be a number or a "$spacing.x" string token.
        auto resolveOne = [](const JsonValue& el) -> float {
            if (el.IsString()) {
                if (auto v = TryResolveNumberToken(el.stringValue, Theme::NumberCategory::Spacing)) return *v;
                return 0.0f;
            }
            return static_cast<float>(el.numberValue);
        };
        thickness.left   = resolveOne(value[0]);
        thickness.top    = resolveOne(value[1]);
        thickness.right  = resolveOne(value[2]);
        thickness.bottom = resolveOne(value[3]);
    } else if (value.IsNumber()) {
        const float v = static_cast<float>(value.numberValue);
        thickness = Thickness{v, v, v, v};
    }
    return thickness;
}

BorderSpec ParseBorderSpec(const JsonValue& value) {
    BorderSpec border;
    if (!value.IsObject()) {
        return border;
    }
    if (value["width"].IsNumber()) {
        const float w = static_cast<float>(value["width"].numberValue);
        border.width = Thickness{w, w, w, w};
    } else if (value["width"].IsArray() && value["width"].arrayValue.size() == 4) {
        border.width = ParseThickness(value["width"]);
    }
    if (value["color"].IsString()) {
        border.color = LayoutParser::ColorFromString(value["color"].stringValue);
    }
    return border;
}

StyleSpec ParseStyleSpec(const JsonValue& value) {
    StyleSpec spec;
    if (!value.IsObject()) {
        return spec;
    }
    BoxDecoration deco;
    bool decoTouched = false;
    if (value["background"].IsString()) {
        deco.background = LayoutParser::ColorFromString(value["background"].stringValue);
        decoTouched = true;
    }
    if (value["border"].IsObject()) {
        deco.border = ParseBorderSpec(value["border"]);
        decoTouched = true;
    }
    if (value["cornerRadius"].IsNumber()) {
        deco.radius = CornerRadius::Uniform(static_cast<float>(value["cornerRadius"].numberValue));
        decoTouched = true;
    }
    if (value["opacity"].IsNumber()) {
        deco.opacity = static_cast<float>(value["opacity"].numberValue);
        decoTouched = true;
        spec.opacity = static_cast<float>(value["opacity"].numberValue);
    }
    if (decoTouched) {
        spec.decoration = deco;
    }
    if (value["foreground"].IsString()) {
        spec.foreground = LayoutParser::ColorFromString(value["foreground"].stringValue);
    }
    if (value["fontSize"].IsNumber()) {
        spec.fontSize = static_cast<float>(value["fontSize"].numberValue);
    }
    if (value["padding"].IsArray()) {
        spec.padding = ParseThickness(value["padding"]);
    }
    if (value["margin"].IsArray()) {
        spec.margin = ParseThickness(value["margin"]);
    }
    if (value["borderWidth"].IsArray() || value["borderWidth"].IsNumber()) {
        spec.border = ParseThickness(value["borderWidth"]);
    }
    return spec;
}

ComponentStyle ParseComponentStyle(const JsonValue& value) {
    ComponentStyle style;
    if (!value.IsObject()) {
        return style;
    }
    if (value["base"].IsObject()) {
        style.base = ParseStyleSpec(value["base"]);
    }
    static const struct { const char* key; StyleState state; } kStateKeys[] = {
        {"hover",    StyleState::Hover},
        {"pressed",  StyleState::Pressed},
        {"focused",  StyleState::Focused},
        {"disabled", StyleState::Disabled},
        {"selected", StyleState::Selected},
        {"readonly", StyleState::ReadOnly},
    };
    for (const auto& entry : kStateKeys) {
        if (value[entry.key].IsObject()) {
            style.overrides[static_cast<std::size_t>(entry.state)] = ParseStyleSpec(value[entry.key]);
        }
    }
    return style;
}

std::vector<float> ParseNumberList(const std::string& value, char separator = ',') {
    std::vector<float> numbers;
    const auto pieces = SplitString(value, separator);
    numbers.reserve(pieces.size());
    for (const auto& piece : pieces) {
        const std::string trimmed = TrimString(piece);
        if (!trimmed.empty()) {
            numbers.push_back(std::stof(trimmed));
        }
    }
    return numbers;
}

std::vector<float> ParseNumberArray(const JsonValue& value) {
    std::vector<float> numbers;
    if (!value.IsArray()) {
        return numbers;
    }
    numbers.reserve(value.arrayValue.size());
    for (const auto& item : value.arrayValue) {
        if (item.IsNumber()) {
            numbers.push_back(static_cast<float>(item.numberValue));
        }
    }
    return numbers;
}

std::wstring JsonCellToWide(const JsonValue& value) {
    if (value.IsString()) {
        return LayoutParser::Utf8ToUtf16(value.stringValue);
    }
    if (value.IsNumber()) {
        std::ostringstream oss;
        oss << value.numberValue;
        return LayoutParser::Utf8ToUtf16(oss.str());
    }
    if (value.IsBool()) {
        return value.boolValue ? L"true" : L"false";
    }
    return L"";
}

std::vector<std::wstring> ParseWideStringList(const std::string& value, char separator = '|') {
    std::vector<std::wstring> result;
    for (const auto& piece : SplitString(value, separator)) {
        result.push_back(LayoutParser::Utf8ToUtf16(TrimString(piece)));
    }
    return result;
}

std::vector<std::vector<std::wstring>> ParseWideStringRows(const std::string& value) {
    std::vector<std::vector<std::wstring>> rows;
    for (const auto& rowText : SplitString(value, ';')) {
        const std::string trimmedRow = TrimString(rowText);
        if (trimmedRow.empty()) {
            continue;
        }
        rows.push_back(ParseWideStringList(trimmedRow, '|'));
    }
    return rows;
}

std::vector<std::wstring> ParseWideStringPaths(const std::string& value) {
    std::vector<std::wstring> paths;
    for (const auto& piece : SplitString(value, ';')) {
        const std::string trimmed = TrimString(piece);
        if (!trimmed.empty()) {
            paths.push_back(LayoutParser::Utf8ToUtf16(trimmed));
        }
    }
    return paths;
}

std::vector<std::wstring> ParseWideStringArray(const JsonValue& value) {
    std::vector<std::wstring> result;
    if (!value.IsArray()) {
        return result;
    }
    result.reserve(value.arrayValue.size());
    for (const auto& item : value.arrayValue) {
        result.push_back(JsonCellToWide(item));
    }
    return result;
}

Panel::AlignItems ParseAlignItems(const std::string& value) {
    const std::string normalized = ToLowerAscii(TrimString(value));
    if (normalized == "start" || normalized == "left" || normalized == "flex-start" || normalized == "flexstart") {
        return Panel::AlignItems::Start;
    }
    if (normalized == "center") {
        return Panel::AlignItems::Center;
    }
    if (normalized == "end" || normalized == "right" || normalized == "flex-end" || normalized == "flexend") {
        return Panel::AlignItems::End;
    }
    return Panel::AlignItems::Stretch;
}

UIElement::SelfAlign ParseSelfAlign(const std::string& value) {
    const std::string normalized = ToLowerAscii(TrimString(value));
    if (normalized == "stretch") {
        return UIElement::SelfAlign::Stretch;
    }
    if (normalized == "start" || normalized == "flex-start" || normalized == "flexstart") {
        return UIElement::SelfAlign::Start;
    }
    if (normalized == "center") {
        return UIElement::SelfAlign::Center;
    }
    if (normalized == "end" || normalized == "flex-end" || normalized == "flexend") {
        return UIElement::SelfAlign::End;
    }
    return UIElement::SelfAlign::Auto;
}

Panel::Wrap ParseWrap(const std::string& value) {
    const std::string normalized = ToLowerAscii(TrimString(value));
    if (normalized == "wrap") {
        return Panel::Wrap::Wrap;
    }
    return Panel::Wrap::NoWrap;
}

Panel::Direction ParseDirection(const std::string& value) {
    const std::string normalized = ToLowerAscii(TrimString(value));
    if (normalized == "row" || normalized == "horizontal") {
        return Panel::Direction::Row;
    }
    return Panel::Direction::Column;
}

Panel::JustifyContent ParseJustifyContent(const std::string& value) {
    const std::string normalized = ToLowerAscii(TrimString(value));
    if (normalized == "center") {
        return Panel::JustifyContent::Center;
    }
    if (normalized == "end" || normalized == "flex-end" || normalized == "flexend") {
        return Panel::JustifyContent::End;
    }
    if (normalized == "space-between" || normalized == "spacebetween") {
        return Panel::JustifyContent::SpaceBetween;
    }
    return Panel::JustifyContent::Start;
}

void ApplyFlexShorthand(const std::string& rawValue, UIElement& element) {
    const std::string normalized = ToLowerAscii(TrimString(rawValue));
    if (normalized.empty()) {
        return;
    }

    if (normalized == "none") {
        element.SetFlexGrow(0.0f);
        element.SetFlexShrink(0.0f);
        return;
    }
    if (normalized == "auto") {
        element.SetFlexGrow(1.0f);
        element.SetFlexShrink(1.0f);
        return;
    }

    const auto tokens = SplitString(normalized, ' ');
    std::vector<std::string> parts;
    parts.reserve(tokens.size());
    for (const auto& token : tokens) {
        const std::string trimmed = TrimString(token);
        if (!trimmed.empty()) {
            parts.push_back(trimmed);
        }
    }
    if (parts.empty()) {
        return;
    }

    element.SetFlexGrow(std::stof(parts[0]));
    if (parts.size() == 1) {
        element.SetFlexShrink(1.0f);
        element.SetFlexBasis(0.0f);
        return;
    }

    element.SetFlexShrink(std::stof(parts[1]));
    if (parts.size() >= 3 && parts[2] != "auto") {
        element.SetFlexBasis(std::stof(parts[2]));
    }
}

void ApplyCommonJsonProps(UIElement& element, const JsonValue& props) {
    if (!props.IsObject()) {
        return;
    }

    if (props["flex"].IsNumber()) {
        element.SetFlexGrow(static_cast<float>(props["flex"].numberValue));
        element.SetFlexShrink(1.0f);
        element.SetFlexBasis(0.0f);
    } else if (props["flex"].IsString()) {
        ApplyFlexShorthand(props["flex"].stringValue, element);
    }

    if (props["width"].IsNumber()) {
        element.SetFixedWidth(static_cast<float>(props["width"].numberValue));
    }
    if (props["height"].IsNumber()) {
        element.SetFixedHeight(static_cast<float>(props["height"].numberValue));
    }
    if (props["minWidth"].IsNumber()) {
        element.SetMinWidth(static_cast<float>(props["minWidth"].numberValue));
    } else if (props["min-width"].IsNumber()) {
        element.SetMinWidth(static_cast<float>(props["min-width"].numberValue));
    }
    if (props["maxWidth"].IsNumber()) {
        element.SetMaxWidth(static_cast<float>(props["maxWidth"].numberValue));
    } else if (props["max-width"].IsNumber()) {
        element.SetMaxWidth(static_cast<float>(props["max-width"].numberValue));
    }
    if (props["minHeight"].IsNumber()) {
        element.SetMinHeight(static_cast<float>(props["minHeight"].numberValue));
    } else if (props["min-height"].IsNumber()) {
        element.SetMinHeight(static_cast<float>(props["min-height"].numberValue));
    }
    if (props["maxHeight"].IsNumber()) {
        element.SetMaxHeight(static_cast<float>(props["maxHeight"].numberValue));
    } else if (props["max-height"].IsNumber()) {
        element.SetMaxHeight(static_cast<float>(props["max-height"].numberValue));
    }
    if (props["margin"].IsArray() && props["margin"].arrayValue.size() == 4) {
        element.SetMargin(ParseThickness(props["margin"]));
    }
    if (props["border"].IsArray() || props["border"].IsNumber()) {
        element.SetBorder(ParseThickness(props["border"]));
    }
    if (props["style"].IsObject()) {
        element.SetStyle(ParseComponentStyle(props["style"]));
    }
    if (props["disabled"].IsBool() && props["disabled"].boolValue) {
        element.SetEnabled(false);
    }
    if (props["flexGrow"].IsNumber()) {
        element.SetFlexGrow(static_cast<float>(props["flexGrow"].numberValue));
    } else if (props["flex-grow"].IsNumber()) {
        element.SetFlexGrow(static_cast<float>(props["flex-grow"].numberValue));
    }
    if (props["flexShrink"].IsNumber()) {
        element.SetFlexShrink(static_cast<float>(props["flexShrink"].numberValue));
    } else if (props["flex-shrink"].IsNumber()) {
        element.SetFlexShrink(static_cast<float>(props["flex-shrink"].numberValue));
    }
    if (props["flexBasis"].IsNumber()) {
        element.SetFlexBasis(static_cast<float>(props["flexBasis"].numberValue));
    } else if (props["flex-basis"].IsNumber()) {
        element.SetFlexBasis(static_cast<float>(props["flex-basis"].numberValue));
    }
    if (props["alignSelf"].IsString()) {
        element.SetAlignSelf(ParseSelfAlign(props["alignSelf"].stringValue));
    } else if (props["align-self"].IsString()) {
        element.SetAlignSelf(ParseSelfAlign(props["align-self"].stringValue));
    }
}

void ApplyCommonXmlAttributes(UIElement& element, const XmlNode& node) {
    if (auto it = node.attributes.find("flex"); it != node.attributes.end()) {
        ApplyFlexShorthand(it->second, element);
    }

    if (auto it = node.attributes.find("width"); it != node.attributes.end()) {
        element.SetFixedWidth(std::stof(TrimString(it->second)));
    }
    if (auto it = node.attributes.find("height"); it != node.attributes.end()) {
        element.SetFixedHeight(std::stof(TrimString(it->second)));
    }
    if (auto it = node.attributes.find("minWidth"); it != node.attributes.end()) {
        element.SetMinWidth(std::stof(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("min-width"); it2 != node.attributes.end()) {
        element.SetMinWidth(std::stof(TrimString(it2->second)));
    }
    if (auto it = node.attributes.find("maxWidth"); it != node.attributes.end()) {
        element.SetMaxWidth(std::stof(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("max-width"); it2 != node.attributes.end()) {
        element.SetMaxWidth(std::stof(TrimString(it2->second)));
    }
    if (auto it = node.attributes.find("minHeight"); it != node.attributes.end()) {
        element.SetMinHeight(std::stof(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("min-height"); it2 != node.attributes.end()) {
        element.SetMinHeight(std::stof(TrimString(it2->second)));
    }
    if (auto it = node.attributes.find("maxHeight"); it != node.attributes.end()) {
        element.SetMaxHeight(std::stof(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("max-height"); it2 != node.attributes.end()) {
        element.SetMaxHeight(std::stof(TrimString(it2->second)));
    }
    if (auto it = node.attributes.find("margin"); it != node.attributes.end()) {
        const auto values = SplitString(it->second, ',');
        if (values.size() == 4) {
            element.SetMargin(ParseThickness(values));
        }
    }
    if (auto it = node.attributes.find("flexGrow"); it != node.attributes.end()) {
        element.SetFlexGrow(std::stof(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("flex-grow"); it2 != node.attributes.end()) {
        element.SetFlexGrow(std::stof(TrimString(it2->second)));
    }
    if (auto it = node.attributes.find("flexShrink"); it != node.attributes.end()) {
        element.SetFlexShrink(std::stof(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("flex-shrink"); it2 != node.attributes.end()) {
        element.SetFlexShrink(std::stof(TrimString(it2->second)));
    }
    if (auto it = node.attributes.find("flexBasis"); it != node.attributes.end()) {
        element.SetFlexBasis(std::stof(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("flex-basis"); it2 != node.attributes.end()) {
        element.SetFlexBasis(std::stof(TrimString(it2->second)));
    }
    if (auto it = node.attributes.find("alignSelf"); it != node.attributes.end()) {
        element.SetAlignSelf(ParseSelfAlign(TrimString(it->second)));
    } else if (auto it2 = node.attributes.find("align-self"); it2 != node.attributes.end()) {
        element.SetAlignSelf(ParseSelfAlign(TrimString(it2->second)));
    }
}

Color ParseHexColor(const std::string& value) {
    std::string hex = value;
    if (!hex.empty() && hex[0] == '#') {
        hex.erase(0, 1);
    }
    // Case-insensitive sentinel for "no fill" / transparent background.
    std::string lowered = hex;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowered == "transparent" || lowered == "none") {
        return ColorFromHex(0x000000, 0.0f);
    }
    if (hex.size() == 8) {
        unsigned int rgba = 0;
        std::stringstream ss;
        ss << std::hex << hex;
        ss >> rgba;
        const uint32_t rgb = (rgba >> 8) & 0xFFFFFFu;
        const float alpha = static_cast<float>(rgba & 0xFFu) / 255.0f;
        return ColorFromHex(rgb, alpha);
    }
    if (hex.size() == 6) {
        unsigned int rgb = 0;
        std::stringstream ss;
        ss << std::hex << hex;
        ss >> rgb;
        return ColorFromHex(rgb);
    }
    return ColorFromHex(0xFFFFFF);
}

} // namespace

const JsonValue& JsonValue::operator[](const std::string& key) const {
    static const JsonValue nullValue;
    auto it = objectValue.find(key);
    return it != objectValue.end() ? it->second : nullValue;
}

const JsonValue& JsonValue::operator[](size_t index) const {
    static const JsonValue nullValue;
    if (index < arrayValue.size()) {
        return arrayValue[index];
    }
    return nullValue;
}

bool JsonValue::HasKey(const std::string& key) const {
    return objectValue.find(key) != objectValue.end();
}

JsonValue LayoutParser::ParseJson(const std::string& text) {
    size_t pos = 0;
    return ParseJsonValue(text, pos);
}

XmlNode LayoutParser::ParseXml(const std::string& text) {
    size_t pos = 0;
    return ::ParseXml(text, pos);
}

std::wstring LayoutParser::Utf8ToUtf16(const std::string& utf8) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.data(), size);
    return result;
}

Color LayoutParser::ColorFromString(const std::string& value) {
    if (auto token = TryResolveColorToken(value)) {
        return *token;
    }
    return ParseHexColor(value);
}

float LayoutParser::ParseNumberValue(const JsonValue& value,
                                     Theme::NumberCategory category,
                                     float fallback) {
    if (value.IsNumber()) {
        return static_cast<float>(value.numberValue);
    }
    if (value.IsString()) {
        if (auto v = TryResolveNumberToken(value.stringValue, category)) {
            return *v;
        }
    }
    return fallback;
}

std::unique_ptr<UIElement> CreateElementFromJson(const JsonValue& node, IResourceProvider& provider, UIEventResolver eventResolver);
std::unique_ptr<UIElement> CreateElementFromXml(const XmlNode& node, IResourceProvider& provider, UIEventResolver eventResolver);

std::unique_ptr<UIElement> LayoutParser::BuildFromFile(UIContext& context,
                                                       const std::string& resourcePath) {
    if (!context.resourceProvider) {
        return nullptr;
    }

    g_activeTheme = context.theme;

    IResourceProvider& provider = *context.resourceProvider;
    std::string path = resourcePath;
    if (!provider.Exists(path)) {
        const bool isJson = path.size() >= 5 && path.substr(path.size() - 5) == ".json";
        const bool isXml = path.size() >= 4 && path.substr(path.size() - 4) == ".xml";

        if (isJson) {
            std::string xmlPath = path.substr(0, path.size() - 5) + ".xml";
            if (provider.Exists(xmlPath)) {
                path = std::move(xmlPath);
            }
        } else if (isXml) {
            std::string jsonPath = path.substr(0, path.size() - 4) + ".json";
            if (provider.Exists(jsonPath)) {
                path = std::move(jsonPath);
            }
        }
    }

    if (!provider.Exists(path)) {
        return nullptr;
    }

    const std::string text = provider.LoadText(path);
    const bool isJson = path.size() >= 5 && path.substr(path.size() - 5) == ".json";
    const bool isXml = path.size() >= 4 && path.substr(path.size() - 4) == ".xml";

    try {
        if (isJson) {
            JsonValue root = ParseJson(text);
            auto element = CreateElementFromJson(root, provider, context.eventResolver);
            if (element) {
                element->SetContext(&context);
            }
            return element;
        }
        if (isXml) {
            XmlNode root = ParseXml(text);
            auto element = CreateElementFromXml(root, provider, context.eventResolver);
            if (element) {
                element->SetContext(&context);
            }
            return element;
        }
    } catch (const std::exception&) {
        return nullptr;
    }

    return nullptr;
}

std::wstring MakeUtf16(const std::string& text) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring result(size > 0 ? size - 1 : 0, L'\0');
    if (size > 0) {
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), size);
    }
    return result;
}

std::unique_ptr<UIElement> CreateElementFromJson(const JsonValue& node, IResourceProvider& provider, UIEventResolver eventResolver) {
    if (!node.IsObject()) {
        return nullptr;
    }

    const std::string type = node["type"].IsString() ? node["type"].stringValue : "";
    std::unique_ptr<UIElement> element;

    if (type == "Panel") {
        auto panel = std::make_unique<Panel>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["background"].IsString()) {
                panel->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, panel->cornerRadius);
            TryAssignNumber(props["spacing"], Theme::NumberCategory::Spacing, panel->spacing);
            if (props["padding"].IsArray() && props["padding"].arrayValue.size() == 4) {
                panel->padding = ParseThickness(props["padding"]);
            }
            if (props["direction"].IsString()) {
                panel->direction = ParseDirection(props["direction"].stringValue);
            }
            if (props["wrap"].IsString()) {
                panel->wrap = ParseWrap(props["wrap"].stringValue);
            }
            if (props["alignItems"].IsString()) {
                panel->alignItems = ParseAlignItems(props["alignItems"].stringValue);
            } else if (props["align-items"].IsString()) {
                panel->alignItems = ParseAlignItems(props["align-items"].stringValue);
            }
            if (props["justifyContent"].IsString()) {
                panel->justifyContent = ParseJustifyContent(props["justifyContent"].stringValue);
            } else if (props["justify-content"].IsString()) {
                panel->justifyContent = ParseJustifyContent(props["justify-content"].stringValue);
            }
            ApplyCommonJsonProps(*panel, props);
        }
        element = std::move(panel);
    } else if (type == "Label") {
        std::wstring text = L"";
        auto label = std::make_unique<Label>(std::move(text));
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["text"].IsString()) {
                label->SetText(LayoutParser::Utf8ToUtf16(props["text"].stringValue));
            }
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, label->m_fontSize);
            if (props["color"].IsString()) {
                label->m_color = LayoutParser::ColorFromString(props["color"].stringValue);
            }
            ApplyCommonJsonProps(*label, props);
        }
        element = std::move(label);
    } else if (type == "StatCard") {
        auto statCard = std::make_unique<StatCard>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["title"].IsString()) {
                statCard->SetTitle(LayoutParser::Utf8ToUtf16(props["title"].stringValue));
            }
            if (props["value"].IsString()) {
                statCard->SetValue(LayoutParser::Utf8ToUtf16(props["value"].stringValue));
            }
            if (props["deltaText"].IsString()) {
                statCard->SetDeltaText(LayoutParser::Utf8ToUtf16(props["deltaText"].stringValue));
            } else if (props["delta"].IsString()) {
                statCard->SetDeltaText(LayoutParser::Utf8ToUtf16(props["delta"].stringValue));
            }
            if (props["background"].IsString()) {
                statCard->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                statCard->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["accentColor"].IsString()) {
                statCard->accentColor = LayoutParser::ColorFromString(props["accentColor"].stringValue);
            }
            if (props["titleColor"].IsString()) {
                statCard->titleColor = LayoutParser::ColorFromString(props["titleColor"].stringValue);
            }
            if (props["valueColor"].IsString()) {
                statCard->valueColor = LayoutParser::ColorFromString(props["valueColor"].stringValue);
            }
            if (props["deltaColor"].IsString()) {
                statCard->deltaColor = LayoutParser::ColorFromString(props["deltaColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, statCard->cornerRadius);
            ApplyCommonJsonProps(*statCard, props);
        }
        element = std::move(statCard);
    } else if (type == "SparklineChart") {
        auto chart = std::make_unique<SparklineChart>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["points"].IsArray()) {
                chart->SetPoints(ParseNumberArray(props["points"]));
            } else if (props["points"].IsString()) {
                chart->SetPoints(ParseNumberList(props["points"].stringValue));
            }
            if (props["lineColor"].IsString()) {
                chart->lineColor = LayoutParser::ColorFromString(props["lineColor"].stringValue);
            }
            if (props["baselineColor"].IsString()) {
                chart->baselineColor = LayoutParser::ColorFromString(props["baselineColor"].stringValue);
            }
            if (props["background"].IsString()) {
                chart->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                chart->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["strokeWidth"].IsNumber()) {
                chart->strokeWidth = static_cast<float>(props["strokeWidth"].numberValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, chart->cornerRadius);
            if (props["minValue"].IsNumber() && props["maxValue"].IsNumber()) {
                chart->SetRange(
                    static_cast<float>(props["minValue"].numberValue),
                    static_cast<float>(props["maxValue"].numberValue));
            }
            ApplyCommonJsonProps(*chart, props);
        }
        element = std::move(chart);
    } else if (type == "DataTable") {
        auto table = std::make_unique<DataTable>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];

            if (props["columns"].IsArray()) {
                std::vector<DataTable::Column> columns;
                for (const auto& colNode : props["columns"].arrayValue) {
                    DataTable::Column column;
                    if (colNode.IsString()) {
                        column.title = LayoutParser::Utf8ToUtf16(colNode.stringValue);
                    } else if (colNode.IsObject()) {
                        if (colNode["title"].IsString()) {
                            column.title = LayoutParser::Utf8ToUtf16(colNode["title"].stringValue);
                        } else if (colNode["name"].IsString()) {
                            column.title = LayoutParser::Utf8ToUtf16(colNode["name"].stringValue);
                        }
                        if (colNode["width"].IsNumber()) {
                            column.width = static_cast<float>(colNode["width"].numberValue);
                        }
                    }
                    if (column.title.empty()) {
                        column.title = L"Column";
                    }
                    columns.push_back(std::move(column));
                }
                table->SetColumns(std::move(columns));
            } else if (props["columns"].IsString()) {
                std::vector<DataTable::Column> columns;
                for (const auto& title : ParseWideStringList(props["columns"].stringValue, '|')) {
                    columns.push_back(DataTable::Column{title, -1.0f});
                }
                table->SetColumns(std::move(columns));
            }

            if (props["columnWidths"].IsArray()) {
                std::vector<float> widths;
                for (const auto& widthNode : props["columnWidths"].arrayValue) {
                    if (widthNode.IsNumber()) {
                        widths.push_back(static_cast<float>(widthNode.numberValue));
                    }
                }
                table->SetColumnWidths(std::move(widths));
            } else if (props["columnWidths"].IsString()) {
                table->SetColumnWidths(ParseNumberList(props["columnWidths"].stringValue, '|'));
            }

            if (props["rows"].IsArray()) {
                std::vector<std::vector<std::wstring>> rows;
                for (const auto& rowNode : props["rows"].arrayValue) {
                    if (!rowNode.IsArray()) {
                        continue;
                    }
                    std::vector<std::wstring> row;
                    row.reserve(rowNode.arrayValue.size());
                    for (const auto& cellNode : rowNode.arrayValue) {
                        row.push_back(JsonCellToWide(cellNode));
                    }
                    rows.push_back(std::move(row));
                }
                table->SetRows(std::move(rows));
            } else if (props["rows"].IsString()) {
                table->SetRows(ParseWideStringRows(props["rows"].stringValue));
            }

            if (props["background"].IsString()) {
                table->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                table->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["headerBackground"].IsString()) {
                table->headerBackground = LayoutParser::ColorFromString(props["headerBackground"].stringValue);
            }
            if (props["rowBackgroundA"].IsString()) {
                table->rowBackgroundA = LayoutParser::ColorFromString(props["rowBackgroundA"].stringValue);
            }
            if (props["rowBackgroundB"].IsString()) {
                table->rowBackgroundB = LayoutParser::ColorFromString(props["rowBackgroundB"].stringValue);
            }
            if (props["gridLineColor"].IsString()) {
                table->gridLineColor = LayoutParser::ColorFromString(props["gridLineColor"].stringValue);
            }
            if (props["headerTextColor"].IsString()) {
                table->headerTextColor = LayoutParser::ColorFromString(props["headerTextColor"].stringValue);
            }
            if (props["textColor"].IsString()) {
                table->textColor = LayoutParser::ColorFromString(props["textColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, table->cornerRadius);
            if (props["headerHeight"].IsNumber()) {
                table->headerHeight = static_cast<float>(props["headerHeight"].numberValue);
            }
            if (props["rowHeight"].IsNumber()) {
                table->rowHeight = static_cast<float>(props["rowHeight"].numberValue);
            }
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, table->fontSize);
            if (props["headerFontSize"].IsNumber()) {
                table->headerFontSize = static_cast<float>(props["headerFontSize"].numberValue);
            }
            if (props["cellPadding"].IsArray() && props["cellPadding"].arrayValue.size() == 4) {
                table->cellPadding = ParseThickness(props["cellPadding"]);
            }
            ApplyCommonJsonProps(*table, props);
        }
        element = std::move(table);
    } else if (type == "SeagullAnimation") {
        auto animation = std::make_unique<SeagullAnimation>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["count"].IsNumber()) {
                animation->SetCount(static_cast<int>(props["count"].numberValue));
            }
            if (props["speed"].IsNumber()) {
                animation->SetSpeed(static_cast<float>(props["speed"].numberValue));
            }
            if (props["wingAmplitude"].IsNumber()) {
                animation->SetWingAmplitude(static_cast<float>(props["wingAmplitude"].numberValue));
            }
            if (props["pathHeight"].IsNumber()) {
                animation->SetPathHeight(static_cast<float>(props["pathHeight"].numberValue));
            }
            if (props["scale"].IsNumber()) {
                animation->SetScale(static_cast<float>(props["scale"].numberValue));
            }
            if (props["opacity"].IsNumber()) {
                animation->SetOpacity(static_cast<float>(props["opacity"].numberValue));
            }
            if (props["background"].IsString()) {
                animation->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                animation->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["birdColor"].IsString()) {
                animation->birdColor = LayoutParser::ColorFromString(props["birdColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, animation->cornerRadius);
            ApplyCommonJsonProps(*animation, props);
        }
        element = std::move(animation);
    } else if (type == "ProgressBar") {
        auto progressBar = std::make_unique<ProgressBar>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["label"].IsString()) {
                progressBar->SetLabel(LayoutParser::Utf8ToUtf16(props["label"].stringValue));
            }
            if (props["min"].IsNumber()) {
                const float minValue = static_cast<float>(props["min"].numberValue);
                const float maxValue = props["max"].IsNumber()
                    ? static_cast<float>(props["max"].numberValue)
                    : 100.0f;
                progressBar->SetRange(minValue, maxValue);
            } else if (props["max"].IsNumber()) {
                progressBar->SetRange(0.0f, static_cast<float>(props["max"].numberValue));
            }
            if (props["value"].IsNumber()) {
                progressBar->SetValue(static_cast<float>(props["value"].numberValue));
            }
            if (props["step"].IsNumber()) {
                progressBar->SetStep(static_cast<float>(props["step"].numberValue));
            }
            if (props["showValueText"].IsBool()) {
                progressBar->SetShowValueText(props["showValueText"].boolValue);
            }
            if (props["background"].IsString()) {
                progressBar->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                progressBar->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["trackColor"].IsString()) {
                progressBar->trackColor = LayoutParser::ColorFromString(props["trackColor"].stringValue);
            }
            if (props["fillColor"].IsString()) {
                progressBar->fillColor = LayoutParser::ColorFromString(props["fillColor"].stringValue);
            }
            if (props["textColor"].IsString()) {
                progressBar->textColor = LayoutParser::ColorFromString(props["textColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, progressBar->cornerRadius);
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, progressBar->fontSize);
            if (props["barHeight"].IsNumber()) {
                progressBar->barHeight = static_cast<float>(props["barHeight"].numberValue);
            }
            ApplyCommonJsonProps(*progressBar, props);
        }
        element = std::move(progressBar);
    } else if (type == "ListBox") {
        auto listBox = std::make_unique<ListBox>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["items"].IsArray()) {
                listBox->SetItems(ParseWideStringArray(props["items"]));
            } else if (props["items"].IsString()) {
                listBox->SetItems(ParseWideStringList(props["items"].stringValue, '|'));
            }
            if (props["selectedIndex"].IsNumber()) {
                listBox->SetSelectedIndex(static_cast<int>(props["selectedIndex"].numberValue));
            }
            if (props["background"].IsString()) {
                listBox->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                listBox->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["selectedBackground"].IsString()) {
                listBox->selectedBackground = LayoutParser::ColorFromString(props["selectedBackground"].stringValue);
            }
            if (props["hoverBackground"].IsString()) {
                listBox->hoverBackground = LayoutParser::ColorFromString(props["hoverBackground"].stringValue);
            }
            if (props["textColor"].IsString()) {
                listBox->textColor = LayoutParser::ColorFromString(props["textColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, listBox->cornerRadius);
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, listBox->fontSize);
            if (props["itemHeight"].IsNumber()) {
                listBox->itemHeight = static_cast<float>(props["itemHeight"].numberValue);
            }
            ApplyCommonJsonProps(*listBox, props);
        }
        element = std::move(listBox);
    } else if (type == "ComboBox") {
        auto comboBox = std::make_unique<ComboBox>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["items"].IsArray()) {
                comboBox->SetItems(ParseWideStringArray(props["items"]));
            } else if (props["items"].IsString()) {
                comboBox->SetItems(ParseWideStringList(props["items"].stringValue, '|'));
            }
            if (props["selectedIndex"].IsNumber()) {
                comboBox->SetSelectedIndex(static_cast<int>(props["selectedIndex"].numberValue));
            }
            if (props["expanded"].IsBool()) {
                comboBox->SetExpanded(props["expanded"].boolValue);
            }
            if (props["background"].IsString()) {
                comboBox->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["headerBackground"].IsString()) {
                comboBox->headerBackground = LayoutParser::ColorFromString(props["headerBackground"].stringValue);
            }
            if (props["dropdownBackground"].IsString()) {
                comboBox->dropdownBackground = LayoutParser::ColorFromString(props["dropdownBackground"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                comboBox->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["selectedBackground"].IsString()) {
                comboBox->selectedBackground = LayoutParser::ColorFromString(props["selectedBackground"].stringValue);
            }
            if (props["hoverBackground"].IsString()) {
                comboBox->hoverBackground = LayoutParser::ColorFromString(props["hoverBackground"].stringValue);
            }
            if (props["textColor"].IsString()) {
                comboBox->textColor = LayoutParser::ColorFromString(props["textColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, comboBox->cornerRadius);
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, comboBox->fontSize);
            if (props["headerHeight"].IsNumber()) {
                comboBox->headerHeight = static_cast<float>(props["headerHeight"].numberValue);
            }
            if (props["itemHeight"].IsNumber()) {
                comboBox->itemHeight = static_cast<float>(props["itemHeight"].numberValue);
            }
            if (props["maxVisibleItems"].IsNumber()) {
                comboBox->maxVisibleItems = std::max(1, static_cast<int>(props["maxVisibleItems"].numberValue));
            }
            ApplyCommonJsonProps(*comboBox, props);
        }
        element = std::move(comboBox);
    } else if (type == "TabControl") {
        auto tabControl = std::make_unique<TabControl>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["tabs"].IsArray()) {
                tabControl->SetTabs(ParseWideStringArray(props["tabs"]));
            } else if (props["tabs"].IsString()) {
                tabControl->SetTabs(ParseWideStringList(props["tabs"].stringValue, '|'));
            }
            if (props["selectedIndex"].IsNumber()) {
                tabControl->SetSelectedIndex(static_cast<int>(props["selectedIndex"].numberValue));
            }
            if (props["background"].IsString()) {
                tabControl->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["headerBackground"].IsString()) {
                tabControl->headerBackground = LayoutParser::ColorFromString(props["headerBackground"].stringValue);
            }
            if (props["contentBackground"].IsString()) {
                tabControl->contentBackground = LayoutParser::ColorFromString(props["contentBackground"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                tabControl->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["tabBackground"].IsString()) {
                tabControl->tabBackground = LayoutParser::ColorFromString(props["tabBackground"].stringValue);
            }
            if (props["hoverTabBackground"].IsString()) {
                tabControl->hoverTabBackground = LayoutParser::ColorFromString(props["hoverTabBackground"].stringValue);
            }
            if (props["selectedTabBackground"].IsString()) {
                tabControl->selectedTabBackground = LayoutParser::ColorFromString(props["selectedTabBackground"].stringValue);
            }
            if (props["tabTextColor"].IsString()) {
                tabControl->tabTextColor = LayoutParser::ColorFromString(props["tabTextColor"].stringValue);
            }
            if (props["selectedTabTextColor"].IsString()) {
                tabControl->selectedTabTextColor = LayoutParser::ColorFromString(props["selectedTabTextColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, tabControl->cornerRadius);
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, tabControl->fontSize);
            if (props["headerHeight"].IsNumber()) {
                tabControl->headerHeight = static_cast<float>(props["headerHeight"].numberValue);
            }
            ApplyCommonJsonProps(*tabControl, props);
        }
        element = std::move(tabControl);
    } else if (type == "ListView") {
        auto listView = std::make_unique<ListView>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];

            if (props["columns"].IsArray()) {
                std::vector<ListView::Column> columns;
                for (const auto& colNode : props["columns"].arrayValue) {
                    ListView::Column column;
                    if (colNode.IsString()) {
                        column.title = LayoutParser::Utf8ToUtf16(colNode.stringValue);
                    } else if (colNode.IsObject()) {
                        if (colNode["title"].IsString()) {
                            column.title = LayoutParser::Utf8ToUtf16(colNode["title"].stringValue);
                        } else if (colNode["name"].IsString()) {
                            column.title = LayoutParser::Utf8ToUtf16(colNode["name"].stringValue);
                        }
                        if (colNode["width"].IsNumber()) {
                            column.width = static_cast<float>(colNode["width"].numberValue);
                        }
                    }
                    if (column.title.empty()) {
                        column.title = L"Column";
                    }
                    columns.push_back(std::move(column));
                }
                listView->SetColumns(std::move(columns));
            } else if (props["columns"].IsString()) {
                std::vector<ListView::Column> columns;
                for (const auto& title : ParseWideStringList(props["columns"].stringValue, '|')) {
                    columns.push_back(ListView::Column{title, -1.0f});
                }
                listView->SetColumns(std::move(columns));
            }

            if (props["columnWidths"].IsArray()) {
                std::vector<float> widths;
                for (const auto& widthNode : props["columnWidths"].arrayValue) {
                    if (widthNode.IsNumber()) {
                        widths.push_back(static_cast<float>(widthNode.numberValue));
                    }
                }
                listView->SetColumnWidths(std::move(widths));
            } else if (props["columnWidths"].IsString()) {
                listView->SetColumnWidths(ParseNumberList(props["columnWidths"].stringValue, '|'));
            }

            if (props["rows"].IsArray()) {
                std::vector<std::vector<std::wstring>> rows;
                for (const auto& rowNode : props["rows"].arrayValue) {
                    if (!rowNode.IsArray()) {
                        continue;
                    }
                    std::vector<std::wstring> row;
                    row.reserve(rowNode.arrayValue.size());
                    for (const auto& cellNode : rowNode.arrayValue) {
                        row.push_back(JsonCellToWide(cellNode));
                    }
                    rows.push_back(std::move(row));
                }
                listView->SetRows(std::move(rows));
            } else if (props["rows"].IsString()) {
                listView->SetRows(ParseWideStringRows(props["rows"].stringValue));
            }

            if (props["selectedIndex"].IsNumber()) {
                listView->SetSelectedIndex(static_cast<int>(props["selectedIndex"].numberValue));
            }
            if (props["background"].IsString()) {
                listView->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                listView->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["headerBackground"].IsString()) {
                listView->headerBackground = LayoutParser::ColorFromString(props["headerBackground"].stringValue);
            }
            if (props["rowBackgroundA"].IsString()) {
                listView->rowBackgroundA = LayoutParser::ColorFromString(props["rowBackgroundA"].stringValue);
            }
            if (props["rowBackgroundB"].IsString()) {
                listView->rowBackgroundB = LayoutParser::ColorFromString(props["rowBackgroundB"].stringValue);
            }
            if (props["hoverRowBackground"].IsString()) {
                listView->hoverRowBackground = LayoutParser::ColorFromString(props["hoverRowBackground"].stringValue);
            }
            if (props["selectedRowBackground"].IsString()) {
                listView->selectedRowBackground = LayoutParser::ColorFromString(props["selectedRowBackground"].stringValue);
            }
            if (props["gridLineColor"].IsString()) {
                listView->gridLineColor = LayoutParser::ColorFromString(props["gridLineColor"].stringValue);
            }
            if (props["headerTextColor"].IsString()) {
                listView->headerTextColor = LayoutParser::ColorFromString(props["headerTextColor"].stringValue);
            }
            if (props["textColor"].IsString()) {
                listView->textColor = LayoutParser::ColorFromString(props["textColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, listView->cornerRadius);
            if (props["headerHeight"].IsNumber()) {
                listView->headerHeight = static_cast<float>(props["headerHeight"].numberValue);
            }
            if (props["rowHeight"].IsNumber()) {
                listView->rowHeight = static_cast<float>(props["rowHeight"].numberValue);
            }
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, listView->fontSize);
            if (props["headerFontSize"].IsNumber()) {
                listView->headerFontSize = static_cast<float>(props["headerFontSize"].numberValue);
            }
            if (props["cellPadding"].IsArray() && props["cellPadding"].arrayValue.size() == 4) {
                listView->cellPadding = ParseThickness(props["cellPadding"]);
            }
            ApplyCommonJsonProps(*listView, props);
        }
        element = std::move(listView);
    } else if (type == "TreeView") {
        auto treeView = std::make_unique<TreeView>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["items"].IsArray()) {
                treeView->SetPaths(ParseWideStringArray(props["items"]));
            } else if (props["items"].IsString()) {
                treeView->SetPaths(ParseWideStringPaths(props["items"].stringValue));
            }
            if (props["expandedAll"].IsBool()) {
                treeView->SetExpandedAll(props["expandedAll"].boolValue);
            }
            if (props["background"].IsString()) {
                treeView->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["borderColor"].IsString()) {
                treeView->borderColor = LayoutParser::ColorFromString(props["borderColor"].stringValue);
            }
            if (props["selectedBackground"].IsString()) {
                treeView->selectedBackground = LayoutParser::ColorFromString(props["selectedBackground"].stringValue);
            }
            if (props["hoverBackground"].IsString()) {
                treeView->hoverBackground = LayoutParser::ColorFromString(props["hoverBackground"].stringValue);
            }
            if (props["textColor"].IsString()) {
                treeView->textColor = LayoutParser::ColorFromString(props["textColor"].stringValue);
            }
            if (props["mutedTextColor"].IsString()) {
                treeView->mutedTextColor = LayoutParser::ColorFromString(props["mutedTextColor"].stringValue);
            }
            if (props["glyphColor"].IsString()) {
                treeView->glyphColor = LayoutParser::ColorFromString(props["glyphColor"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, treeView->cornerRadius);
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, treeView->fontSize);
            if (props["itemHeight"].IsNumber()) {
                treeView->itemHeight = static_cast<float>(props["itemHeight"].numberValue);
            }
            if (props["indentWidth"].IsNumber()) {
                treeView->indentWidth = static_cast<float>(props["indentWidth"].numberValue);
            }
            ApplyCommonJsonProps(*treeView, props);
        }
        element = std::move(treeView);
    } else if (type == "Button") {
        std::wstring text = L"Button";
        auto button = std::make_unique<Button>(std::move(text));
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["text"].IsString()) {
                button->SetText(LayoutParser::Utf8ToUtf16(props["text"].stringValue));
            }
            TryAssignNumber(props["fontSize"], Theme::NumberCategory::FontSize, button->fontSize);
            if (props["background"].IsString()) {
                button->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["foreground"].IsString()) {
                button->foreground = LayoutParser::ColorFromString(props["foreground"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, button->cornerRadius);
            ApplyCommonJsonProps(*button, props);
        }
        if (node["events"].IsObject() && node["events"]["onClick"].IsString()) {
            const std::string eventId = node["events"]["onClick"].stringValue;
            auto handler = eventResolver(eventId);
            if (handler) {
                button->SetOnClick(std::move(handler));
            }
        }
        element = std::move(button);
    } else if (type == "TextInput") {
        std::wstring text = L"";
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["text"].IsString()) {
                text = LayoutParser::Utf8ToUtf16(props["text"].stringValue);
            }
        }
        auto input = std::make_unique<TextInput>(std::move(text));
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["fontSize"].IsNumber()) {
                input->SetFontSize(static_cast<float>(props["fontSize"].numberValue));
            }
            if (props["background"].IsString()) {
                input->SetBackgroundColor(LayoutParser::ColorFromString(props["background"].stringValue));
            }
            if (props["textColor"].IsString()) {
                input->SetTextColor(LayoutParser::ColorFromString(props["textColor"].stringValue));
            }
            if (props["cornerRadius"].IsNumber()) {
                input->SetCornerRadius(static_cast<float>(props["cornerRadius"].numberValue));
            }
            ApplyCommonJsonProps(*input, props);
        }
        element = std::move(input);
    } else if (type == "Checkbox") {
        std::wstring text = L"";
        auto checkbox = std::make_unique<Checkbox>(std::move(text));
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["text"].IsString()) {
                checkbox->SetText(LayoutParser::Utf8ToUtf16(props["text"].stringValue));
            }
            if (props["checked"].IsBool()) {
                checkbox->SetChecked(props["checked"].boolValue);
            }
            ApplyCommonJsonProps(*checkbox, props);
        }
        element = std::move(checkbox);
    } else if (type == "RadioButton") {
        std::wstring text = L"";
        std::wstring group = L"default";
        auto radio = std::make_unique<RadioButton>(std::move(text), std::move(group));
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["text"].IsString()) {
                radio->SetText(LayoutParser::Utf8ToUtf16(props["text"].stringValue));
            }
            if (props["group"].IsString()) {
                radio->SetGroup(LayoutParser::Utf8ToUtf16(props["group"].stringValue));
            }
            if (props["checked"].IsBool()) {
                radio->SetChecked(props["checked"].boolValue);
            }
            ApplyCommonJsonProps(*radio, props);
        }
        element = std::move(radio);
    } else if (type == "Slider") {
        std::wstring label = L"";
        auto slider = std::make_unique<Slider>(std::move(label));
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["label"].IsString()) {
                slider->SetText(LayoutParser::Utf8ToUtf16(props["label"].stringValue));
            }
            if (props["min"].IsNumber() && props["max"].IsNumber()) {
                slider->SetRange(static_cast<float>(props["min"].numberValue), static_cast<float>(props["max"].numberValue));
            }
            if (props["value"].IsNumber()) {
                slider->SetValue(static_cast<float>(props["value"].numberValue));
            }
            if (props["step"].IsNumber()) {
                slider->SetStep(static_cast<float>(props["step"].numberValue));
            }
            ApplyCommonJsonProps(*slider, props);
        }
        element = std::move(slider);
    } else if (type == "Grid") {
        auto grid = std::make_unique<GridPanel>();
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["background"].IsString()) {
                grid->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            TryAssignNumber(props["cornerRadius"], Theme::NumberCategory::Radius, grid->cornerRadius);
            if (props["spacing"].IsNumber()) {
                grid->cellSpacing = static_cast<float>(props["spacing"].numberValue);
            } else if (props["cellSpacing"].IsNumber()) {
                grid->cellSpacing = static_cast<float>(props["cellSpacing"].numberValue);
            }
            if (props["columns"].IsNumber()) {
                grid->columns = static_cast<int>(props["columns"].numberValue);
            } else if (props["cols"].IsNumber()) {
                grid->columns = std::max(1, static_cast<int>(props["cols"].numberValue));
            }
            if (props["padding"].IsArray() && props["padding"].arrayValue.size() == 4) {
                grid->padding = ParseThickness(props["padding"]);
            }
            if (props["rowHeight"].IsNumber()) {
                grid->rowHeight = static_cast<float>(props["rowHeight"].numberValue);
            }
            ApplyCommonJsonProps(*grid, props);
        }
        element = std::move(grid);
    } else if (type == "Image") {
        std::wstring source = L"";
        if (node["props"].IsObject() && node["props"]["source"].IsString()) {
            source = LayoutParser::Utf8ToUtf16(node["props"]["source"].stringValue);
        }
        auto image = std::make_unique<Image>(std::move(source));
        if (node["props"].IsObject()) {
            if (node["props"]["cornerRadius"].IsNumber()) {
                image->cornerRadius = static_cast<float>(node["props"]["cornerRadius"].numberValue);
            }
            if (node["props"]["stretch"].IsString()) {
                const std::string stretchMode = node["props"]["stretch"].stringValue;
                if (stretchMode == "fill") {
                    image->stretch = Image::StretchMode::Fill;
                } else if (stretchMode == "uniformToFill") {
                    image->stretch = Image::StretchMode::UniformToFill;
                } else {
                    image->stretch = Image::StretchMode::Uniform;
                }
            }
            if (node["props"]["source"].IsString()) {
                const std::string sourcePath = node["props"]["source"].stringValue;
                if (provider.Exists(sourcePath)) {
                    image->SetImageData(provider.LoadBytes(sourcePath));
                }
            }
            ApplyCommonJsonProps(*image, node["props"]);
        }
        element = std::move(image);
    } else if (type == "Spacer") {
        auto spacer = std::make_unique<Spacer>();
        if (node["props"].IsObject()) {
            ApplyCommonJsonProps(*spacer, node["props"]);
        }
        element = std::move(spacer);
    }

    if (!element) {
        return nullptr;
    }

    if (node["children"].IsArray()) {
        for (const auto& childNode : node["children"].arrayValue) {
            if (auto child = CreateElementFromJson(childNode, provider, eventResolver)) {
                element->AddChild(std::move(child));
            }
        }
    }

    return element;
}

std::unique_ptr<UIElement> CreateElementFromXml(const XmlNode& node, IResourceProvider& provider, UIEventResolver eventResolver) {
    if (node.name.empty()) {
        return nullptr;
    }

    std::unique_ptr<UIElement> element;
    if (node.name == "Panel") {
        auto panel = std::make_unique<Panel>();
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            panel->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            panel->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("spacing"); it != node.attributes.end()) {
            panel->spacing = std::stof(it->second);
        }
        if (auto it = node.attributes.find("padding"); it != node.attributes.end()) {
            const auto values = SplitString(it->second, ',');
            if (values.size() == 4) {
                panel->padding = ParseThickness(values);
            }
        }
        if (auto it = node.attributes.find("direction"); it != node.attributes.end()) {
            panel->direction = ParseDirection(it->second);
        }
        if (auto it = node.attributes.find("wrap"); it != node.attributes.end()) {
            panel->wrap = ParseWrap(it->second);
        }
        if (auto it = node.attributes.find("alignItems"); it != node.attributes.end()) {
            panel->alignItems = ParseAlignItems(it->second);
        } else if (auto it2 = node.attributes.find("align-items"); it2 != node.attributes.end()) {
            panel->alignItems = ParseAlignItems(it2->second);
        }
        if (auto it = node.attributes.find("justifyContent"); it != node.attributes.end()) {
            panel->justifyContent = ParseJustifyContent(it->second);
        } else if (auto it2 = node.attributes.find("justify-content"); it2 != node.attributes.end()) {
            panel->justifyContent = ParseJustifyContent(it2->second);
        }
        ApplyCommonXmlAttributes(*panel, node);
        element = std::move(panel);
    } else if (node.name == "Label") {
        std::wstring text = L"";
        auto label = std::make_unique<Label>(std::move(text));
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            label->SetText(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            label->m_fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("color"); it != node.attributes.end()) {
            label->m_color = LayoutParser::ColorFromString(it->second);
        }
        ApplyCommonXmlAttributes(*label, node);
        element = std::move(label);
    } else if (node.name == "StatCard") {
        auto statCard = std::make_unique<StatCard>();
        if (auto it = node.attributes.find("title"); it != node.attributes.end()) {
            statCard->SetTitle(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("value"); it != node.attributes.end()) {
            statCard->SetValue(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("deltaText"); it != node.attributes.end()) {
            statCard->SetDeltaText(LayoutParser::Utf8ToUtf16(it->second));
        } else if (auto it2 = node.attributes.find("delta"); it2 != node.attributes.end()) {
            statCard->SetDeltaText(LayoutParser::Utf8ToUtf16(it2->second));
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            statCard->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            statCard->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("accentColor"); it != node.attributes.end()) {
            statCard->accentColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("titleColor"); it != node.attributes.end()) {
            statCard->titleColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("valueColor"); it != node.attributes.end()) {
            statCard->valueColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("deltaColor"); it != node.attributes.end()) {
            statCard->deltaColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            statCard->cornerRadius = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*statCard, node);
        element = std::move(statCard);
    } else if (node.name == "SparklineChart") {
        auto chart = std::make_unique<SparklineChart>();
        if (auto it = node.attributes.find("points"); it != node.attributes.end()) {
            chart->SetPoints(ParseNumberList(it->second));
        }
        if (auto it = node.attributes.find("lineColor"); it != node.attributes.end()) {
            chart->lineColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("baselineColor"); it != node.attributes.end()) {
            chart->baselineColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            chart->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            chart->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("strokeWidth"); it != node.attributes.end()) {
            chart->strokeWidth = std::stof(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            chart->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("minValue"); it != node.attributes.end()) {
            if (auto it2 = node.attributes.find("maxValue"); it2 != node.attributes.end()) {
                chart->SetRange(std::stof(it->second), std::stof(it2->second));
            }
        }
        ApplyCommonXmlAttributes(*chart, node);
        element = std::move(chart);
    } else if (node.name == "DataTable") {
        auto table = std::make_unique<DataTable>();
        if (auto it = node.attributes.find("columns"); it != node.attributes.end()) {
            std::vector<DataTable::Column> columns;
            for (const auto& title : ParseWideStringList(it->second, '|')) {
                columns.push_back(DataTable::Column{title, -1.0f});
            }
            table->SetColumns(std::move(columns));
        }
        if (auto it = node.attributes.find("columnWidths"); it != node.attributes.end()) {
            table->SetColumnWidths(ParseNumberList(it->second, '|'));
        }
        if (auto it = node.attributes.find("rows"); it != node.attributes.end()) {
            table->SetRows(ParseWideStringRows(it->second));
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            table->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            table->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("headerBackground"); it != node.attributes.end()) {
            table->headerBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("rowBackgroundA"); it != node.attributes.end()) {
            table->rowBackgroundA = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("rowBackgroundB"); it != node.attributes.end()) {
            table->rowBackgroundB = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("gridLineColor"); it != node.attributes.end()) {
            table->gridLineColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("headerTextColor"); it != node.attributes.end()) {
            table->headerTextColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("textColor"); it != node.attributes.end()) {
            table->textColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            table->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("headerHeight"); it != node.attributes.end()) {
            table->headerHeight = std::stof(it->second);
        }
        if (auto it = node.attributes.find("rowHeight"); it != node.attributes.end()) {
            table->rowHeight = std::stof(it->second);
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            table->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("headerFontSize"); it != node.attributes.end()) {
            table->headerFontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("cellPadding"); it != node.attributes.end()) {
            const auto values = SplitString(it->second, ',');
            if (values.size() == 4) {
                table->cellPadding = ParseThickness(values);
            }
        }
        ApplyCommonXmlAttributes(*table, node);
        element = std::move(table);
    } else if (node.name == "SeagullAnimation") {
        auto animation = std::make_unique<SeagullAnimation>();
        if (auto it = node.attributes.find("count"); it != node.attributes.end()) {
            animation->SetCount(std::stoi(it->second));
        }
        if (auto it = node.attributes.find("speed"); it != node.attributes.end()) {
            animation->SetSpeed(std::stof(it->second));
        }
        if (auto it = node.attributes.find("wingAmplitude"); it != node.attributes.end()) {
            animation->SetWingAmplitude(std::stof(it->second));
        }
        if (auto it = node.attributes.find("pathHeight"); it != node.attributes.end()) {
            animation->SetPathHeight(std::stof(it->second));
        }
        if (auto it = node.attributes.find("scale"); it != node.attributes.end()) {
            animation->SetScale(std::stof(it->second));
        }
        if (auto it = node.attributes.find("opacity"); it != node.attributes.end()) {
            animation->SetOpacity(std::stof(it->second));
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            animation->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            animation->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("birdColor"); it != node.attributes.end()) {
            animation->birdColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            animation->cornerRadius = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*animation, node);
        element = std::move(animation);
    } else if (node.name == "ProgressBar") {
        auto progressBar = std::make_unique<ProgressBar>();
        if (auto it = node.attributes.find("label"); it != node.attributes.end()) {
            progressBar->SetLabel(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("min"); it != node.attributes.end()) {
            const float minValue = std::stof(it->second);
            if (auto it2 = node.attributes.find("max"); it2 != node.attributes.end()) {
                progressBar->SetRange(minValue, std::stof(it2->second));
            } else {
                progressBar->SetRange(minValue, 100.0f);
            }
        } else if (auto it = node.attributes.find("max"); it != node.attributes.end()) {
            progressBar->SetRange(0.0f, std::stof(it->second));
        }
        if (auto it = node.attributes.find("value"); it != node.attributes.end()) {
            progressBar->SetValue(std::stof(it->second));
        }
        if (auto it = node.attributes.find("step"); it != node.attributes.end()) {
            progressBar->SetStep(std::stof(it->second));
        }
        if (auto it = node.attributes.find("showValueText"); it != node.attributes.end()) {
            progressBar->SetShowValueText(ToLowerAscii(it->second) == "true");
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            progressBar->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            progressBar->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("trackColor"); it != node.attributes.end()) {
            progressBar->trackColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("fillColor"); it != node.attributes.end()) {
            progressBar->fillColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("textColor"); it != node.attributes.end()) {
            progressBar->textColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            progressBar->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            progressBar->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("barHeight"); it != node.attributes.end()) {
            progressBar->barHeight = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*progressBar, node);
        element = std::move(progressBar);
    } else if (node.name == "ListBox") {
        auto listBox = std::make_unique<ListBox>();
        if (auto it = node.attributes.find("items"); it != node.attributes.end()) {
            listBox->SetItems(ParseWideStringList(it->second, '|'));
        }
        if (auto it = node.attributes.find("selectedIndex"); it != node.attributes.end()) {
            listBox->SetSelectedIndex(std::stoi(it->second));
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            listBox->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            listBox->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("selectedBackground"); it != node.attributes.end()) {
            listBox->selectedBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("hoverBackground"); it != node.attributes.end()) {
            listBox->hoverBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("textColor"); it != node.attributes.end()) {
            listBox->textColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            listBox->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            listBox->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("itemHeight"); it != node.attributes.end()) {
            listBox->itemHeight = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*listBox, node);
        element = std::move(listBox);
    } else if (node.name == "ComboBox") {
        auto comboBox = std::make_unique<ComboBox>();
        if (auto it = node.attributes.find("items"); it != node.attributes.end()) {
            comboBox->SetItems(ParseWideStringList(it->second, '|'));
        }
        if (auto it = node.attributes.find("selectedIndex"); it != node.attributes.end()) {
            comboBox->SetSelectedIndex(std::stoi(it->second));
        }
        if (auto it = node.attributes.find("expanded"); it != node.attributes.end()) {
            comboBox->SetExpanded(ToLowerAscii(it->second) == "true");
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            comboBox->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("headerBackground"); it != node.attributes.end()) {
            comboBox->headerBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("dropdownBackground"); it != node.attributes.end()) {
            comboBox->dropdownBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            comboBox->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("selectedBackground"); it != node.attributes.end()) {
            comboBox->selectedBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("hoverBackground"); it != node.attributes.end()) {
            comboBox->hoverBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("textColor"); it != node.attributes.end()) {
            comboBox->textColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            comboBox->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            comboBox->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("headerHeight"); it != node.attributes.end()) {
            comboBox->headerHeight = std::stof(it->second);
        }
        if (auto it = node.attributes.find("itemHeight"); it != node.attributes.end()) {
            comboBox->itemHeight = std::stof(it->second);
        }
        if (auto it = node.attributes.find("maxVisibleItems"); it != node.attributes.end()) {
            comboBox->maxVisibleItems = std::max(1, std::stoi(it->second));
        }
        ApplyCommonXmlAttributes(*comboBox, node);
        element = std::move(comboBox);
    } else if (node.name == "TabControl") {
        auto tabControl = std::make_unique<TabControl>();
        if (auto it = node.attributes.find("tabs"); it != node.attributes.end()) {
            tabControl->SetTabs(ParseWideStringList(it->second, '|'));
        }
        if (auto it = node.attributes.find("selectedIndex"); it != node.attributes.end()) {
            tabControl->SetSelectedIndex(std::stoi(it->second));
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            tabControl->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("headerBackground"); it != node.attributes.end()) {
            tabControl->headerBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("contentBackground"); it != node.attributes.end()) {
            tabControl->contentBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            tabControl->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("tabBackground"); it != node.attributes.end()) {
            tabControl->tabBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("hoverTabBackground"); it != node.attributes.end()) {
            tabControl->hoverTabBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("selectedTabBackground"); it != node.attributes.end()) {
            tabControl->selectedTabBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("tabTextColor"); it != node.attributes.end()) {
            tabControl->tabTextColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("selectedTabTextColor"); it != node.attributes.end()) {
            tabControl->selectedTabTextColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            tabControl->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            tabControl->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("headerHeight"); it != node.attributes.end()) {
            tabControl->headerHeight = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*tabControl, node);
        element = std::move(tabControl);
    } else if (node.name == "ListView") {
        auto listView = std::make_unique<ListView>();
        if (auto it = node.attributes.find("columns"); it != node.attributes.end()) {
            std::vector<ListView::Column> columns;
            for (const auto& title : ParseWideStringList(it->second, '|')) {
                columns.push_back(ListView::Column{title, -1.0f});
            }
            listView->SetColumns(std::move(columns));
        }
        if (auto it = node.attributes.find("columnWidths"); it != node.attributes.end()) {
            listView->SetColumnWidths(ParseNumberList(it->second, '|'));
        }
        if (auto it = node.attributes.find("rows"); it != node.attributes.end()) {
            listView->SetRows(ParseWideStringRows(it->second));
        }
        if (auto it = node.attributes.find("selectedIndex"); it != node.attributes.end()) {
            listView->SetSelectedIndex(std::stoi(it->second));
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            listView->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            listView->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("headerBackground"); it != node.attributes.end()) {
            listView->headerBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("rowBackgroundA"); it != node.attributes.end()) {
            listView->rowBackgroundA = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("rowBackgroundB"); it != node.attributes.end()) {
            listView->rowBackgroundB = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("hoverRowBackground"); it != node.attributes.end()) {
            listView->hoverRowBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("selectedRowBackground"); it != node.attributes.end()) {
            listView->selectedRowBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("gridLineColor"); it != node.attributes.end()) {
            listView->gridLineColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("headerTextColor"); it != node.attributes.end()) {
            listView->headerTextColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("textColor"); it != node.attributes.end()) {
            listView->textColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            listView->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("headerHeight"); it != node.attributes.end()) {
            listView->headerHeight = std::stof(it->second);
        }
        if (auto it = node.attributes.find("rowHeight"); it != node.attributes.end()) {
            listView->rowHeight = std::stof(it->second);
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            listView->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("headerFontSize"); it != node.attributes.end()) {
            listView->headerFontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("cellPadding"); it != node.attributes.end()) {
            const auto values = SplitString(it->second, ',');
            if (values.size() == 4) {
                listView->cellPadding = ParseThickness(values);
            }
        }
        ApplyCommonXmlAttributes(*listView, node);
        element = std::move(listView);
    } else if (node.name == "TreeView") {
        auto treeView = std::make_unique<TreeView>();
        if (auto it = node.attributes.find("items"); it != node.attributes.end()) {
            treeView->SetPaths(ParseWideStringPaths(it->second));
        }
        if (auto it = node.attributes.find("expandedAll"); it != node.attributes.end()) {
            treeView->SetExpandedAll(ToLowerAscii(it->second) == "true");
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            treeView->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("borderColor"); it != node.attributes.end()) {
            treeView->borderColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("selectedBackground"); it != node.attributes.end()) {
            treeView->selectedBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("hoverBackground"); it != node.attributes.end()) {
            treeView->hoverBackground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("textColor"); it != node.attributes.end()) {
            treeView->textColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("mutedTextColor"); it != node.attributes.end()) {
            treeView->mutedTextColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("glyphColor"); it != node.attributes.end()) {
            treeView->glyphColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            treeView->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            treeView->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("itemHeight"); it != node.attributes.end()) {
            treeView->itemHeight = std::stof(it->second);
        }
        if (auto it = node.attributes.find("indentWidth"); it != node.attributes.end()) {
            treeView->indentWidth = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*treeView, node);
        element = std::move(treeView);
    } else if (node.name == "Button") {
        std::wstring text = L"Button";
        auto button = std::make_unique<Button>(std::move(text));
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            button->SetText(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            button->fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            button->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("foreground"); it != node.attributes.end()) {
            button->foreground = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            button->cornerRadius = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*button, node);
        if (auto it = node.attributes.find("onClick"); it != node.attributes.end()) {
            auto handler = eventResolver(it->second);
            if (handler) {
                button->SetOnClick(std::move(handler));
            }
        }
        element = std::move(button);
    } else if (node.name == "TextInput") {
        std::wstring text = L"";
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            text = LayoutParser::Utf8ToUtf16(it->second);
        }
        auto input = std::make_unique<TextInput>(std::move(text));
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            input->SetFontSize(std::stof(it->second));
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            input->SetBackgroundColor(LayoutParser::ColorFromString(it->second));
        }
        if (auto it = node.attributes.find("textColor"); it != node.attributes.end()) {
            input->SetTextColor(LayoutParser::ColorFromString(it->second));
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            input->SetCornerRadius(std::stof(it->second));
        }
        ApplyCommonXmlAttributes(*input, node);
        element = std::move(input);
    } else if (node.name == "Checkbox") {
        std::wstring text = L"";
        auto checkbox = std::make_unique<Checkbox>(std::move(text));
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            checkbox->SetText(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("checked"); it != node.attributes.end()) {
            checkbox->SetChecked(ToLowerAscii(it->second) == "true");
        }
        ApplyCommonXmlAttributes(*checkbox, node);
        element = std::move(checkbox);
    } else if (node.name == "RadioButton") {
        std::wstring text = L"";
        std::wstring group = L"default";
        auto radio = std::make_unique<RadioButton>(std::move(text), std::move(group));
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            radio->SetText(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("group"); it != node.attributes.end()) {
            radio->SetGroup(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("checked"); it != node.attributes.end()) {
            radio->SetChecked(ToLowerAscii(it->second) == "true");
        }
        ApplyCommonXmlAttributes(*radio, node);
        element = std::move(radio);
    } else if (node.name == "Slider") {
        std::wstring label = L"";
        auto slider = std::make_unique<Slider>(std::move(label));
        if (auto it = node.attributes.find("label"); it != node.attributes.end()) {
            slider->SetText(LayoutParser::Utf8ToUtf16(it->second));
        }
        if (auto it = node.attributes.find("min"); it != node.attributes.end()) {
            const float minVal = std::stof(it->second);
            if (auto it2 = node.attributes.find("max"); it2 != node.attributes.end()) {
                const float maxVal = std::stof(it2->second);
                slider->SetRange(minVal, maxVal);
            }
        }
        if (auto it = node.attributes.find("value"); it != node.attributes.end()) {
            slider->SetValue(std::stof(it->second));
        }
        if (auto it = node.attributes.find("step"); it != node.attributes.end()) {
            slider->SetStep(std::stof(it->second));
        }
        ApplyCommonXmlAttributes(*slider, node);
        element = std::move(slider);
    } else if (node.name == "Grid") {
        auto grid = std::make_unique<GridPanel>();
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            grid->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            grid->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("spacing"); it != node.attributes.end()) {
            grid->cellSpacing = std::stof(it->second);
        } else if (auto it2 = node.attributes.find("cellSpacing"); it2 != node.attributes.end()) {
            grid->cellSpacing = std::stof(it2->second);
        }
        if (auto it = node.attributes.find("columns"); it != node.attributes.end()) {
            grid->columns = std::max(1, std::stoi(it->second));
        } else if (auto it2 = node.attributes.find("cols"); it2 != node.attributes.end()) {
            grid->columns = std::max(1, std::stoi(it2->second));
        }
        if (auto it = node.attributes.find("padding"); it != node.attributes.end()) {
            const auto values = SplitString(it->second, ',');
            if (values.size() == 4) {
                grid->padding = ParseThickness(values);
            }
        }
        if (auto it = node.attributes.find("rowHeight"); it != node.attributes.end()) {
            grid->rowHeight = std::stof(it->second);
        }
        ApplyCommonXmlAttributes(*grid, node);
        element = std::move(grid);
    } else if (node.name == "Image") {
        std::wstring source = L"";
        if (auto it = node.attributes.find("source"); it != node.attributes.end()) {
            source = LayoutParser::Utf8ToUtf16(it->second);
        }
        auto image = std::make_unique<Image>(std::move(source));
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            image->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("stretch"); it != node.attributes.end()) {
            const std::string stretchMode = it->second;
            if (stretchMode == "fill") {
                image->stretch = Image::StretchMode::Fill;
            } else if (stretchMode == "uniformToFill") {
                image->stretch = Image::StretchMode::UniformToFill;
            } else {
                image->stretch = Image::StretchMode::Uniform;
            }
        }
        if (auto it = node.attributes.find("source"); it != node.attributes.end()) {
            if (provider.Exists(it->second)) {
                image->SetImageData(provider.LoadBytes(it->second));
            }
        }
        ApplyCommonXmlAttributes(*image, node);
        element = std::move(image);
    } else if (node.name == "Spacer") {
        auto spacer = std::make_unique<Spacer>();
        ApplyCommonXmlAttributes(*spacer, node);
        element = std::move(spacer);
    }

    if (!element) {
        return nullptr;
    }

    for (const auto& child : node.children) {
        if (auto childElement = CreateElementFromXml(child, provider, eventResolver)) {
            element->AddChild(std::move(childElement));
        }
    }

    return element;
}
