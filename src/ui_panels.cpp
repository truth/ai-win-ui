#include "ui_panels.h"
#include <algorithm>

bool Panel::OnDrop(const std::vector<std::wstring>& files, float x, float y) {
    if (!m_allowDrop) return false;
    
    if (!m_placeholderCleared) {
        m_children.clear();
        m_placeholderCleared = true;
    }
    
    for (const auto& file : files) {
        std::wstring filename = file;
        auto pos = filename.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            filename = filename.substr(pos + 1);
        }
        
        auto chip = std::make_unique<Panel>();
        chip->direction = Direction::Row;
        chip->alignItems = AlignItems::Center;
        chip->background = ColorFromHex(0x3A4B5D, 1.0f);
        chip->cornerRadius = 6.0f;
        chip->padding = Thickness{12, 8, 12, 8};
        chip->SetMargin(Thickness{0, 0, 10, 10});
        
        auto label = std::make_unique<Label>(filename);
        label->m_fontSize = 14.0f;
        label->m_color = ColorFromHex(0xFFFFFF, 1.0f);
        
        chip->AddChild(std::move(label));
        AddChild(std::move(chip));
    }

    if (m_context && m_context->requestLayout) {
        m_context->requestLayout();
    }
    return true;
}
