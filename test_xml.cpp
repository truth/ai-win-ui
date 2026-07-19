#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main() {
    std::ifstream file("resource/layouts/drag_drop_cases.xml", std::ios::binary);
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string text = oss.str();
    
    std::cout << "Read " << text.size() << " bytes.\n";
    if (text.size() > 0) std::cout << "First char: " << text[0] << "\n";
    return 0;
}
