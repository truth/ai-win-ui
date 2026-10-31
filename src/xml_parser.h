#pragma once

#include <string>
#include <vector>
#include <map>

struct XmlNode {
    std::string name;
    std::map<std::string, std::string> attributes;
    std::vector<XmlNode> children;
};

class XmlParser {
public:
    static XmlNode Parse(const std::string& xmlText);
};
