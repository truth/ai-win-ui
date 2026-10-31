#pragma once

#include "theme.h"
#include "ui_context.h"
#include "ui.h"
#include "json_parser.h"
#include "xml_parser.h"

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

class LayoutParser {
public:
    static std::unique_ptr<UIElement> BuildFromFile(UIContext& context,
                                                    const std::string& resourcePath);

    // Build a UI tree from an in-memory layout string (no file IO). Used by
    // embedders that generate layouts at runtime. Pass the raw XML (or JSON when
    // isJson=true). Requires context.resourceProvider to be set.
    static std::unique_ptr<UIElement> BuildFromXmlText(UIContext& context,
                                                       const std::string& xmlText,
                                                       bool isJson = false);

    static Color ColorFromString(const std::string& value);
    static std::wstring Utf8ToUtf16(const std::string& utf8);

    static JsonValue ParseJson(const std::string& text);

    // Public entry for style catalog / external tools (same rules as layout style blocks).
    static ComponentStyle ParseComponentStylePublic(const JsonValue& value);

    // Temporarily set the active StyleCatalog used by $style / extend while parsing.
    // Returns the previous catalog pointer.
    static const StyleCatalog* SetActiveStyleCatalog(const StyleCatalog* catalog);

    // Active Theme for $color / $spacing token resolution during catalog reload (S4).
    static const Theme* SetActiveTheme(const Theme* theme);

    // Resolve a single number JSON value, accepting either a literal number
    // (e.g. 12) or a "$category.key" string token (e.g. "$spacing.lg").
    // Pass the natural category of the field; unknown tokens return fallback + warning.
    static float ParseNumberValue(const JsonValue& value,
                                  Theme::NumberCategory category,
                                  float fallback = 0.0f);

private:
    static std::unique_ptr<UIElement> BuildFromJson(const JsonValue& node,
                                                    IResourceProvider& provider,
                                                    UIEventResolver eventResolver);
    static std::unique_ptr<UIElement> BuildFromXml(const XmlNode& node,
                                                   IResourceProvider& provider,
                                                   UIEventResolver eventResolver);

    static XmlNode ParseXml(const std::string& text);

    // Sub-parser module helpers (layout_parser_panels.cpp & layout_parser_controls.cpp)
    static std::unique_ptr<UIElement> CreatePanelElementFromXml(const XmlNode& node, IResourceProvider& provider, UIEventResolver eventResolver);
    static std::unique_ptr<UIElement> CreateControlElementFromXml(const XmlNode& node, IResourceProvider& provider, UIEventResolver eventResolver);
    static std::unique_ptr<UIElement> CreatePanelElementFromJson(const JsonValue& node, IResourceProvider& provider, UIEventResolver eventResolver);
    static std::unique_ptr<UIElement> CreateControlElementFromJson(const JsonValue& node, IResourceProvider& provider, UIEventResolver eventResolver);
};
