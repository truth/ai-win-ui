// Shared protocol constants 1:1 with src/react/react_protocol.h.

export const enum NodeType {
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
  FinancialChart = 19,
  CircularProgress = 20,
  Image = 21,
}

export const enum Prop {
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
  TextColor = 74,

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

  ScrollX = 130,
  ScrollY = 131,
  ShowScrollbars = 132,
  BarThickness = 133,
  OnText = 134,
  OffText = 135,
  LabelText = 136,

  ChartMode = 140,
  PreClose = 141,
  ModernTheme = 142,
  ShowCrosshair = 143,
  ShowHeaderInfo = 144,
  SymbolName = 145,
  PeriodLabel = 146,

  Src = 150,
}

export const enum EventType {
  Click = 1,
  Change = 2,
  TextChanged = 3,
  Submit = 4,
  PageChange = 5,
  TreeSelect = 6,
  TreeCheck = 7,
  KeyDown = 8,
}

export const enum EventMask {
  Click = 1 << 0,
  Change = 1 << 1,
  TextChanged = 1 << 2,
  Submit = 1 << 3,
  PageChange = 1 << 4,
  TreeSelect = 1 << 5,
  TreeCheck = 1 << 6,
}

export const TAG_TO_NODE_TYPE: Record<string, NodeType> = {
  Panel: NodeType.Panel,
  ScrollViewer: NodeType.ScrollViewer,
  Label: NodeType.Label,
  Button: NodeType.Button,
  TextInput: NodeType.TextInput,
  Switch: NodeType.Switch,
  Slider: NodeType.Slider,
  Checkbox: NodeType.Checkbox,
  ProgressBar: NodeType.ProgressBar,
  Badge: NodeType.Badge,
  Avatar: NodeType.Avatar,
  Divider: NodeType.Divider,
  Icon: NodeType.Icon,
  Breadcrumb: NodeType.Breadcrumb,
  BreadcrumbItem: NodeType.BreadcrumbItem,
  Pagination: NodeType.Pagination,
  TreeView: NodeType.TreeView,
  TreeItem: NodeType.TreeItem,
  FinancialChart: NodeType.FinancialChart,
  CircularProgress: NodeType.CircularProgress,
  Image: NodeType.Image,
};

export interface ThicknessObject {
  x?: number;
  y?: number;
  left?: number;
  top?: number;
  right?: number;
  bottom?: number;
}

export type ThicknessInput = number | ThicknessObject;
export type ColorInput = string | number;

export function parseColor(value: ColorInput): number | string {
  if (typeof value === "number") {
    return value >>> 0;
  }
  const s = value.trim();
  if (s.startsWith("$color.")) {
    return s;
  }
  const lower = s.toLowerCase();
  if (lower === "transparent" || lower === "none") {
    return 0x00000000;
  }
  if (s[0] === "#") {
    const hex = s.slice(1);
    if (hex.length === 3) {
      const r = parseInt(hex[0] + hex[0], 16);
      const g = parseInt(hex[1] + hex[1], 16);
      const b = parseInt(hex[2] + hex[2], 16);
      return ((0xff << 24) | (r << 16) | (g << 8) | b) >>> 0;
    }
    if (hex.length === 4) {
      const r = parseInt(hex[0] + hex[0], 16);
      const g = parseInt(hex[1] + hex[1], 16);
      const b = parseInt(hex[2] + hex[2], 16);
      const a = parseInt(hex[3] + hex[3], 16);
      return ((a << 24) | (r << 16) | (g << 8) | b) >>> 0;
    }
    if (hex.length === 6) {
      const rgb = parseInt(hex, 16) >>> 0;
      return (0xff000000 | rgb) >>> 0;
    }
    if (hex.length === 8) {
      // Match ai-win-ui CSS #RRGGBBAA -> 0xAARRGGBB
      const r = parseInt(hex.slice(0, 2), 16);
      const g = parseInt(hex.slice(2, 4), 16);
      const b = parseInt(hex.slice(4, 6), 16);
      const a = parseInt(hex.slice(6, 8), 16);
      return ((a << 24) | (r << 16) | (g << 8) | b) >>> 0;
    }
  }
  return s;
}

export function normalizeThickness(v: ThicknessInput): [number, number, number, number] {
  if (typeof v === "number") {
    return [v, v, v, v];
  }
  const hx = v.x ?? 0;
  const hy = v.y ?? 0;
  return [v.left ?? hx, v.top ?? hy, v.right ?? hx, v.bottom ?? hy];
}

