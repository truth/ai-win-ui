#include "json_parser.h"
#include "yyjson.h"

namespace {

static const JsonValue g_nullJsonValue;

JsonValue ConvertYyjsonVal(yyjson_val* val) {
    JsonValue out;
    if (!val) {
        return out;
    }

    switch (yyjson_get_type(val)) {
    case YYJSON_TYPE_NULL:
        out.type = JsonValue::Type::Null;
        break;
    case YYJSON_TYPE_BOOL:
        out.type = JsonValue::Type::Bool;
        out.boolValue = yyjson_get_bool(val);
        break;
    case YYJSON_TYPE_NUM:
        out.type = JsonValue::Type::Number;
        out.numberValue = yyjson_get_num(val);
        break;
    case YYJSON_TYPE_STR:
        out.type = JsonValue::Type::String;
        out.stringValue = std::string(yyjson_get_str(val), yyjson_get_len(val));
        break;
    case YYJSON_TYPE_ARR: {
        out.type = JsonValue::Type::Array;
        size_t idx, max;
        yyjson_val* item;
        yyjson_arr_foreach(val, idx, max, item) {
            out.arrayValue.push_back(ConvertYyjsonVal(item));
        }
        break;
    }
    case YYJSON_TYPE_OBJ: {
        out.type = JsonValue::Type::Object;
        size_t idx, max;
        yyjson_val* key;
        yyjson_val* item;
        yyjson_obj_foreach(val, idx, max, key, item) {
            std::string k(yyjson_get_str(key), yyjson_get_len(key));
            out.objectValue.emplace(std::move(k), ConvertYyjsonVal(item));
        }
        break;
    }
    default:
        break;
    }
    return out;
}

} // namespace

const JsonValue& JsonValue::operator[](const std::string& key) const {
    auto it = objectValue.find(key);
    if (it != objectValue.end()) {
        return it->second;
    }
    return g_nullJsonValue;
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (index < arrayValue.size()) {
        return arrayValue[index];
    }
    return g_nullJsonValue;
}

bool JsonValue::HasKey(const std::string& key) const {
    return objectValue.find(key) != objectValue.end();
}

JsonValue JsonParser::Parse(const std::string& jsonText) {
    if (jsonText.empty()) {
        return JsonValue{};
    }

    yyjson_read_err err;
    yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(jsonText.data()), jsonText.size(), 0, nullptr, &err);
    if (!doc) {
        return JsonValue{};
    }

    yyjson_val* root = yyjson_doc_get_root(doc);
    JsonValue result = ConvertYyjsonVal(root);
    yyjson_doc_free(doc);
    return result;
}
