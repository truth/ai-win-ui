#pragma once
// ListView, TreeView
// See ui.h for the full umbrella include.
#include "ui_scroll.h"

class ListView : public UIElement {
public:
    struct Column {
        std::wstring title;
        float width = -1.0f;
    };

    ListView() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x101C29));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x35506A));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xD0DDEC));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        auto item = std::make_unique<ComponentStyle>();
        item->base.decoration = BoxDecoration{};
        item->base.decoration->background = Color{0, 0, 0, 0};
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xD0DDEC));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x203B56));
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2D567C));
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].decoration = selected;
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].foreground =
            ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));
        s.itemStyle = std::move(item);
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
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            m_hoveredRow = -1;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_rows.empty()) {
            return false;
        }
        switch (keyCode) {
            case VK_UP:
                return SetSelectedIndex(m_selectedRow - 1);
            case VK_DOWN:
                return SetSelectedIndex(m_selectedRow + 1);
            case VK_HOME:
                return SetSelectedIndex(0);
            case VK_END:
                return SetSelectedIndex(static_cast<int>(m_rows.size()) - 1);
            case VK_PRIOR:
                return SetSelectedIndex(m_selectedRow - VisibleRowCount());
            case VK_NEXT:
                return SetSelectedIndex(m_selectedRow + VisibleRowCount());
            case VK_LEFT:
                return ScrollHorizontal(-ScaleValue(40.0f));
            case VK_RIGHT:
                return ScrollHorizontal(ScaleValue(40.0f));
            default:
                return false;
        }
    }

    bool OnMouseWheel(float delta, float x, float y, bool shiftHeld) override {
        if (!HitTest(x, y)) {
            return false;
        }
        if (shiftHeld) {
            return ScrollHorizontal(-delta * ScaleValue(48.0f));
        }
        if (m_rows.empty()) {
            return false;
        }
        const int before = m_scrollOffset;
        m_scrollOffset = std::clamp(
            m_scrollOffset - static_cast<int>(std::lround(delta * 3.0f)),
            0,
            std::max(0, static_cast<int>(m_rows.size()) - VisibleRowCount()));
        return m_scrollOffset != before;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const int rowIndex = RowIndexFromPoint(y);
        if (rowIndex >= 0) {
            SetSelectedIndex(rowIndex);
            return true;
        }
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (!HitTest(x, y)) {
            if (m_hoveredRow == -1) {
                return false;
            }
            m_hoveredRow = -1;
            return true;
        }
        const int nextHovered = RowIndexFromPoint(y);
        if (nextHovered == m_hoveredRow) {
            return false;
        }
        m_hoveredRow = nextHovered;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hoveredRow == -1) {
            return false;
        }
        m_hoveredRow = -1;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        if (m_columns.empty()) {
            return std::min(availableWidth, ScaleValue(420.0f));
        }

        const Thickness scaledCellPadding = ScaleThickness(cellPadding);
        float total = 0.0f;
        for (size_t col = 0; col < m_columns.size(); ++col) {
            if (m_columns[col].width > 0.0f) {
                total += ScaleValue(m_columns[col].width);
                continue;
            }

            float width = 0.0f;
            const Size header = MeasureTextValue(
                m_columns[col].title.empty() ? L"Column" : m_columns[col].title,
                headerFontSize,
                4096.0f,
                TextWrapMode::NoWrap);
            width = std::max(width, header.width);

            const size_t inspectRows = std::min<size_t>(m_rows.size(), 30);
            for (size_t row = 0; row < inspectRows; ++row) {
                if (col >= m_rows[row].size()) {
                    continue;
                }
                const Size cell = MeasureTextValue(
                    m_rows[row][col],
                    fontSize,
                    4096.0f,
                    TextWrapMode::NoWrap);
                width = std::max(width, cell.width);
            }
            total += width + scaledCellPadding.left + scaledCellPadding.right + ScaleValue(10.0f);
        }
        return std::min(availableWidth, total + ScaleValue(2.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const float header = ScaleValue(headerHeight);
        const float row = ScaleValue(rowHeight);
        // Prefer a compact default so validation pages fit without huge empty body.
        const int rows = std::clamp(static_cast<int>(m_rows.size()), 3, 6);
        return header + row * static_cast<float>(rows) + ScaleValue(2.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCornerRadius = ScaleValue(cornerRadius);
        Color shellFg = textColor;
        float shellFs = fontSize;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCornerRadius = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            shellFg = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            shellFs = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        if (m_columns.empty()) {
            const std::wstring message = L"No columns";
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                message.c_str(),
                static_cast<UINT32>(message.size()),
                m_bounds,
                shellFg,
                ScaleValue(shellFs),
                textOptions);
            return;
        }

        const std::vector<float> widths = ResolveColumnWidths();
        if (widths.empty()) {
            return;
        }

        const Rect headerRect = HeaderRect();
        const Rect bodyRect = BodyRect();
        const float scaledHeaderHeight = ScaleValue(headerHeight);
        const float scaledRowHeight = std::max(1.0f, ScaleValue(rowHeight));
        const Thickness scaledCellPadding = ScaleThickness(cellPadding);
        const ComponentStyle* itemStyle = m_style.itemStyle ? m_style.itemStyle.get() : nullptr;

        float contentW = 0.0f;
        for (float w : widths) {
            contentW += w;
        }
        const float contentH = scaledRowHeight * static_cast<float>(std::max<size_t>(1, m_rows.size()));
        const float viewW = std::max(1.0f, m_bounds.Width());
        const float viewH = std::max(1.0f, bodyRect.Height());
        ClampHorizontalScroll(contentW, viewW);

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);
        renderer.FillRect(headerRect, headerBackground);
        renderer.DrawLine(
            PointF{m_bounds.left, headerRect.bottom},
            PointF{m_bounds.right, headerRect.bottom},
            gridLineColor,
            1.0f);

        float x = m_bounds.left - m_hScroll;
        for (size_t col = 0; col < m_columns.size(); ++col) {
            const Rect cellRect = Rect::Make(
                x + scaledCellPadding.left,
                headerRect.top + scaledCellPadding.top,
                x + widths[col] - scaledCellPadding.right,
                headerRect.bottom - scaledCellPadding.bottom);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                m_columns[col].title.c_str(),
                static_cast<UINT32>(m_columns[col].title.size()),
                cellRect,
                headerTextColor,
                ScaleValue(headerFontSize),
                textOptions);

            if (col + 1 < m_columns.size()) {
                renderer.DrawLine(
                    PointF{x + widths[col], m_bounds.top},
                    PointF{x + widths[col], m_bounds.bottom},
                    gridLineColor,
                    1.0f);
            }
            x += widths[col];
        }

        const int visibleRows = std::max(1, static_cast<int>(std::floor(bodyRect.Height() / scaledRowHeight)));
        const int maxStart = std::max(0, static_cast<int>(m_rows.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int endIndex = std::min(static_cast<int>(m_rows.size()), m_scrollOffset + visibleRows);

        for (int row = m_scrollOffset; row < endIndex; ++row) {
            const float rowTop = bodyRect.top + static_cast<float>(row - m_scrollOffset) * scaledRowHeight;
            const float rowBottom = std::min(bodyRect.bottom, rowTop + scaledRowHeight);
            const Rect rowRect = Rect::Make(m_bounds.left, rowTop, m_bounds.right, rowBottom);

            StyleState itemState = StyleState::Normal;
            if (row == m_selectedRow) {
                itemState = StyleState::Selected;
            } else if (row == m_hoveredRow) {
                itemState = StyleState::Hover;
            }

            Color rowBg = (row % 2 == 0) ? rowBackgroundA : rowBackgroundB;
            Color rowFg = shellFg;
            float rowRadius = 0.0f;
            if (itemStyle) {
                const StyleSpec itemSpec = itemStyle->Resolve(itemState);
                if (itemSpec.decoration.has_value()) {
                    if (itemState != StyleState::Normal || itemSpec.decoration->background.a > 0.001f) {
                        rowBg = itemSpec.decoration->background;
                    }
                    if (!itemSpec.decoration->radius.IsZero()) {
                        rowRadius = ScaleValue(itemSpec.decoration->radius.MaxRadius());
                    }
                }
                if (itemSpec.foreground.has_value()) {
                    rowFg = *itemSpec.foreground;
                }
            } else {
                if (row == m_selectedRow) {
                    rowBg = selectedRowBackground;
                } else if (row == m_hoveredRow) {
                    rowBg = hoverRowBackground;
                }
            }

            if (rowBg.a > 0.001f) {
                if (rowRadius > 0.5f) {
                    renderer.FillRoundedRect(rowRect, rowBg, rowRadius);
                } else {
                    renderer.FillRect(rowRect, rowBg);
                }
            }

            float rowX = m_bounds.left - m_hScroll;
            for (size_t col = 0; col < m_columns.size(); ++col) {
                const std::wstring value = (col < m_rows[row].size()) ? m_rows[row][col] : L"";
                const Rect textRect = Rect::Make(
                    rowX + scaledCellPadding.left,
                    rowTop + scaledCellPadding.top,
                    rowX + widths[col] - scaledCellPadding.right,
                    rowBottom - scaledCellPadding.bottom);
                const TextRenderOptions textOptions{
                    TextWrapMode::NoWrap,
                    TextHorizontalAlign::Start,
                    TextVerticalAlign::Center
                };
                renderer.DrawTextW(
                    value.c_str(),
                    static_cast<UINT32>(value.size()),
                    textRect,
                    rowFg,
                    ScaleValue(shellFs),
                    textOptions);
                rowX += widths[col];
            }

            renderer.DrawLine(
                PointF{m_bounds.left, rowBottom},
                PointF{m_bounds.right, rowBottom},
                gridLineColor,
                1.0f);
        }

        PaintPillScrollbars(
            renderer,
            m_bounds,
            contentW,
            contentH,
            viewW,
            viewH,
            m_hScroll,
            static_cast<float>(m_scrollOffset) * scaledRowHeight,
            scrollTrackColor,
            scrollThumbColor,
            ScaleValue(8.0f));

        renderer.PopLayer();
        (void)scaledHeaderHeight;
    }

    void SetColumns(std::vector<Column> columns) { m_columns = std::move(columns); }
    void SetRows(std::vector<std::vector<std::wstring>> rows) {
        m_rows = std::move(rows);
        if (m_rows.empty()) {
            m_selectedRow = -1;
            m_scrollOffset = 0;
        } else if (m_selectedRow < 0) {
            m_selectedRow = 0;
        } else {
            m_selectedRow = std::clamp(m_selectedRow, 0, static_cast<int>(m_rows.size()) - 1);
        }
        EnsureSelectionVisible();
    }

    void SetColumnWidths(std::vector<float> widths) {
        const size_t count = std::min(widths.size(), m_columns.size());
        for (size_t i = 0; i < count; ++i) {
            m_columns[i].width = widths[i];
        }
    }

    bool SetSelectedIndex(int index) {
        if (m_rows.empty()) {
            if (m_selectedRow == -1) {
                return false;
            }
            m_selectedRow = -1;
            return true;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(m_rows.size()) - 1);
        if (clamped == m_selectedRow) {
            return false;
        }
        m_selectedRow = clamped;
        EnsureSelectionVisible();
        return true;
    }

    int SelectedIndex() const { return m_selectedRow; }

    Color background = ColorFromHex(0x101C29);
    Color borderColor = ColorFromHex(0x35506A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color headerBackground = ColorFromHex(0x1D3247);
    Color rowBackgroundA = ColorFromHex(0x142536);
    Color rowBackgroundB = ColorFromHex(0x102132);
    Color hoverRowBackground = ColorFromHex(0x203B56);
    Color selectedRowBackground = ColorFromHex(0x2D567C);
    Color gridLineColor = ColorFromHex(0x2D445A);
    Color headerTextColor = ColorFromHex(0xEAF4FF);
    Color textColor = ColorFromHex(0xD0DDEC);
    Color scrollThumbColor = ColorFromHex(0x7A93AD);
    Color scrollTrackColor = ColorFromHex(0x162433);
    float cornerRadius = 10.0f;
    float headerHeight = 30.0f;
    float rowHeight = 26.0f;
    float fontSize = 13.0f;
    float headerFontSize = 12.0f;
    Thickness cellPadding{8, 5, 8, 5};

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
        if (m_style.itemStyle) {
            const StyleSpec hover = m_style.itemStyle->Resolve(StyleState::Hover);
            const StyleSpec selected = m_style.itemStyle->Resolve(StyleState::Selected);
            if (hover.decoration.has_value()) {
                hoverRowBackground = hover.decoration->background;
            }
            if (selected.decoration.has_value()) {
                selectedRowBackground = selected.decoration->background;
            }
        }
    }

    bool ScrollHorizontal(float delta) {
        const float before = m_hScroll;
        m_hScroll += delta;
        const auto widths = ResolveColumnWidths();
        float contentW = 0.0f;
        for (float w : widths) {
            contentW += w;
        }
        ClampHorizontalScroll(contentW, std::max(1.0f, m_bounds.Width()));
        return m_hScroll != before;
    }

    void ClampHorizontalScroll(float contentW, float viewW) {
        const float maxScroll = std::max(0.0f, contentW - viewW);
        m_hScroll = std::clamp(m_hScroll, 0.0f, maxScroll);
    }

    Rect HeaderRect() const {
        const float h = std::min(m_bounds.Height(), ScaleValue(headerHeight));
        return Rect::Make(m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.top + h);
    }

    Rect BodyRect() const {
        const Rect header = HeaderRect();
        return Rect::Make(m_bounds.left, header.bottom, m_bounds.right, m_bounds.bottom);
    }

    int VisibleRowCount() const {
        const Rect body = BodyRect();
        const float row = std::max(1.0f, ScaleValue(rowHeight));
        return std::max(1, static_cast<int>(std::floor(body.Height() / row)));
    }

    void EnsureSelectionVisible() {
        if (m_selectedRow < 0 || m_rows.empty()) {
            m_scrollOffset = 0;
            return;
        }
        const int visible = VisibleRowCount();
        if (m_selectedRow < m_scrollOffset) {
            m_scrollOffset = m_selectedRow;
        } else if (m_selectedRow >= m_scrollOffset + visible) {
            m_scrollOffset = m_selectedRow - visible + 1;
        }
        const int maxStart = std::max(0, static_cast<int>(m_rows.size()) - visible);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
    }

    int RowIndexFromPoint(float y) const {
        if (m_rows.empty()) {
            return -1;
        }
        const Rect body = BodyRect();
        if (y < body.top || y > body.bottom) {
            return -1;
        }
        const float row = std::max(1.0f, ScaleValue(rowHeight));
        const int displayRow = static_cast<int>(std::floor((y - body.top) / row));
        const int index = m_scrollOffset + displayRow;
        return (index >= 0 && index < static_cast<int>(m_rows.size())) ? index : -1;
    }

    std::vector<float> ResolveColumnWidths() const {
        std::vector<float> widths;
        if (m_columns.empty()) {
            return widths;
        }
        widths.resize(m_columns.size(), 0.0f);
        const float totalWidth = std::max(1.0f, m_bounds.Width());

        float fixed = 0.0f;
        size_t autoCount = 0;
        for (size_t i = 0; i < m_columns.size(); ++i) {
            if (m_columns[i].width > 0.0f) {
                widths[i] = ScaleValue(m_columns[i].width);
                fixed += widths[i];
            } else {
                ++autoCount;
            }
        }

        const float autoWidth = autoCount > 0
            ? std::max(ScaleValue(52.0f), (std::max(0.0f, totalWidth - fixed) / static_cast<float>(autoCount)))
            : 0.0f;
        float sum = 0.0f;
        for (size_t i = 0; i < widths.size(); ++i) {
            if (widths[i] <= 0.0f) {
                widths[i] = autoWidth;
            }
            widths[i] = std::max(ScaleValue(36.0f), widths[i]);
            sum += widths[i];
        }

        // Keep natural column widths; horizontal pill scrollbar handles overflow.
        (void)totalWidth;
        return widths;
    }

    std::vector<Column> m_columns;
    std::vector<std::vector<std::wstring>> m_rows;
    int m_selectedRow = -1;
    int m_hoveredRow = -1;
    int m_scrollOffset = 0;
    float m_hScroll = 0.0f;
};

class TreeView : public UIElement {
public:
    TreeView() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x101B28));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x35506A));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE8F5));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        auto item = std::make_unique<ComponentStyle>();
        item->base.decoration = BoxDecoration{};
        item->base.decoration->background = Color{0, 0, 0, 0};
        item->base.decoration->radius = CornerRadius::Uniform(4.0f);
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE8F5));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x1F3B55));
        hover.radius = CornerRadius::Uniform(4.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2E577D));
        selected.radius = CornerRadius::Uniform(4.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].decoration = selected;
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].foreground =
            ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));
        s.itemStyle = std::move(item);
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
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        bool changed = false;
        if (m_hasFocus) {
            m_hasFocus = false;
            changed = true;
        }
        if (m_hasHoveredPath) {
            m_hasHoveredPath = false;
            m_hoveredPath.clear();
            changed = true;
        }
        return changed;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            return false;
        }

        int selectedIndex = SelectedVisibleIndex(visible);
        if (selectedIndex < 0) {
            selectedIndex = 0;
            SetSelectedPath(visible[0].path);
        }

        if (keyCode == VK_UP) {
            if (selectedIndex > 0) {
                SetSelectedPath(visible[static_cast<size_t>(selectedIndex - 1)].path);
                EnsureSelectedVisible(visible);
                return true;
            }
            return false;
        }
        if (keyCode == VK_DOWN) {
            if (selectedIndex + 1 < static_cast<int>(visible.size())) {
                SetSelectedPath(visible[static_cast<size_t>(selectedIndex + 1)].path);
                EnsureSelectedVisible(visible);
                return true;
            }
            return false;
        }
        if (keyCode == VK_HOME) {
            SetSelectedPath(visible.front().path);
            EnsureSelectedVisible(visible);
            return true;
        }
        if (keyCode == VK_END) {
            SetSelectedPath(visible.back().path);
            EnsureSelectedVisible(visible);
            return true;
        }

        Node* selected = GetNode(m_selectedPath);
        if (!selected) {
            return false;
        }

        if (keyCode == VK_LEFT) {
            if (!selected->children.empty() && selected->expanded) {
                selected->expanded = false;
                return true;
            }
            if (m_selectedPath.size() > 1) {
                auto parentPath = m_selectedPath;
                parentPath.pop_back();
                SetSelectedPath(parentPath);
                EnsureSelectedVisible(BuildVisibleNodes());
                return true;
            }
            return false;
        }

        if (keyCode == VK_RIGHT) {
            if (!selected->children.empty() && !selected->expanded) {
                selected->expanded = true;
                return true;
            }
            if (!selected->children.empty()) {
                auto childPath = m_selectedPath;
                childPath.push_back(0);
                SetSelectedPath(childPath);
                EnsureSelectedVisible(BuildVisibleNodes());
                return true;
            }
            return false;
        }

        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            if (!selected->children.empty()) {
                selected->expanded = !selected->expanded;
                return true;
            }
            return false;
        }

        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }

        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            return true;
        }
        const int visibleIndex = VisibleIndexFromPoint(y, static_cast<int>(visible.size()));
        if (visibleIndex < 0 || visibleIndex >= static_cast<int>(visible.size())) {
            return true;
        }

        const auto& entry = visible[static_cast<size_t>(visibleIndex)];
        const int rowIndex = m_scrollOffset + visibleIndex;
        SetSelectedPath(entry.path);
        EnsureSelectedVisible(visible);

        if (entry.node && !entry.node->children.empty() && ToggleGlyphRect(entry.depth, rowIndex).Contains(x, y)) {
            entry.node->expanded = !entry.node->expanded;
        }
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        auto visible = BuildVisibleNodes();
        if (!HitTest(x, y) || visible.empty()) {
            if (!m_hasHoveredPath) {
                return false;
            }
            m_hasHoveredPath = false;
            m_hoveredPath.clear();
            return true;
        }

        const int visibleIndex = VisibleIndexFromPoint(y, static_cast<int>(visible.size()));
        if (visibleIndex < 0 || visibleIndex >= static_cast<int>(visible.size())) {
            if (!m_hasHoveredPath) {
                return false;
            }
            m_hasHoveredPath = false;
            m_hoveredPath.clear();
            return true;
        }

        const auto& path = visible[static_cast<size_t>(visibleIndex)].path;
        if (m_hasHoveredPath && path == m_hoveredPath) {
            return false;
        }
        m_hoveredPath = path;
        m_hasHoveredPath = true;
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_hasHoveredPath) {
            return false;
        }
        m_hasHoveredPath = false;
        m_hoveredPath.clear();
        return true;
    }

    bool OnMouseWheel(float delta, float x, float y, bool /*shiftHeld*/) override {
        if (!HitTest(x, y)) {
            return false;
        }
        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            return false;
        }
        const int before = m_scrollOffset;
        const int visibleRows = VisibleRowCount(static_cast<int>(visible.size()));
        m_scrollOffset = std::clamp(
            m_scrollOffset - static_cast<int>(std::lround(delta * 3.0f)),
            0,
            std::max(0, static_cast<int>(visible.size()) - visibleRows));
        return m_scrollOffset != before;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float preferred = ScaleValue(320.0f);
        const float maxWidth = std::max(availableWidth, preferred);
        return std::min(availableWidth, maxWidth);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(200.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCornerRadius = ScaleValue(cornerRadius);
        Color shellFg = textColor;
        float shellFs = fontSize;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCornerRadius = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            shellFg = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            shellFs = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            const std::wstring message = L"(empty tree)";
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                message.c_str(),
                static_cast<UINT32>(message.size()),
                m_bounds,
                mutedTextColor,
                ScaleValue(shellFs),
                textOptions);
            return;
        }

        const Rect content = ContentRect();
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int visibleRows = std::max(1, static_cast<int>(std::floor(content.Height() / row)));
        const int maxStart = std::max(0, static_cast<int>(visible.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int count = std::min(visibleRows, static_cast<int>(visible.size()) - m_scrollOffset);
        const ComponentStyle* itemStyle = m_style.itemStyle ? m_style.itemStyle.get() : nullptr;

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);
        for (int i = 0; i < count; ++i) {
            const int globalIndex = m_scrollOffset + i;
            const auto& entry = visible[static_cast<size_t>(globalIndex)];
            const float top = content.top + static_cast<float>(i) * row;
            const float bottom = std::min(content.bottom, top + row);
            const Rect rowRect = Rect::Make(content.left, top, content.right, bottom);

            const bool selected = entry.path == m_selectedPath;
            const bool hovered = m_hasHoveredPath && entry.path == m_hoveredPath;
            StyleState itemState = StyleState::Normal;
            if (selected) {
                itemState = StyleState::Selected;
            } else if (hovered) {
                itemState = StyleState::Hover;
            }

            Color rowBg = Color{0, 0, 0, 0};
            Color rowFg = shellFg;
            float rowRadius = ScaleValue(4.0f);
            if (itemStyle) {
                const StyleSpec itemSpec = itemStyle->Resolve(itemState);
                if (itemSpec.decoration.has_value()) {
                    rowBg = itemSpec.decoration->background;
                    if (!itemSpec.decoration->radius.IsZero()) {
                        rowRadius = ScaleValue(itemSpec.decoration->radius.MaxRadius());
                    }
                }
                if (itemSpec.foreground.has_value()) {
                    rowFg = *itemSpec.foreground;
                }
            } else {
                if (selected) {
                    rowBg = selectedBackground;
                } else if (hovered) {
                    rowBg = hoverBackground;
                }
            }
            if (rowBg.a > 0.001f) {
                renderer.FillRoundedRect(rowRect, rowBg, rowRadius);
            }

            const float indent = ScaleValue(indentWidth) * static_cast<float>(entry.depth);
            const float glyphLeft = content.left + ScaleValue(4.0f) + indent;
            const float glyphCenterY = top + row * 0.5f;

            if (!entry.node->children.empty()) {
                const Rect glyphRect = ToggleGlyphRect(entry.depth, globalIndex);
                renderer.DrawRect(glyphRect, glyphColor, 1.0f);
                renderer.DrawLine(
                    PointF{glyphRect.left + ScaleValue(2.0f), glyphCenterY},
                    PointF{glyphRect.right - ScaleValue(2.0f), glyphCenterY},
                    glyphColor,
                    1.0f);
                if (!entry.node->expanded) {
                    renderer.DrawLine(
                        PointF{glyphRect.left + glyphRect.Width() * 0.5f, glyphRect.top + ScaleValue(2.0f)},
                        PointF{glyphRect.left + glyphRect.Width() * 0.5f, glyphRect.bottom - ScaleValue(2.0f)},
                        glyphColor,
                        1.0f);
                }
            }

            const float textLeft = glyphLeft + ScaleValue(16.0f);
            const Rect textRect = Rect::Make(textLeft, top, content.right - ScaleValue(6.0f), bottom);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                entry.node->label.c_str(),
                static_cast<UINT32>(entry.node->label.size()),
                textRect,
                rowFg,
                ScaleValue(shellFs),
                textOptions);
        }

        const float contentH = row * static_cast<float>(std::max<size_t>(1, visible.size()));
        PaintPillScrollbars(
            renderer,
            m_bounds,
            content.Width(),
            contentH,
            content.Width(),
            content.Height(),
            0.0f,
            static_cast<float>(m_scrollOffset) * row,
            scrollTrackColor,
            scrollThumbColor,
            ScaleValue(8.0f));

        renderer.PopLayer();
    }

    void SetPaths(std::vector<std::wstring> paths) {
        m_roots.clear();
        for (const auto& path : paths) {
            AddPath(path);
        }
        if (!m_roots.empty()) {
            m_selectedPath = std::vector<int>{0};
        } else {
            m_selectedPath.clear();
        }
        m_scrollOffset = 0;
    }

    void SetExpandedAll(bool expanded) {
        for (auto& root : m_roots) {
            SetExpandedAllRecursive(root, expanded);
        }
    }

    Color background = ColorFromHex(0x101B28);
    Color borderColor = ColorFromHex(0x35506A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color selectedBackground = ColorFromHex(0x2E577D);
    Color hoverBackground = ColorFromHex(0x1F3B55);
    Color textColor = ColorFromHex(0xDCE8F5);
    Color mutedTextColor = ColorFromHex(0x96A9BC);
    Color glyphColor = ColorFromHex(0xAFC3D9);
    Color scrollThumbColor = ColorFromHex(0x7A93AD);
    Color scrollTrackColor = ColorFromHex(0x162433);
    float cornerRadius = 10.0f;
    float fontSize = 13.0f;
    float itemHeight = 24.0f;
    float indentWidth = 18.0f;

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
        if (m_style.itemStyle) {
            const StyleSpec hover = m_style.itemStyle->Resolve(StyleState::Hover);
            const StyleSpec selected = m_style.itemStyle->Resolve(StyleState::Selected);
            if (hover.decoration.has_value()) {
                hoverBackground = hover.decoration->background;
            }
            if (selected.decoration.has_value()) {
                selectedBackground = selected.decoration->background;
            }
        }
    }

    struct Node {
        std::wstring label;
        std::vector<Node> children;
        bool expanded = true;
    };

    struct VisibleNode {
        Node* node = nullptr;
        int depth = 0;
        std::vector<int> path;
    };

    static std::wstring TrimWide(const std::wstring& value) {
        size_t start = 0;
        while (start < value.size() && (value[start] == L' ' || value[start] == L'\t' || value[start] == L'\r' || value[start] == L'\n')) {
            ++start;
        }
        size_t end = value.size();
        while (end > start && (value[end - 1] == L' ' || value[end - 1] == L'\t' || value[end - 1] == L'\r' || value[end - 1] == L'\n')) {
            --end;
        }
        return value.substr(start, end - start);
    }

    static std::vector<std::wstring> SplitPath(const std::wstring& path) {
        std::vector<std::wstring> segments;
        std::wstring current;
        for (wchar_t ch : path) {
            if (ch == L'/' || ch == L'>' || ch == L'\\') {
                const std::wstring trimmed = TrimWide(current);
                if (!trimmed.empty()) {
                    segments.push_back(trimmed);
                }
                current.clear();
            } else {
                current.push_back(ch);
            }
        }
        const std::wstring trimmed = TrimWide(current);
        if (!trimmed.empty()) {
            segments.push_back(trimmed);
        }
        return segments;
    }

    void AddPath(const std::wstring& path) {
        const auto segments = SplitPath(path);
        if (segments.empty()) {
            return;
        }

        std::vector<Node>* level = &m_roots;
        for (const auto& segment : segments) {
            auto it = std::find_if(level->begin(), level->end(), [&](const Node& node) {
                return node.label == segment;
            });
            if (it == level->end()) {
                level->push_back(Node{segment, {}, true});
                it = level->end() - 1;
            }
            level = &it->children;
        }
    }

    void SetExpandedAllRecursive(Node& node, bool expanded) {
        node.expanded = expanded;
        for (auto& child : node.children) {
            SetExpandedAllRecursive(child, expanded);
        }
    }

    Rect ContentRect() const {
        const float inset = ScaleValue(4.0f);
        return Rect::Make(
            m_bounds.left + inset,
            m_bounds.top + inset,
            m_bounds.right - inset,
            m_bounds.bottom - inset);
    }

    Rect ToggleGlyphRect(int depth, int globalIndex) const {
        const Rect content = ContentRect();
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int localIndex = globalIndex - m_scrollOffset;
        const float rowTop = content.top + static_cast<float>(localIndex) * row;
        const float centerY = rowTop + row * 0.5f;
        const float left = content.left + ScaleValue(4.0f) + ScaleValue(indentWidth) * static_cast<float>(depth);
        return Rect::Make(left, centerY - ScaleValue(5.0f), left + ScaleValue(10.0f), centerY + ScaleValue(5.0f));
    }

    std::vector<VisibleNode> BuildVisibleNodes() {
        std::vector<VisibleNode> result;
        std::vector<int> path;
        for (size_t i = 0; i < m_roots.size(); ++i) {
            path.clear();
            path.push_back(static_cast<int>(i));
            AppendVisibleNode(m_roots[i], 0, path, result);
        }
        return result;
    }

    void AppendVisibleNode(Node& node, int depth, std::vector<int>& path, std::vector<VisibleNode>& out) {
        out.push_back(VisibleNode{&node, depth, path});
        if (!node.expanded) {
            return;
        }
        for (size_t i = 0; i < node.children.size(); ++i) {
            path.push_back(static_cast<int>(i));
            AppendVisibleNode(node.children[i], depth + 1, path, out);
            path.pop_back();
        }
    }

    Node* GetNode(const std::vector<int>& path) {
        if (path.empty()) {
            return nullptr;
        }
        if (path[0] < 0 || path[0] >= static_cast<int>(m_roots.size())) {
            return nullptr;
        }

        Node* node = &m_roots[static_cast<size_t>(path[0])];
        for (size_t i = 1; i < path.size(); ++i) {
            const int idx = path[i];
            if (idx < 0 || idx >= static_cast<int>(node->children.size())) {
                return nullptr;
            }
            node = &node->children[static_cast<size_t>(idx)];
        }
        return node;
    }

    int SelectedVisibleIndex(const std::vector<VisibleNode>& visible) const {
        for (size_t i = 0; i < visible.size(); ++i) {
            if (visible[i].path == m_selectedPath) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    int VisibleRowCount(int totalVisible) const {
        const Rect content = ContentRect();
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int count = std::max(1, static_cast<int>(std::floor(content.Height() / row)));
        return std::min(count, std::max(1, totalVisible));
    }

    void EnsureSelectedVisible(const std::vector<VisibleNode>& visible) {
        if (visible.empty()) {
            m_scrollOffset = 0;
            return;
        }
        const int index = SelectedVisibleIndex(visible);
        if (index < 0) {
            return;
        }
        const int visibleRows = VisibleRowCount(static_cast<int>(visible.size()));
        if (index < m_scrollOffset) {
            m_scrollOffset = index;
        } else if (index >= m_scrollOffset + visibleRows) {
            m_scrollOffset = index - visibleRows + 1;
        }
        const int maxStart = std::max(0, static_cast<int>(visible.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
    }

    int VisibleIndexFromPoint(float y, int totalVisible) const {
        const Rect content = ContentRect();
        if (y < content.top || y > content.bottom) {
            return -1;
        }
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int local = static_cast<int>(std::floor((y - content.top) / row));
        const int visibleRows = VisibleRowCount(totalVisible);
        if (local < 0 || local >= visibleRows) {
            return -1;
        }
        return local;
    }

    bool SetSelectedPath(std::vector<int> path) {
        if (path == m_selectedPath) {
            return false;
        }
        m_selectedPath = std::move(path);
        return true;
    }

    std::vector<Node> m_roots;
    std::vector<int> m_selectedPath;
    std::vector<int> m_hoveredPath;
    bool m_hasHoveredPath = false;
    int m_scrollOffset = 0;
};

struct StripItem {
    std::wstring text;
    std::vector<uint8_t> svgData;
    SvgHandle svg = nullptr;
    Size intrinsicSize{16.0f, 16.0f};

    bool HasIcon() const { return !svgData.empty(); }
};

inline void EnsureStripItemSvg(IRenderer& renderer, StripItem& item) {
    if (!item.svg && !item.svgData.empty()) {
        item.svg = renderer.CreateSvgFromBytes(item.svgData.data(), item.svgData.size());
        if (item.svg) {
            item.intrinsicSize = renderer.GetSvgSize(item.svg);
        }
    }
}

