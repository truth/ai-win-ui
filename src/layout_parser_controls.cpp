#include "layout_parser.h"
#include "ui_inputs.h"
#include "ui_text.h"
#include "ui_lists.h"

// Sub-parser module for Control elements (Label, Button, TextInput, ListBox, etc.)
std::unique_ptr<UIElement> LayoutParser::CreateControlElementFromXml(const XmlNode& node,
                                                                     IResourceProvider& provider,
                                                                     UIEventResolver eventResolver) {
    if (node.name == "Label") {
        std::wstring text = L"Label";
        if (auto it = node.attributes.find("text"); it != node.attributes.end()) {
            text = LayoutParser::Utf8ToUtf16(it->second);
        }
        auto label = std::make_unique<Label>(std::move(text));
        if (auto it = node.attributes.find("fontSize"); it != node.attributes.end()) {
            label->m_fontSize = std::stof(it->second);
        }
        if (auto it = node.attributes.find("color"); it != node.attributes.end()) {
            label->m_color = LayoutParser::ColorFromString(it->second);
        }
        return label;
    }
    return nullptr;
}

std::unique_ptr<UIElement> LayoutParser::CreateControlElementFromJson(const JsonValue& node,
                                                                      IResourceProvider& provider,
                                                                      UIEventResolver eventResolver) {
    if (!node.IsObject()) return nullptr;
    const std::string type = node["type"].IsString() ? node["type"].stringValue : "";
    if (type == "Label") {
        std::wstring text = node["text"].IsString() ? LayoutParser::Utf8ToUtf16(node["text"].stringValue) : L"Label";
        auto label = std::make_unique<Label>(std::move(text));
        return label;
    }
    return nullptr;
}
