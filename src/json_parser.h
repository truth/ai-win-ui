#pragma once

#include <string>
#include <vector>
#include <map>

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

class JsonParser {
public:
    static JsonValue Parse(const std::string& jsonText);
};
