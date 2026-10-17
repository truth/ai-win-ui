#include "layout_parser.h"
#include "ui_panels.h"
#include "ui_scroll.h"
#include "ui_overlays.h"

// Sub-parser module for Panel, Grid, ScrollViewer, ShapePanel container elements
std::unique_ptr<UIElement> LayoutParser::CreatePanelElementFromXml(const XmlNode& node,
                                                                   IResourceProvider& provider,
                                                                   UIEventResolver eventResolver) {
    if (node.name == "Panel") {
        auto panel = std::make_unique<Panel>();
        if (auto it = node.attributes.find("direction"); it != node.attributes.end()) {
            panel->direction = (it->second == "row" || it->second == "Row") ? Panel::Direction::Row : Panel::Direction::Column;
        }
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            panel->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("cornerRadius"); it != node.attributes.end()) {
            panel->cornerRadius = std::stof(it->second);
        }
        if (auto it = node.attributes.find("spacing"); it != node.attributes.end()) {
            panel->spacing = std::stof(it->second);
        }
        return panel;
    }
    if (node.name == "ScrollViewer") {
        auto scrollViewer = std::make_unique<ScrollViewer>();
        if (auto it = node.attributes.find("background"); it != node.attributes.end()) {
            scrollViewer->background = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("barThickness"); it != node.attributes.end()) {
            scrollViewer->barThickness = std::stof(it->second);
        }
        if (auto it = node.attributes.find("trackColor"); it != node.attributes.end()) {
            scrollViewer->trackColor = LayoutParser::ColorFromString(it->second);
        }
        if (auto it = node.attributes.find("thumbColor"); it != node.attributes.end()) {
            scrollViewer->thumbColor = LayoutParser::ColorFromString(it->second);
        }
        return scrollViewer;
    }
    return nullptr;
}

std::unique_ptr<UIElement> LayoutParser::CreatePanelElementFromJson(const JsonValue& node,
                                                                    IResourceProvider& provider,
                                                                    UIEventResolver eventResolver) {
    if (!node.IsObject()) return nullptr;
    const std::string type = node["type"].IsString() ? node["type"].stringValue : "";
    if (type == "Panel") {
        auto panel = std::make_unique<Panel>();
        return panel;
    }
    if (type == "ScrollViewer") {
        auto scrollViewer = std::make_unique<ScrollViewer>();
        return scrollViewer;
    }
    return nullptr;
}
