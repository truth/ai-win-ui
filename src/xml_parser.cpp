#include "xml_parser.h"
#include "pugixml.hpp"

namespace {

XmlNode ConvertPugiNode(const pugi::xml_node& pNode) {
    XmlNode node;
    node.name = pNode.name();

    for (const pugi::xml_attribute& attr : pNode.attributes()) {
        node.attributes.emplace(attr.name(), attr.value());
    }

    for (const pugi::xml_node& child : pNode.children()) {
        if (child.type() == pugi::node_element) {
            node.children.push_back(ConvertPugiNode(child));
        }
    }

    return node;
}

} // namespace

XmlNode XmlParser::Parse(const std::string& xmlText) {
    if (xmlText.empty()) {
        return XmlNode{};
    }

    pugi::xml_document doc;
    // parse_default handles entity decoding, skips comments, etc.
    pugi::xml_parse_result result = doc.load_buffer(xmlText.data(), xmlText.size(), pugi::parse_default);
    if (!result) {
        return XmlNode{};
    }

    pugi::xml_node rootElement = doc.first_child();
    while (rootElement && rootElement.type() != pugi::node_element) {
        rootElement = rootElement.next_sibling();
    }

    if (!rootElement) {
        return XmlNode{};
    }

    return ConvertPugiNode(rootElement);
}
