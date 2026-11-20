import { createElement, type ChildNode, type ComponentFn } from "./jsx-runtime.ts";
import type { ColorInput, ThicknessInput } from "./protocol.ts";

export interface CommonProps {
  key?: string | number;
  id?: string;
  width?: number;
  height?: number;
  minWidth?: number;
  maxWidth?: number;
  minHeight?: number;
  maxHeight?: number;
  flexGrow?: number;
  flexShrink?: number;
  flexBasis?: number;
  alignSelf?: "auto" | "stretch" | "start" | "center" | "end";
  margin?: ThicknessInput;
  border?: ThicknessInput;
  disabled?: boolean;
  tooltip?: string;
  styleRef?: string;
  hitTest?: "default" | "caption" | "client";
}

export interface PanelProps extends CommonProps {
  direction?: "column" | "row";
  alignItems?: "stretch" | "start" | "center" | "end";
  justifyContent?: "start" | "center" | "end" | "space-between" | "spaceBetween";
  wrap?: "nowrap" | "wrap";
  padding?: ThicknessInput;
  spacing?: number;
  cornerRadius?: number;
  background?: ColorInput;
  borderColor?: ColorInput;
  shadowBlur?: number;
  shadowOffsetX?: number;
  shadowOffsetY?: number;
  shadowColor?: ColorInput;
  gradientStart?: ColorInput;
  gradientEnd?: ColorInput;
  gradientAngle?: number;
  children?: ChildNode;
}

export interface ScrollViewerProps extends CommonProps {
  scrollX?: boolean;
  scrollY?: boolean;
  showScrollbars?: boolean;
  barThickness?: number;
  background?: ColorInput;
  trackColor?: ColorInput;
  thumbColor?: ColorInput;
  children?: ChildNode;
}

export interface LabelProps extends CommonProps {
  text?: string;
  fontSize?: number;
  color?: ColorInput;
  foreground?: ColorInput;
  fontWeight?: number;
  bold?: boolean;
  italic?: boolean;
  ellipsis?: boolean;
  horizontalAlign?: "start" | "center" | "end";
  verticalAlign?: "start" | "center" | "end";
  children?: ChildNode;
}

export interface ButtonProps extends CommonProps {
  text?: string;
  fontSize?: number;
  cornerRadius?: number;
  background?: ColorInput;
  foreground?: ColorInput;
  color?: ColorInput;
  borderColor?: ColorInput;
  onClick?: () => void;
  children?: ChildNode;
}

export interface TextInputProps extends CommonProps {
  text?: string;
  placeholder?: string;
  placeholderColor?: ColorInput;
  fontSize?: number;
  cornerRadius?: number;
  background?: ColorInput;
  borderColor?: ColorInput;
  focusBorderColor?: ColorInput;
  color?: ColorInput;
  foreground?: ColorInput;
  onTextChanged?: (text: string) => void;
  onSubmit?: (text: string) => void;
}

export interface SwitchProps extends CommonProps {
  checked?: boolean;
  label?: string;
  text?: string;
  onText?: string;
  offText?: string;
  activeColor?: ColorInput;
  inactiveColor?: ColorInput;
  color?: ColorInput;
  foreground?: ColorInput;
  fontSize?: number;
  onChange?: (checked: boolean) => void;
  children?: ChildNode;
}

export interface SliderProps extends CommonProps {
  value?: number;
  min?: number;
  max?: number;
  step?: number;
  text?: string;
  label?: string;
  fontSize?: number;
  cornerRadius?: number;
  background?: ColorInput;
  borderColor?: ColorInput;
  highlightColor?: ColorInput;
  activeColor?: ColorInput;
  fillColor?: ColorInput;
  color?: ColorInput;
  foreground?: ColorInput;
  onChange?: (value: number) => void;
}

export interface CheckboxProps extends CommonProps {
  checked?: boolean;
  text?: string;
  color?: ColorInput;
  foreground?: ColorInput;
  fontSize?: number;
  onChange?: (checked: boolean) => void;
  children?: ChildNode;
}

export interface ProgressBarProps extends CommonProps {
  value?: number;
  min?: number;
  max?: number;
  text?: string;
  label?: string;
  showValueText?: boolean;
  barHeight?: number;
  background?: ColorInput;
  borderColor?: ColorInput;
  trackColor?: ColorInput;
  fillColor?: ColorInput;
  activeColor?: ColorInput;
  color?: ColorInput;
  foreground?: ColorInput;
  cornerRadius?: number;
  fontSize?: number;
}

export interface BadgeProps extends CommonProps {
  text?: string;
  variant?: "neutral" | "success" | "warning" | "danger" | "error" | "info" | "primary";
  dot?: boolean;
  icon?: string;
  fontSize?: number;
  cornerRadius?: number;
  background?: ColorInput;
  color?: ColorInput;
  foreground?: ColorInput;
  borderColor?: ColorInput;
  children?: ChildNode;
}

export interface AvatarProps extends CommonProps {
  initials?: string;
  text?: string;
  icon?: string;
  size?: number;
  circle?: boolean;
  cornerRadius?: number;
  background?: ColorInput;
  color?: ColorInput;
  foreground?: ColorInput;
  borderColor?: ColorInput;
  thickness?: number;
  strokeWidth?: number;
  status?: "online" | "busy" | "away" | "offline";
}

export interface DividerProps extends CommonProps {
  vertical?: boolean;
  dashed?: boolean;
  thickness?: number;
  color?: ColorInput;
  borderColor?: ColorInput;
  text?: string;
  foreground?: ColorInput;
  fontSize?: number;
  children?: ChildNode;
}

export interface IconProps extends CommonProps {
  name?: string;
  icon?: string;
  size?: number;
  color?: ColorInput;
  foreground?: ColorInput;
  fill?: ColorInput;
  strokeWidth?: number;
}

export interface BreadcrumbProps extends CommonProps {
  separator?: string;
  spacing?: number;
  fontSize?: number;
  separatorColor?: ColorInput;
  color?: ColorInput;
  children?: ChildNode;
}

export interface BreadcrumbItemProps extends CommonProps {
  text?: string;
  icon?: string;
  href?: string;
  active?: boolean;
  fontSize?: number;
  color?: ColorInput;
  activeColor?: ColorInput;
  hoverColor?: ColorInput;
  onClick?: () => void;
  children?: ChildNode;
}

export interface PaginationProps extends CommonProps {
  current?: number;
  total?: number;
  pageSize?: number;
  showTotal?: boolean;
  itemSize?: number;
  spacing?: number;
  fontSize?: number;
  cornerRadius?: number;
  activeColor?: ColorInput;
  activeTextColor?: ColorInput;
  hoverColor?: ColorInput;
  color?: ColorInput;
  disabledColor?: ColorInput;
  borderColor?: ColorInput;
  onPageChange?: (page: number, pageSize: number) => void;
}

export interface TreeViewProps extends CommonProps {
  showLines?: boolean;
  showCheckBox?: boolean;
  itemHeight?: number;
  indentWidth?: number;
  fontSize?: number;
  cornerRadius?: number;
  background?: ColorInput;
  borderColor?: ColorInput;
  selectedBackground?: ColorInput;
  hoverBackground?: ColorInput;
  color?: ColorInput;
  lineColor?: ColorInput;
  glyphColor?: ColorInput;
  onSelect?: (label: string, itemNodeId: number) => void;
  onCheck?: (checked: boolean, label?: string, itemNodeId?: number) => void;
  children?: ChildNode;
}

export interface TreeItemProps extends CommonProps {
  header?: string;
  text?: string;
  icon?: string;
  expanded?: boolean;
  selected?: boolean;
  checked?: boolean;
  onSelect?: (label: string, itemNodeId: number) => void;
  onCheck?: (checked: boolean) => void;
  children?: ChildNode;
}

export interface FinancialPointData {
  price: number;
  leadPrice: number;
  avgPrice?: number;
  volume: number;
  indicator1?: number;
  indicator2?: number;
  isUp: boolean;
  timeStr: string;
}

export interface CandleBarData {
  date: string;
  open: number;
  high: number;
  low: number;
  close: number;
  volume: number;
  ma5: number;
  ma10: number;
  ma20: number;
}

export interface FinancialChartProps extends CommonProps {
  mode?: "timeshare" | "candlestick";
  preClose?: number;
  modernTheme?: boolean;
  showCrosshair?: boolean;
  showHeaderInfo?: boolean;
  symbolName?: string;
  periodLabel?: string;
  points?: FinancialPointData[];
  candles?: CandleBarData[];
}

export interface SpacerProps {
  width?: number;
  height?: number;
  flexGrow?: number;
  flexBasis?: number;
}

declare global {
  namespace JSX {
    interface IntrinsicElements {
      Panel: PanelProps;
      ScrollViewer: ScrollViewerProps;
      Label: LabelProps;
      Button: ButtonProps;
      TextInput: TextInputProps;
      Switch: SwitchProps;
      Slider: SliderProps;
      Checkbox: CheckboxProps;
      ProgressBar: ProgressBarProps;
      Badge: BadgeProps;
      Avatar: AvatarProps;
      Divider: DividerProps;
      Icon: IconProps;
      Breadcrumb: BreadcrumbProps;
      BreadcrumbItem: BreadcrumbItemProps;
      Pagination: PaginationProps;
      TreeView: TreeViewProps;
      TreeItem: TreeItemProps;
      FinancialChart: FinancialChartProps;
    }
  }
}

export const Panel = "Panel" as unknown as ComponentFn<PanelProps>;
export const ScrollViewer = "ScrollViewer" as unknown as ComponentFn<ScrollViewerProps>;
export const Label = "Label" as unknown as ComponentFn<LabelProps>;
export const Button = "Button" as unknown as ComponentFn<ButtonProps>;
export const TextInput = "TextInput" as unknown as ComponentFn<TextInputProps>;
export const Switch = "Switch" as unknown as ComponentFn<SwitchProps>;
export const Slider = "Slider" as unknown as ComponentFn<SliderProps>;
export const Checkbox = "Checkbox" as unknown as ComponentFn<CheckboxProps>;
export const ProgressBar = "ProgressBar" as unknown as ComponentFn<ProgressBarProps>;
export const Badge = "Badge" as unknown as ComponentFn<BadgeProps>;
export const Avatar = "Avatar" as unknown as ComponentFn<AvatarProps>;
export const Divider = "Divider" as unknown as ComponentFn<DividerProps>;
export const Icon = "Icon" as unknown as ComponentFn<IconProps>;
export const Breadcrumb = "Breadcrumb" as unknown as ComponentFn<BreadcrumbProps>;
export const BreadcrumbItem = "BreadcrumbItem" as unknown as ComponentFn<BreadcrumbItemProps>;
export const Pagination = "Pagination" as unknown as ComponentFn<PaginationProps>;
export const TreeView = "TreeView" as unknown as ComponentFn<TreeViewProps>;
export const TreeItem = "TreeItem" as unknown as ComponentFn<TreeItemProps>;
export const FinancialChart = "FinancialChart" as unknown as ComponentFn<FinancialChartProps>;

export function Spacer(props: SpacerProps) {
  return createElement("Panel", {
    width: props.width,
    height: props.height,
    flexGrow: props.flexGrow,
    flexBasis: props.flexBasis,
    padding: 0,
  });
}

