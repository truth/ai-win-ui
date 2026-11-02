#include "keyboard_genie.h"
#include <cctype>

KeyboardGenie::KeyboardGenie() {
    InitDatabase();
}

void KeyboardGenie::InitDatabase() {
    m_database = {
        { L"1A0001", L"上证指数", "SZZS", L"上证" },
        { L"399001", L"深证成指", "SZCZ", L"深证" },
        { L"399006", L"创业板指", "CYBZ", L"创业" },
        { L"000688", L"科创50",   "KC50", L"科创" },
        { L"600519", L"贵州茅台", "GZMT", L"沪A" },
        { L"600036", L"招商银行", "ZSYH", L"沪A" },
        { L"601166", L"兴业银行", "XYYH", L"沪A" },
        { L"600196", L"复星医药", "FXYY", L"沪A" },
        { L"600019", L"宝钢股份", "BGGF", L"沪A" },
        { L"603833", L"欧派家居", "OPJJ", L"沪A" },
        { L"601088", L"中国神华", "ZGSH", L"沪A" },
        { L"603259", L"药明康德", "YMKD", L"沪A" },
        { L"601318", L"中国平安", "ZGPA", L"沪A" },
        { L"600809", L"山西汾酒", "SXFJ", L"沪A" },
        { L"600703", L"三安光电", "SAGD", L"沪A" },
        { L"600919", L"江苏银行", "JSYH", L"沪A" },
        { L"600000", L"浦发银行", "PFYH", L"沪A" },
        { L"600585", L"海螺水泥", "HLSN", L"沪A" },
        { L"000001", L"平安银行", "PAYH", L"深A" },
        { L"002594", L"比亚迪",   "BYD",  L"深A" },
        { L"300750", L"宁德时代", "NDSD", L"创业" },
    };
}

void KeyboardGenie::AppendChar(char c) {
    if (std::isalnum(static_cast<unsigned char>(c))) {
        currentQuery.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        isVisible = true;
        selectedIndex = 0;
    }
}

void KeyboardGenie::Backspace() {
    if (!currentQuery.empty()) {
        currentQuery.pop_back();
        if (currentQuery.empty()) {
            Hide();
        }
    }
}

std::vector<StockSymbol> KeyboardGenie::Query(const std::string& input, size_t maxResults) const {
    if (input.empty()) return {};

    std::string upperInput;
    for (char c : input) {
        upperInput.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }

    std::vector<StockSymbol> results;
    for (const auto& item : m_database) {
        std::string codeStr;
        for (wchar_t ch : item.code) {
            codeStr.push_back(static_cast<char>(ch & 0x7F));
        }

        if (codeStr.find(upperInput) != std::string::npos ||
            item.pinyin.find(upperInput) != std::string::npos) {
            results.push_back(item);
            if (results.size() >= maxResults) break;
        }
    }
    return results;
}
