#pragma once
// RichTextBox
// See ui.h for the full umbrella include.
#include "ui_pickers.h"

class RichTextBox : public UIElement {
public:
    explicit RichTextBox(std::wstring text = L"") {
        m_style = DefaultStyle(nullptr);
        SetText(std::move(text));
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyColorsFromStyle();
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            ResetCaretBlink();
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (!m_hasFocus) {
            return false;
        }
        m_hasFocus = false;
        m_isDragging = false;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (ctrlDown && (keyCode == 'B' || keyCode == 'b')) {
            ToggleStyleOnSelectionOrTyping(/*bold=*/true, /*italic=*/false);
            return true;
        }
        if (ctrlDown && (keyCode == 'I' || keyCode == 'i')) {
            ToggleStyleOnSelectionOrTyping(/*bold=*/false, /*italic=*/true);
            return true;
        }
        if (ctrlDown && (keyCode == 'A' || keyCode == 'a')) {
            m_selectionStart = 0;
            m_selectionEnd = m_text.size();
            m_caretPosition = m_text.size();
            ResetCaretBlink();
            return true;
        }

        switch (keyCode) {
            case VK_RETURN:
                InsertText(L"\n");
                return true;
            case VK_BACK:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition > 0) {
                    EraseRange(m_caretPosition - 1, 1);
                    --m_caretPosition;
                    ClearSelection();
                    ResetCaretBlink();
                    return true;
                }
                return false;
            case VK_DELETE:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition < m_text.size()) {
                    EraseRange(m_caretPosition, 1);
                    ResetCaretBlink();
                    return true;
                }
                return false;
            case VK_LEFT:
                if (m_caretPosition > 0) {
                    if (shiftDown && !HasSelection()) {
                        m_selectionStart = m_caretPosition;
                    }
                    --m_caretPosition;
                    if (shiftDown) {
                        m_selectionEnd = m_caretPosition;
                    } else {
                        ClearSelection();
                    }
                    ResetCaretBlink();
                    return true;
                }
                return false;
            case VK_RIGHT:
                if (m_caretPosition < m_text.size()) {
                    if (shiftDown && !HasSelection()) {
                        m_selectionStart = m_caretPosition;
                    }
                    ++m_caretPosition;
                    if (shiftDown) {
                        m_selectionEnd = m_caretPosition;
                    } else {
                        ClearSelection();
                    }
                    ResetCaretBlink();
                    return true;
                }
                return false;
            case VK_UP:
                return MoveCaretByLine(-1, shiftDown);
            case VK_DOWN:
                return MoveCaretByLine(1, shiftDown);
            case VK_HOME:
                m_caretPosition = LineStart(m_caretPosition);
                if (shiftDown) {
                    if (!HasSelection()) {
                        m_selectionStart = m_selectionEnd;
                    }
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            case VK_END:
                m_caretPosition = LineEnd(m_caretPosition);
                if (shiftDown) {
                    if (!HasSelection()) {
                        m_selectionStart = m_selectionEnd;
                    }
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            default:
                return false;
        }
    }

    bool OnChar(wchar_t ch) override {
        if (ch == L'\b' || ch == L'\r' || ch == L'\n') {
            return true;
        }
        if (ch >= L' ' && ch != 0x7F) {
            InsertText(std::wstring(1, ch));
            return true;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const size_t index = IndexFromPoint(x, y);
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shiftDown) {
            if (!HasSelection()) {
                m_selectionStart = m_caretPosition;
            }
            m_caretPosition = index;
            m_selectionEnd = m_caretPosition;
        } else {
            m_caretPosition = index;
            ClearSelection();
        }
        m_dragAnchor = m_caretPosition;
        m_isDragging = true;
        ResetCaretBlink();
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (!m_isDragging) {
            return false;
        }
        const size_t index = IndexFromPoint(x, y);
        if (index == m_caretPosition) {
            return false;
        }
        m_caretPosition = index;
        m_selectionStart = m_dragAnchor;
        m_selectionEnd = m_caretPosition;
        ResetCaretBlink();
        return true;
    }

    bool OnMouseUp(float /*x*/, float /*y*/) override {
        if (!m_isDragging) {
            return false;
        }
        m_isDragging = false;
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_isDragging) {
            return false;
        }
        m_isDragging = false;
        return true;
    }

    bool OnTimer(UINT_PTR /*timerId*/) override {
        if (!m_hasFocus) {
            return false;
        }
        const DWORD now = GetTickCount();
        if (now - m_lastBlink >= 500) {
            m_showCaret = !m_showCaret;
            m_lastBlink = now;
            return true;
        }
        return false;
    }

    bool GetTextInputCaretRect(Rect& out) const override {
        if (!m_hasFocus) {
            return false;
        }
        out = CaretRect();
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(420.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(std::max(80.0f, preferredHeight));
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCorner = ScaleValue(cornerRadius);
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledR = scaledCorner;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledR = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            textColor = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            fontSize = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledR);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledR);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledR);
        }

        const Rect content = ContentRect();
        renderer.PushRoundedClip(content, ScaleValue(2.0f));

        if (HasSelection()) {
            DrawSelection(renderer, content);
        }

        const auto lines = BuildLines();
        float y = content.top - m_scrollY;
        for (const LineInfo& line : lines) {
            if (y + LineHeight() < content.top) {
                y += LineHeight();
                continue;
            }
            if (y > content.bottom) {
                break;
            }
            DrawStyledLine(renderer, line, content.left, y, content.right);
            y += LineHeight();
        }

        if (m_hasFocus && m_showCaret) {
            const Rect caret = CaretRect();
            if (caret.bottom >= content.top && caret.top <= content.bottom) {
                renderer.FillRect(caret, textColor);
            }
        }

        renderer.PopLayer();
    }

    void SetText(std::wstring text) {
        m_text.clear();
        m_bold.clear();
        m_italic.clear();
        ParseMarkupInto(UnescapeLayoutText(std::move(text)));
        m_caretPosition = m_text.size();
        ClearSelection();
        m_scrollY = 0.0f;
    }

    void SetPlainText(std::wstring text) {
        m_text = UnescapeLayoutText(std::move(text));
        m_bold.assign(m_text.size(), false);
        m_italic.assign(m_text.size(), false);
        m_caretPosition = std::min(m_caretPosition, m_text.size());
        ClearSelection();
    }

    const std::wstring& Text() const { return m_text; }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color textColor = ColorFromHex(0xEAF3FF);
    Color selectionColor = ColorFromHex(0x3A86FF);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;
    float preferredHeight = 120.0f;

private:
    void SyncLegacyColorsFromStyle() {
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            borderColor = m_style.base.decoration->border.color;
            if (!m_style.base.decoration->radius.IsZero()) {
                cornerRadius = m_style.base.decoration->radius.MaxRadius();
            }
        }
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
        }
        if (m_style.base.fontSize.has_value()) {
            fontSize = *m_style.base.fontSize;
        }
    }

    struct LineInfo {
        size_t start = 0;
        size_t end = 0; // exclusive
    };

    static std::wstring UnescapeLayoutText(std::wstring text) {
        // XML attributes often carry literal "\n"; JSON already expands escapes.
        std::wstring out;
        out.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == L'\\' && i + 1 < text.size()) {
                const wchar_t next = text[i + 1];
                if (next == L'n') {
                    out.push_back(L'\n');
                    ++i;
                    continue;
                }
                if (next == L't') {
                    out.push_back(L'\t');
                    ++i;
                    continue;
                }
                if (next == L'\\') {
                    out.push_back(L'\\');
                    ++i;
                    continue;
                }
            }
            out.push_back(text[i]);
        }
        return out;
    }

    void ParseMarkupInto(const std::wstring& source) {
        bool bold = false;
        bool italic = false;
        for (size_t i = 0; i < source.size();) {
            if (i + 1 < source.size() && source[i] == L'*' && source[i + 1] == L'*') {
                bold = !bold;
                i += 2;
                continue;
            }
            if (source[i] == L'*') {
                italic = !italic;
                ++i;
                continue;
            }
            m_text.push_back(source[i]);
            m_bold.push_back(bold);
            m_italic.push_back(italic);
            ++i;
        }
    }

    float LineHeight() const {
        return ScaleValue(fontSize * 1.35f);
    }

    Rect ContentRect() const {
        const float inset = ScaleValue(10.0f);
        return Rect::Make(
            m_bounds.left + inset,
            m_bounds.top + inset,
            std::max(m_bounds.left + inset, m_bounds.right - inset),
            std::max(m_bounds.top + inset, m_bounds.bottom - inset));
    }

    std::vector<LineInfo> BuildLines() const {
        std::vector<LineInfo> lines;
        size_t start = 0;
        for (size_t i = 0; i <= m_text.size(); ++i) {
            if (i == m_text.size() || m_text[i] == L'\n') {
                lines.push_back(LineInfo{start, i});
                start = i + 1;
            }
        }
        if (lines.empty()) {
            lines.push_back(LineInfo{0, 0});
        }
        return lines;
    }

    size_t LineStart(size_t index) const {
        if (m_text.empty()) {
            return 0;
        }
        index = std::min(index, m_text.size());
        while (index > 0 && m_text[index - 1] != L'\n') {
            --index;
        }
        return index;
    }

    size_t LineEnd(size_t index) const {
        index = std::min(index, m_text.size());
        while (index < m_text.size() && m_text[index] != L'\n') {
            ++index;
        }
        return index;
    }

    int LineIndexOf(size_t caret) const {
        const auto lines = BuildLines();
        for (size_t i = 0; i < lines.size(); ++i) {
            if (caret >= lines[i].start && caret <= lines[i].end) {
                // caret == end is on this line unless it's a newline boundary of a later line
                if (caret == lines[i].end && i + 1 < lines.size() && caret < m_text.size() && m_text[caret] == L'\n') {
                    // caret at the newline belongs to this line's end
                }
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(lines.size()) - 1;
    }

    bool MoveCaretByLine(int delta, bool shiftDown) {
        const auto lines = BuildLines();
        if (lines.empty()) {
            return false;
        }
        int lineIndex = LineIndexOf(m_caretPosition);
        const size_t col = m_caretPosition - lines[static_cast<size_t>(lineIndex)].start;
        const int nextLine = std::clamp(lineIndex + delta, 0, static_cast<int>(lines.size()) - 1);
        if (nextLine == lineIndex) {
            return false;
        }
        const LineInfo& target = lines[static_cast<size_t>(nextLine)];
        const size_t lineLen = target.end - target.start;
        const size_t newCaret = target.start + std::min(col, lineLen);

        if (shiftDown && !HasSelection()) {
            m_selectionStart = m_caretPosition;
        }
        m_caretPosition = newCaret;
        if (shiftDown) {
            m_selectionEnd = m_caretPosition;
        } else {
            ClearSelection();
        }
        EnsureCaretVisible();
        ResetCaretBlink();
        return true;
    }

    float MeasureStyledWidth(size_t start, size_t end) const {
        if (start >= end || start >= m_text.size()) {
            return 0.0f;
        }
        end = std::min(end, m_text.size());
        float width = 0.0f;
        size_t i = start;
        while (i < end) {
            const bool bold = StyleAt(i, true);
            const bool italic = StyleAt(i, false);
            size_t j = i + 1;
            while (j < end && StyleAt(j, true) == bold && StyleAt(j, false) == italic) {
                ++j;
            }
            const std::wstring chunk = m_text.substr(i, j - i);
            width += MeasureTextValue(chunk, fontSize, 10000.0f, TextWrapMode::NoWrap).width;
            i = j;
        }
        return width;
    }

    bool StyleAt(size_t index, bool wantBold) const {
        if (index >= m_text.size()) {
            return wantBold ? m_typingBold : m_typingItalic;
        }
        return wantBold ? (index < m_bold.size() && m_bold[index])
                        : (index < m_italic.size() && m_italic[index]);
    }

    void DrawStyledLine(IRenderer& renderer, const LineInfo& line, float left, float top, float right) const {
        float x = left;
        size_t i = line.start;
        while (i < line.end) {
            const bool bold = StyleAt(i, true);
            const bool italic = StyleAt(i, false);
            size_t j = i + 1;
            while (j < line.end && StyleAt(j, true) == bold && StyleAt(j, false) == italic) {
                ++j;
            }
            const std::wstring chunk = m_text.substr(i, j - i);
            const float chunkWidth = MeasureTextValue(chunk, fontSize, 10000.0f, TextWrapMode::NoWrap).width;
            const Rect rect = Rect::Make(x, top, std::min(x + chunkWidth + 1.0f, right), top + LineHeight());
            TextRenderOptions options{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                bold,
                italic
            };
            renderer.DrawTextW(
                chunk.c_str(),
                static_cast<UINT32>(chunk.size()),
                rect,
                textColor,
                ScaleValue(fontSize),
                options);
            x += chunkWidth;
            i = j;
            if (x >= right) {
                break;
            }
        }
    }

    void DrawSelection(IRenderer& renderer, const Rect& content) const {
        const auto [selStart, selEnd] = GetSelectionRange();
        if (selStart == selEnd) {
            return;
        }
        const auto lines = BuildLines();
        float y = content.top - m_scrollY;
        for (const LineInfo& line : lines) {
            if (selEnd <= line.start || selStart > line.end) {
                y += LineHeight();
                continue;
            }
            const size_t a = std::max(selStart, line.start);
            const size_t b = std::min(selEnd, line.end);
            float x0 = content.left + MeasureStyledWidth(line.start, a);
            float x1 = content.left + MeasureStyledWidth(line.start, b);
            if (selEnd > line.end) {
                // selection continues past this line - paint a small newline gutter
                x1 = std::max(x1, x0 + ScaleValue(6.0f));
            }
            const Rect sel = Rect::Make(x0, y, std::max(x0 + 2.0f, x1), y + LineHeight());
            if (sel.bottom >= content.top && sel.top <= content.bottom) {
                renderer.FillRect(sel, selectionColor);
            }
            y += LineHeight();
        }
    }

    size_t IndexFromPoint(float x, float y) const {
        const Rect content = ContentRect();
        const auto lines = BuildLines();
        float relY = y - content.top + m_scrollY;
        int lineIndex = static_cast<int>(relY / LineHeight());
        lineIndex = std::clamp(lineIndex, 0, static_cast<int>(lines.size()) - 1);
        const LineInfo& line = lines[static_cast<size_t>(lineIndex)];
        const float localX = x - content.left;

        size_t best = line.start;
        for (size_t i = line.start; i <= line.end; ++i) {
            const float width = MeasureStyledWidth(line.start, i);
            best = i;
            if (localX < width + ScaleValue(3.0f)) {
                break;
            }
        }
        return best;
    }

    Rect CaretRect() const {
        const Rect content = ContentRect();
        const auto lines = BuildLines();
        const int lineIndex = std::max(0, LineIndexOf(m_caretPosition));
        const LineInfo& line = lines[static_cast<size_t>(lineIndex)];
        const float x = content.left + MeasureStyledWidth(line.start, std::min(m_caretPosition, line.end));
        const float y = content.top - m_scrollY + static_cast<float>(lineIndex) * LineHeight();
        return Rect::Make(x, y, x + ScaleValue(1.5f), y + LineHeight());
    }

    void EnsureCaretVisible() {
        const Rect content = ContentRect();
        const int lineIndex = std::max(0, LineIndexOf(m_caretPosition));
        const float caretTop = static_cast<float>(lineIndex) * LineHeight();
        const float caretBottom = caretTop + LineHeight();
        if (caretTop < m_scrollY) {
            m_scrollY = caretTop;
        } else if (caretBottom > m_scrollY + content.Height()) {
            m_scrollY = caretBottom - content.Height();
        }
        m_scrollY = std::max(0.0f, m_scrollY);
    }

    bool HasSelection() const {
        return m_selectionStart != m_selectionEnd;
    }

    std::pair<size_t, size_t> GetSelectionRange() const {
        return m_selectionStart <= m_selectionEnd
            ? std::make_pair(m_selectionStart, m_selectionEnd)
            : std::make_pair(m_selectionEnd, m_selectionStart);
    }

    void ClearSelection() {
        m_selectionStart = m_selectionEnd = m_caretPosition;
    }

    void EraseRange(size_t start, size_t count) {
        if (count == 0 || start >= m_text.size()) {
            return;
        }
        count = std::min(count, m_text.size() - start);
        m_text.erase(start, count);
        if (start < m_bold.size()) {
            m_bold.erase(m_bold.begin() + static_cast<std::ptrdiff_t>(start),
                         m_bold.begin() + static_cast<std::ptrdiff_t>(start + count));
        }
        if (start < m_italic.size()) {
            m_italic.erase(m_italic.begin() + static_cast<std::ptrdiff_t>(start),
                           m_italic.begin() + static_cast<std::ptrdiff_t>(start + count));
        }
        SyncStyleVectors();
    }

    void DeleteSelection() {
        if (!HasSelection()) {
            return;
        }
        const auto [start, end] = GetSelectionRange();
        EraseRange(start, end - start);
        m_caretPosition = start;
        ClearSelection();
    }

    void InsertText(const std::wstring& text) {
        if (text.empty()) {
            return;
        }
        if (HasSelection()) {
            DeleteSelection();
        }
        SyncStyleVectors();
        m_text.insert(m_caretPosition, text);
        m_bold.insert(m_bold.begin() + static_cast<std::ptrdiff_t>(m_caretPosition), text.size(), m_typingBold);
        m_italic.insert(m_italic.begin() + static_cast<std::ptrdiff_t>(m_caretPosition), text.size(), m_typingItalic);
        m_caretPosition += text.size();
        ClearSelection();
        EnsureCaretVisible();
        ResetCaretBlink();
    }

    void SyncStyleVectors() {
        if (m_bold.size() < m_text.size()) {
            m_bold.resize(m_text.size(), false);
        } else if (m_bold.size() > m_text.size()) {
            m_bold.resize(m_text.size());
        }
        if (m_italic.size() < m_text.size()) {
            m_italic.resize(m_text.size(), false);
        } else if (m_italic.size() > m_text.size()) {
            m_italic.resize(m_text.size());
        }
    }

    void ToggleStyleOnSelectionOrTyping(bool toggleBold, bool toggleItalic) {
        if (HasSelection()) {
            const auto [start, end] = GetSelectionRange();
            SyncStyleVectors();
            for (size_t i = start; i < end && i < m_text.size(); ++i) {
                if (toggleBold) {
                    m_bold[i] = !m_bold[i];
                }
                if (toggleItalic) {
                    m_italic[i] = !m_italic[i];
                }
            }
            // Update typing style from first selected char
            if (start < m_text.size()) {
                m_typingBold = m_bold[start];
                m_typingItalic = m_italic[start];
            }
        } else {
            if (toggleBold) {
                m_typingBold = !m_typingBold;
            }
            if (toggleItalic) {
                m_typingItalic = !m_typingItalic;
            }
        }
        ResetCaretBlink();
    }

    void ResetCaretBlink() {
        m_showCaret = true;
        m_lastBlink = GetTickCount();
    }

    std::wstring m_text;
    std::vector<bool> m_bold;
    std::vector<bool> m_italic;
    size_t m_caretPosition = 0;
    size_t m_selectionStart = 0;
    size_t m_selectionEnd = 0;
    size_t m_dragAnchor = 0;
    bool m_isDragging = false;
    bool m_showCaret = true;
    bool m_typingBold = false;
    bool m_typingItalic = false;
    DWORD m_lastBlink = 0;
    float m_scrollY = 0.0f;
};

