#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream file("../resource/layouts/drag_drop_cases.xml", std::ios::binary);
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string text = oss.str();
    
    std::cout << "Read " << text.size() << " bytes.\n";
    if (text.size() > 0) std::cout << "First char: " << text[0] << "\n";
    
    // Simulate what ParseXml does:
    size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && isspace(text[pos])) pos++;
        if (pos >= text.size() || text[pos] != '<') {
            pos++;
            continue;
        }
        std::cout << "Found < at " << pos << "\n";
        pos++;
        size_t startName = pos;
        while (pos < text.size() && !isspace(text[pos]) && text[pos] != '/' && text[pos] != '>') pos++;
        std::string name = text.substr(startName, pos - startName);
        std::cout << "Name: " << name << "\n";
        break;
    }
    return 0;
}
