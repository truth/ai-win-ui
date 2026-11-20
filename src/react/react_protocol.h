#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ai_win_ui::react {

enum class NodeTypeId : uint16_t {
    Panel = 1,
    ScrollViewer = 2,
    Label = 3,
    Button = 4,
    TextInput = 5,
    Switch = 6,
    Slider = 7,
    Checkbox = 8,
    ProgressBar = 9,
    Badge = 10,
    Avatar = 11,
    Divider = 12,
    Icon = 13,
    Breadcrumb = 14,
    BreadcrumbItem = 15,
    Pagination = 16,
    TreeView = 17,
    TreeItem = 18,
};

enum class PropId : uint16_t {
    // Common UIElement / Yoga layout
    Name = 1,
    Width = 2,
    Height = 3,
    MinWidth = 4,
    MaxWidth = 5,
    MinHeight = 6,
    MaxHeight = 7,
    FlexGrow = 8,
    FlexShrink = 9,
    FlexBasis = 10,
    AlignSelf = 11,
    Margin = 12,
    Border = 13,
    Disabled = 14,
    Tooltip = 15,
    StyleRef = 16,
    HitTestRole = 17,

    // Panel / Container visual & flex
    Direction = 20,
    AlignItems = 21,
    JustifyContent = 22,
    Wrap = 23,
    Padding = 24,
    Spacing = 25,
    CornerRadius = 26,
    Background = 27,
    BorderColor = 28,
    ShadowBlur = 29,
    ShadowOffsetX = 30,
    ShadowOffsetY = 31,
    ShadowColor = 32,
    GradientStart = 33,
    GradientEnd = 34,
    GradientAngle = 35,

    // Text / Input / Common Widget props
    Text = 40,
    FontSize = 41,
    Color = 42,
    Foreground = 43,
    FontWeight = 44,
    Bold = 45,
    Italic = 46,
    Ellipsis = 47,
    HorizontalAlign = 48,
    VerticalAlign = 49,
    Placeholder = 50,
    PlaceholderColor = 51,
    FocusBorderColor = 52,

    // Interactive value / range props (subject to eventSeq suppression)
    Checked = 60,
    Value = 61,
    Min = 62,
    Max = 63,
    Step = 64,
    ShowValueText = 65,
    BarHeight = 66,
    TrackColor = 67,
    FillColor = 68,
    ThumbColor = 69,
    ActiveColor = 70,
    InactiveColor = 71,
    HoverColor = 72,
    HighlightColor = 73,

    // Badge / Avatar / Divider / Icon
    Variant = 80,
    Dot = 81,
    Icon = 82,
    Size = 83,
    Circle = 84,
    Status = 85,
    Initials = 86,
    Vertical = 87,
    Dashed = 88,
    ThicknessVal = 89,
    StrokeWidth = 90,
    Fill = 91,

    // Breadcrumb / Pagination / TreeView / TreeItem
    Separator = 100,
    SeparatorColor = 101,
    Active = 102,
    Href = 103,
    Current = 104,
    Total = 105,
    PageSize = 106,
    ShowTotal = 107,
    ItemSize = 108,
    ActiveTextColor = 109,
    DisabledColor = 110,
    ShowLines = 111,
    ShowCheckBox = 112,
    ItemHeight = 113,
    IndentWidth = 114,
    Expanded = 115,
    Selected = 116,
    Header = 117,
    SelectedBackground = 118,
    HoverBackground = 119,
    LineColor = 120,
    GlyphColor = 121,

    // ScrollViewer & Switch text
    ScrollX = 130,
    ScrollY = 131,
    ShowScrollbars = 132,
    BarThickness = 133,
    OnText = 134,
    OffText = 135,
    LabelText = 136,
};

enum class EventTypeId : uint8_t {
    Click = 1,
    Change = 2,
    TextChanged = 3,
    Submit = 4,
    PageChange = 5,
    TreeSelect = 6,
    TreeCheck = 7,
};

namespace EventMask {
constexpr uint32_t Click = 1u << 0;
constexpr uint32_t Change = 1u << 1;
constexpr uint32_t TextChanged = 1u << 2;
constexpr uint32_t Submit = 1u << 3;
constexpr uint32_t PageChange = 1u << 4;
constexpr uint32_t TreeSelect = 1u << 5;
constexpr uint32_t TreeCheck = 1u << 6;
} // namespace EventMask

enum class MutationOpType : uint8_t {
    CreateNode = 1,
    SetPropBool = 2,
    SetPropNum = 3,
    SetPropColor = 4,
    SetPropThickness = 5,
    SetPropStr = 6,
    SetEventMask = 7,
    AppendChild = 8,
    InsertBefore = 9,
    RemoveChild = 10,
    DestroyNode = 11,
    SetRoot = 12,
    SetWindowTitle = 13,
    OpenDemo = 14,
    ResetTree = 15,
};

struct MutationOp {
    MutationOpType type = MutationOpType::CreateNode;
    uint32_t nodeId = 0;
    uint32_t parentId = 0;
    uint32_t refId = 0;
    uint32_t ackSeq = 0;
    uint32_t uintArg = 0;
    uint16_t nodeType = 0;
    uint16_t propId = 0;
    bool boolVal = false;
    double numVal = 0.0;
    float tL = 0.0f;
    float tT = 0.0f;
    float tR = 0.0f;
    float tB = 0.0f;
    std::string strVal;
    std::string strVal2;
    std::string strVal3;
};

struct MutationBatch {
    std::vector<MutationOp> ops;
    bool needsLayout = false;
};

struct UiEvent {
    uint32_t nodeId = 0;
    EventTypeId type = EventTypeId::Click;
    uint32_t eventSeq = 0;
    bool boolVal = false;
    int32_t intVal = 0;
    int32_t intVal2 = 0;
    double numVal = 0.0;
    std::string strVal;
};

} // namespace ai_win_ui::react

