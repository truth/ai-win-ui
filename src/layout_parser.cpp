#include "layout_parser.h"
#include "resource_provider.h"

#include <stdexcept>
#include <sstream>
#include <windows.h>

namespace {

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
    if (value.IsArray() && value.arrayValue.size() == 4) {
        thickness.left = static_cast<float>(value[0].numberValue);
        thickness.top = static_cast<float>(value[1].numberValue);
        thickness.right = static_cast<float>(value[2].numberValue);
        thickness.bottom = static_cast<float>(value[3].numberValue);
    }
    return thickness;
}

Panel::AlignItems ParseAlignItems(const std::string& value) {
    const std::string normalized = ToLowerAscii(TrimString(value));
    if (normalized == "start" || normalized == "left") {
        return Panel::AlignItems::Start;
    }
    if (normalized == "center") {
        return Panel::AlignItems::Center;
    }
    if (normalized == "end" || normalized == "right") {
        return Panel::AlignItems::End;
    }
    return Panel::AlignItems::Stretch;
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
    if (normalized == "end") {
        return Panel::JustifyContent::End;
    }
    if (normalized == "space-between") {
        return Panel::JustifyContent::SpaceBetween;
    }
    return Panel::JustifyContent::Start;
}

void ApplyCommonJsonProps(UIElement& element, const JsonValue& props) {
    if (!props.IsObject()) {
        return;
    }

    if (props["width"].IsNumber()) {
        element.SetFixedWidth(static_cast<float>(props["width"].numberValue));
    }
    if (props["height"].IsNumber()) {
        element.SetFixedHeight(static_cast<float>(props["height"].numberValue));
    }
    if (props["margin"].IsArray() && props["margin"].arrayValue.size() == 4) {
        element.SetMargin(ParseThickness(props["margin"]));
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
}

void ApplyCommonXmlAttributes(UIElement& element, const XmlNode& node) {
    if (auto it = node.attributes.find("width"); it != node.attributes.end()) {
        element.SetFixedWidth(std::stof(TrimString(it->second)));
    }
    if (auto it = node.attributes.find("height"); it != node.attributes.end()) {
        element.SetFixedHeight(std::stof(TrimString(it->second)));
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
}

Color ParseHexColor(const std::string& value) {
    std::string hex = value;
    if (!hex.empty() && hex[0] == '#') {
        hex.erase(0, 1);
    }
    if (hex.size() == 6) {
        unsigned int rgb = 0;
        std::stringstream ss;
        ss << std::hex << hex;
        ss >> rgb;
        float r = ((rgb >> 16) & 0xFF) / 255.0f;
        float g = ((rgb >> 8) & 0xFF) / 255.0f;
        float b = (rgb & 0xFF) / 255.0f;
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
    return ParseHexColor(value);
}

std::unique_ptr<UIElement> CreateElementFromJson(const JsonValue& node, IResourceProvider& provider, UIEventResolver eventResolver);
std::unique_ptr<UIElement> CreateElementFromXml(const XmlNode& node, IResourceProvider& provider, UIEventResolver eventResolver);

std::unique_ptr<UIElement> LayoutParser::BuildFromFile(UIContext& context,
                                                       const std::string& resourcePath) {
    if (!context.resourceProvider) {
        return nullptr;
    }

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
            if (props["cornerRadius"].IsNumber()) {
                panel->cornerRadius = static_cast<float>(props["cornerRadius"].numberValue);
            }
            if (props["spacing"].IsNumber()) {
                panel->spacing = static_cast<float>(props["spacing"].numberValue);
            }
            if (props["padding"].IsArray() && props["padding"].arrayValue.size() == 4) {
                panel->padding = ParseThickness(props["padding"]);
            }
            if (props["direction"].IsString()) {
                panel->direction = ParseDirection(props["direction"].stringValue);
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
            if (props["fontSize"].IsNumber()) {
                label->m_fontSize = static_cast<float>(props["fontSize"].numberValue);
            }
            if (props["color"].IsString()) {
                label->m_color = LayoutParser::ColorFromString(props["color"].stringValue);
            }
            ApplyCommonJsonProps(*label, props);
        }
        element = std::move(label);
    } else if (type == "Button") {
        std::wstring text = L"Button";
        auto button = std::make_unique<Button>(std::move(text));
        if (node["props"].IsObject()) {
            const JsonValue& props = node["props"];
            if (props["text"].IsString()) {
                button->SetText(LayoutParser::Utf8ToUtf16(props["text"].stringValue));
            }
            if (props["fontSize"].IsNumber()) {
                button->fontSize = static_cast<float>(props["fontSize"].numberValue);
            }
            if (props["background"].IsString()) {
                button->background = LayoutParser::ColorFromString(props["background"].stringValue);
            }
            if (props["foreground"].IsString()) {
                button->foreground = LayoutParser::ColorFromString(props["foreground"].stringValue);
            }
            if (props["cornerRadius"].IsNumber()) {
                button->cornerRadius = static_cast<float>(props["cornerRadius"].numberValue);
            }
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
            if (props["cornerRadius"].IsNumber()) {
                grid->cornerRadius = static_cast<float>(props["cornerRadius"].numberValue);
            }
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
