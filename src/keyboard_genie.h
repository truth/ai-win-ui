#pragma once
// keyboard_genie.h - Instant stock search & quick command popup
#include <string>
#include <vector>
#include <algorithm>

struct StockSymbol {
    std::wstring code;
    std::wstring name;
    std::string pinyin;
    std::wstring market;
};

class KeyboardGenie {
public:
    KeyboardGenie();

    // 过滤匹配证券，返回最多 maxResults 条
    std::vector<StockSymbol> Query(const std::string& input, size_t maxResults = 8) const;

    // 当前选中的索引
    int selectedIndex = 0;
    bool isVisible = false;
    std::string currentQuery;

    void Show() { isVisible = true; selectedIndex = 0; }
    void Hide() { isVisible = false; currentQuery.clear(); selectedIndex = 0; }
    void AppendChar(char c);
    void Backspace();

private:
    void InitDatabase();
    std::vector<StockSymbol> m_database;
};
