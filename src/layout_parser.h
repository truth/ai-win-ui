#pragma once

#include "ui_context.h"
#include "ui.h"

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    bool IsNull() const { return type == Type::Null; }
    bool IsBool() const { return type == Type::Bool; }
    bool IsNumber() const { return type == Type::Number; }
    bool IsString() const { return type == Type::String; }
    bool IsArray() const { return type == Type::Array; }
    bool IsObject() const { return type == Type::Object; }

    const JsonValue& operator[](const std::string& key) const;
    const JsonValue& operator[](size_t index) const;
    bool HasKey(const std::string& key) const;
};

struct XmlNode {
    std::string name;
    std::map<std::string, std::string> attributes;
    std::vector<XmlNode> children;
};

class LayoutParser {
public:
    static std::unique_ptr<UIElement> BuildFromFile(UIContext& context,
                                                    const std::string& resourcePath);

    static Color ColorFromString(const std::string& value);
    static std::wstring Utf8ToUtf16(const std::string& utf8);

private:
    static std::unique_ptr<UIElement> BuildFromJson(const JsonValue& node,
                                                    IResourceProvider& provider,
                                                    UIEventResolver eventResolver);
    static std::unique_ptr<UIElement> BuildFromXml(const XmlNode& node,
                                                   IResourceProvider& provider,
                                                   UIEventResolver eventResolver);

    static JsonValue ParseJson(const std::string& text);
    static XmlNode ParseXml(const std::string& text);
};
