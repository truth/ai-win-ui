#include "layout_parser.h"

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

D2D1_COLOR_F ParseHexColor(const std::string& value) {
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
        return D2D1::ColorF(r, g, b);
    }
    return D2D1::ColorF(D2D1::ColorF::White);
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

D2D1_COLOR_F LayoutParser::ColorFromString(const std::string& value) {
    return ParseHexColor(value);
}

std::unique_ptr<UIElement> CreateElementFromJson(const JsonValue& node, IResourceProvider& provider, LayoutParser::EventResolver eventResolver);
std::unique_ptr<UIElement> CreateElementFromXml(const XmlNode& node, IResourceProvider& provider, LayoutParser::EventResolver eventResolver);

std::unique_ptr<UIElement> LayoutParser::BuildFromFile(IResourceProvider& provider,
                                                      const std::string& resourcePath,
                                                      EventResolver eventResolver) {
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
            return CreateElementFromJson(root, provider, std::move(eventResolver));
        }
        if (isXml) {
            XmlNode root = ParseXml(text);
            return CreateElementFromXml(root, provider, std::move(eventResolver));
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

std::unique_ptr<UIElement> CreateElementFromJson(const JsonValue& node, IResourceProvider& provider, LayoutParser::EventResolver eventResolver) {
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
            if (props["spacing"].IsNumber()) {
                panel->spacing = static_cast<float>(props["spacing"].numberValue);
            }
            if (props["padding"].IsArray() && props["padding"].arrayValue.size() == 4) {
                panel->padding.left = static_cast<float>(props["padding"][0].numberValue);
                panel->padding.top = static_cast<float>(props["padding"][1].numberValue);
                panel->padding.right = static_cast<float>(props["padding"][2].numberValue);
                panel->padding.bottom = static_cast<float>(props["padding"][3].numberValue);
            }
        }
        element = std::move(panel);
    } else if (type == "Label") {
        std::wstring text = L"";
        if (node["props"].IsObject() && node["props"]["text"].IsString()) {
            text = LayoutParser::Utf8ToUtf16(node["props"]["text"].stringValue);
        }
        element = std::make_unique<Label>(std::move(text));
    } else if (type == "Button") {
        std::wstring text = L"Button";
        if (node["props"].IsObject() && node["props"]["text"].IsString()) {
            text = LayoutParser::Utf8ToUtf16(node["props"]["text"].stringValue);
        }
        auto button = std::make_unique<Button>(std::move(text));
        if (node["events"].IsObject() && node["events"]["onClick"].IsString()) {
            const std::string eventId = node["events"]["onClick"].stringValue;
            auto handler = eventResolver(eventId);
            if (handler) {
                button->SetOnClick(std::move(handler));
            }
        }
        element = std::move(button);
    } else if (type == "Image") {
        std::wstring source = L"";
        if (node["props"].IsObject() && node["props"]["source"].IsString()) {
            source = LayoutParser::Utf8ToUtf16(node["props"]["source"].stringValue);
        }
        auto image = std::make_unique<Image>(std::move(source));
        if (node["props"].IsObject() && node["props"]["source"].IsString()) {
            const std::string sourcePath = node["props"]["source"].stringValue;
            if (provider.Exists(sourcePath)) {
                image->SetImageData(provider.LoadBytes(sourcePath));
            }
        }
        element = std::move(image);
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

std::unique_ptr<UIElement> CreateElementFromXml(const XmlNode& node, IResourceProvider& provider, LayoutParser::EventResolver eventResolver) {
    if (node.name.empty()) {
        return nullptr;
    }

    std::unique_ptr<UIElement> element;
    if (node.name == "Panel") {
        auto panel = std::make_unique<Panel>();
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            panel->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("spacing"); it != node.attributes.end()) {
            panel->spacing = std::stof(it->second);
        }
        if (auto it = node.attributes.find("padding"); it != node.attributes.end()) {
            const auto values = SplitString(it->second, ',');
            if (values.size() == 4) {
                panel->padding.left = std::stof(values[0]);
                panel->padding.top = std::stof(values[1]);
                panel->padding.right = std::stof(values[2]);
                panel->padding.bottom = std::stof(values[3]);
            }
        }
        element = std::move(panel);
    } else if (node.name == "Label") {
        std::wstring text = L"";
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            text = LayoutParser::Utf8ToUtf16(it->second);
        }
        element = std::make_unique<Label>(std::move(text));
    } else if (node.name == "Button") {
        std::wstring text = L"Button";
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            text = LayoutParser::Utf8ToUtf16(it->second);
        }
        auto button = std::make_unique<Button>(std::move(text));
        if (auto it = node.attributes.find("onClick"); it != node.attributes.end()) {
            auto handler = eventResolver(it->second);
            if (handler) {
                button->SetOnClick(std::move(handler));
            }
        }
        element = std::move(button);
    } else if (node.name == "Image") {
        std::wstring source = L"";
        if (auto it = node.attributes.find("source"); it != node.attributes.end()) {
            source = LayoutParser::Utf8ToUtf16(it->second);
        }
        auto image = std::make_unique<Image>(std::move(source));
        if (auto it = node.attributes.find("source"); it != node.attributes.end()) {
            if (provider.Exists(it->second)) {
                image->SetImageData(provider.LoadBytes(it->second));
            }
        }
        element = std::move(image);
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
