import { Fragment, type ChildNode, type ComponentFn, type VNode } from "./jsx-runtime.ts";
import {
  EventMask,
  EventType,
  NodeType,
  Prop,
  TAG_TO_NODE_TYPE,
  normalizeThickness,
  parseColor,
  type ThicknessInput,
} from "./protocol.ts";

interface NativeBridge {
  beginBatch(): void;
  create(nodeId: number, typeId: number): void;
  setPropNum(nodeId: number, propId: number, value: number, ackSeq?: number): void;
  setPropBool(nodeId: number, propId: number, value: boolean, ackSeq?: number): void;
  setPropColor(nodeId: number, propId: number, argb: number): void;
  setPropThickness(nodeId: number, propId: number, l: number, t: number, r: number, b: number): void;
  setPropStr(nodeId: number, propId: number, str: string, ackSeq?: number): void;
  setEventMask(nodeId: number, mask: number): void;
  append(parentId: number, childId: number): void;
  insertBefore(parentId: number, childId: number, beforeId: number): void;
  remove(parentId: number, childId: number): void;
  destroy(nodeId: number): void;
  setRoot(nodeId: number): void;
  endBatch(needsLayout: boolean): void;
  setTitle(title: string): void;
  openDemo(layoutPath: string, chrome?: string, size?: string): void;
  windowAction(action: string): void;
  chartSetPoints(nodeId: number, points: any[]): void;
  chartAppendPoint(nodeId: number, point: any, replaceLast: boolean): void;
  chartSetCandles(nodeId: number, candles: any[]): void;
}

declare const __aiNative: NativeBridge;

function nativeApi(): NativeBridge {
  return (globalThis as any).__aiNative;
}

type CleanupFn = (() => void) | void;

interface EffectRecord {
  fn: () => CleanupFn;
  deps: readonly unknown[] | undefined;
  cleanup: CleanupFn;
}

interface ComponentInstance {
  kind: "comp";
  fn: ComponentFn;
  props: Record<string, any>;
  key: string | null;
  matchKey: string;
  hooks: any[];
  effects: (EffectRecord | undefined)[];
  pendingEffects: number[];
  child: MountedNode | null;
  parentHostId: number;
  dirty: boolean;
  unmounted: boolean;
}

interface HostInstance {
  kind: "host";
  nodeId: number;
  nodeType: NodeType;
  tag: string;
  props: Record<string, any>;
  key: string | null;
  matchKey: string;
  children: MountedNode[];
}

interface FragmentInstance {
  kind: "frag";
  key: string | null;
  matchKey: string;
  children: MountedNode[];
}

type MountedNode = ComponentInstance | HostInstance | FragmentInstance;

interface HandlerEntry {
  onClick?: () => void;
  onChange?: (val: any) => void;
  onTextChanged?: (text: string) => void;
  onSubmit?: (text: string) => void;
  onPageChange?: (page: number, pageSize: number) => void;
  onSelect?: (label: string, itemNodeId: number) => void;
  onCheck?: (checked: boolean, label?: string, itemNodeId?: number) => void;
}

const enum PropKind {
  Num = 1,
  Bool = 2,
  Color = 3,
  Thickness = 4,
  Str = 5,
}

interface PropMeta {
  id: Prop;
  kind: PropKind;
  layout?: boolean;
  seqControlled?: boolean;
}

const PROP_META: Record<string, PropMeta> = {
  id: { id: Prop.Name, kind: PropKind.Str },
  width: { id: Prop.Width, kind: PropKind.Num, layout: true },
  height: { id: Prop.Height, kind: PropKind.Num, layout: true },
  minWidth: { id: Prop.MinWidth, kind: PropKind.Num, layout: true },
  maxWidth: { id: Prop.MaxWidth, kind: PropKind.Num, layout: true },
  minHeight: { id: Prop.MinHeight, kind: PropKind.Num, layout: true },
  maxHeight: { id: Prop.MaxHeight, kind: PropKind.Num, layout: true },
  flexGrow: { id: Prop.FlexGrow, kind: PropKind.Num, layout: true },
  flexShrink: { id: Prop.FlexShrink, kind: PropKind.Num, layout: true },
  flexBasis: { id: Prop.FlexBasis, kind: PropKind.Num, layout: true },
  alignSelf: { id: Prop.AlignSelf, kind: PropKind.Str, layout: true },
  margin: { id: Prop.Margin, kind: PropKind.Thickness, layout: true },
  border: { id: Prop.Border, kind: PropKind.Thickness, layout: true },
  disabled: { id: Prop.Disabled, kind: PropKind.Bool },
  tooltip: { id: Prop.Tooltip, kind: PropKind.Str },
  styleRef: { id: Prop.StyleRef, kind: PropKind.Str },
  hitTest: { id: Prop.HitTestRole, kind: PropKind.Str },
  windowChrome: { id: Prop.WindowChrome, kind: PropKind.Str },

  direction: { id: Prop.Direction, kind: PropKind.Str, layout: true },
  alignItems: { id: Prop.AlignItems, kind: PropKind.Str, layout: true },
  justifyContent: { id: Prop.JustifyContent, kind: PropKind.Str, layout: true },
  wrap: { id: Prop.Wrap, kind: PropKind.Str, layout: true },
  padding: { id: Prop.Padding, kind: PropKind.Thickness, layout: true },
  spacing: { id: Prop.Spacing, kind: PropKind.Num, layout: true },
  cornerRadius: { id: Prop.CornerRadius, kind: PropKind.Num },
  background: { id: Prop.Background, kind: PropKind.Color },
  borderColor: { id: Prop.BorderColor, kind: PropKind.Color },
  shadowBlur: { id: Prop.ShadowBlur, kind: PropKind.Num },
  shadowOffsetX: { id: Prop.ShadowOffsetX, kind: PropKind.Num },
  shadowOffsetY: { id: Prop.ShadowOffsetY, kind: PropKind.Num },
  shadowColor: { id: Prop.ShadowColor, kind: PropKind.Color },
  gradientStart: { id: Prop.GradientStart, kind: PropKind.Color },
  gradientEnd: { id: Prop.GradientEnd, kind: PropKind.Color },
  gradientAngle: { id: Prop.GradientAngle, kind: PropKind.Num },

  text: { id: Prop.Text, kind: PropKind.Str, seqControlled: true },
  fontSize: { id: Prop.FontSize, kind: PropKind.Num, layout: true },
  color: { id: Prop.Color, kind: PropKind.Color },
  foreground: { id: Prop.Foreground, kind: PropKind.Color },
  fontWeight: { id: Prop.FontWeight, kind: PropKind.Num },
  bold: { id: Prop.Bold, kind: PropKind.Bool },
  italic: { id: Prop.Italic, kind: PropKind.Bool },
  ellipsis: { id: Prop.Ellipsis, kind: PropKind.Bool },
  horizontalAlign: { id: Prop.HorizontalAlign, kind: PropKind.Str },
  verticalAlign: { id: Prop.VerticalAlign, kind: PropKind.Str },
  placeholder: { id: Prop.Placeholder, kind: PropKind.Str },
  placeholderColor: { id: Prop.PlaceholderColor, kind: PropKind.Color },
  focusBorderColor: { id: Prop.FocusBorderColor, kind: PropKind.Color },

  checked: { id: Prop.Checked, kind: PropKind.Bool, seqControlled: true },
  value: { id: Prop.Value, kind: PropKind.Num, seqControlled: true },
  min: { id: Prop.Min, kind: PropKind.Num },
  max: { id: Prop.Max, kind: PropKind.Num },
  step: { id: Prop.Step, kind: PropKind.Num },
  showValueText: { id: Prop.ShowValueText, kind: PropKind.Bool },
  barHeight: { id: Prop.BarHeight, kind: PropKind.Num },
  trackColor: { id: Prop.TrackColor, kind: PropKind.Color },
  fillColor: { id: Prop.FillColor, kind: PropKind.Color },
  thumbColor: { id: Prop.ThumbColor, kind: PropKind.Color },
  activeColor: { id: Prop.ActiveColor, kind: PropKind.Color },
  inactiveColor: { id: Prop.InactiveColor, kind: PropKind.Color },
  hoverColor: { id: Prop.HoverColor, kind: PropKind.Color },
  highlightColor: { id: Prop.HighlightColor, kind: PropKind.Color },

  variant: { id: Prop.Variant, kind: PropKind.Str },
  dot: { id: Prop.Dot, kind: PropKind.Bool, layout: true },
  icon: { id: Prop.Icon, kind: PropKind.Str },
  size: { id: Prop.Size, kind: PropKind.Num, layout: true },
  circle: { id: Prop.Circle, kind: PropKind.Bool },
  status: { id: Prop.Status, kind: PropKind.Str },
  initials: { id: Prop.Initials, kind: PropKind.Str },
  vertical: { id: Prop.Vertical, kind: PropKind.Bool, layout: true },
  dashed: { id: Prop.Dashed, kind: PropKind.Bool },
  thickness: { id: Prop.ThicknessVal, kind: PropKind.Num, layout: true },
  strokeWidth: { id: Prop.StrokeWidth, kind: PropKind.Num },
  fill: { id: Prop.Fill, kind: PropKind.Color },

  separator: { id: Prop.Separator, kind: PropKind.Str, layout: true },
  separatorColor: { id: Prop.SeparatorColor, kind: PropKind.Color },
  active: { id: Prop.Active, kind: PropKind.Bool },
  href: { id: Prop.Href, kind: PropKind.Str },
  current: { id: Prop.Current, kind: PropKind.Num, seqControlled: true },
  total: { id: Prop.Total, kind: PropKind.Num, layout: true },
  pageSize: { id: Prop.PageSize, kind: PropKind.Num, layout: true },
  showTotal: { id: Prop.ShowTotal, kind: PropKind.Bool, layout: true },
  itemSize: { id: Prop.ItemSize, kind: PropKind.Num, layout: true },
  activeTextColor: { id: Prop.ActiveTextColor, kind: PropKind.Color },
  disabledColor: { id: Prop.DisabledColor, kind: PropKind.Color },
  showLines: { id: Prop.ShowLines, kind: PropKind.Bool },
  showCheckBox: { id: Prop.ShowCheckBox, kind: PropKind.Bool },
  itemHeight: { id: Prop.ItemHeight, kind: PropKind.Num },
  indentWidth: { id: Prop.IndentWidth, kind: PropKind.Num },
  expanded: { id: Prop.Expanded, kind: PropKind.Bool },
  selected: { id: Prop.Selected, kind: PropKind.Bool },
  header: { id: Prop.Header, kind: PropKind.Str },
  selectedBackground: { id: Prop.SelectedBackground, kind: PropKind.Color },
  hoverBackground: { id: Prop.HoverBackground, kind: PropKind.Color },
  lineColor: { id: Prop.LineColor, kind: PropKind.Color },
  glyphColor: { id: Prop.GlyphColor, kind: PropKind.Color },

  scrollX: { id: Prop.ScrollX, kind: PropKind.Bool, layout: true },
  scrollY: { id: Prop.ScrollY, kind: PropKind.Bool, layout: true },
  showScrollbars: { id: Prop.ShowScrollbars, kind: PropKind.Bool },
  barThickness: { id: Prop.BarThickness, kind: PropKind.Num },
  onText: { id: Prop.OnText, kind: PropKind.Str },
  offText: { id: Prop.OffText, kind: PropKind.Str },
  label: { id: Prop.LabelText, kind: PropKind.Str },

  mode: { id: Prop.ChartMode, kind: PropKind.Str },
  preClose: { id: Prop.PreClose, kind: PropKind.Num },
  modernTheme: { id: Prop.ModernTheme, kind: PropKind.Bool },
  showCrosshair: { id: Prop.ShowCrosshair, kind: PropKind.Bool },
  showHeaderInfo: { id: Prop.ShowHeaderInfo, kind: PropKind.Bool },
  symbolName: { id: Prop.SymbolName, kind: PropKind.Str },
  periodLabel: { id: Prop.PeriodLabel, kind: PropKind.Str },

  textColor: { id: Prop.TextColor, kind: PropKind.Color },
  src: { id: Prop.Src, kind: PropKind.Str },
  source: { id: Prop.Src, kind: PropKind.Str },
  stretch: { id: Prop.Variant, kind: PropKind.Str },
};

const TEXT_CHILDREN_TYPES = new Set<NodeType>([
  NodeType.Label,
  NodeType.Button,
  NodeType.Badge,
  NodeType.BreadcrumbItem,
  NodeType.Checkbox,
  NodeType.Divider,
  NodeType.Switch,
]);

let nextNodeId = 1;
let currentComp: ComponentInstance | null = null;
let hookCursor = 0;

let batchNeedsLayout = false;
let batchActive = false;

const dirtyQueue = new Set<ComponentInstance>();
let flushScheduled = false;
let effectsQueue: ComponentInstance[] = [];

const nodeHandlers = new Map<number, HandlerEntry>();
const nodeTypes = new Map<number, NodeType>();
const nodeLastSeq = new Map<number, number>();
const contextStacks = new Map<symbol, any[]>();
const windowKeyListeners = new Set<(keyCode: number, isTextInputFocused: boolean) => void>();
let rootMount: MountedNode | null = null;

function extractInlineText(children: ChildNode): string | undefined {
  if (children === undefined || children === null || typeof children === "boolean") {
    return undefined;
  }
  if (typeof children === "string") return children;
  if (typeof children === "number") return String(children);
  if (Array.isArray(children)) {
    let out = "";
    let hasPrimitive = false;
    for (let i = 0; i < children.length; i++) {
      const part = extractInlineText(children[i]);
      if (part !== undefined) {
        out += part;
        hasPrimitive = true;
      }
    }
    return hasPrimitive ? out : undefined;
  }
  return undefined;
}

function normalizeHostProps(nodeType: NodeType, raw: Record<string, any>): Record<string, any> {
  const out: Record<string, any> = {};
  for (const k in raw) {
    if (k === "key" || k === "children") continue;
    if (nodeType === NodeType.Icon && k === "name") {
      out.icon = raw[k];
    } else if (nodeType === NodeType.TreeItem && k === "header") {
      out.header = raw[k];
    } else {
      out[k] = raw[k];
    }
  }
  if (out.text === undefined && TEXT_CHILDREN_TYPES.has(nodeType) && raw.children !== undefined) {
    const inlineTxt = extractInlineText(raw.children);
    if (inlineTxt !== undefined) {
      out.text = inlineTxt;
    }
  }
  // Ensure deterministic order: apply range (min, max) before value/total/pageSize before current.
  if (out.min !== undefined || out.max !== undefined || out.value !== undefined || out.current !== undefined) {
    const ordered: Record<string, any> = {};
    const priority = ["min", "max", "total", "pageSize", "width", "height"];
    for (const p of priority) {
      if (out[p] !== undefined) ordered[p] = out[p];
    }
    for (const k in out) {
      if (ordered[k] === undefined && k !== "value" && k !== "current") {
        ordered[k] = out[k];
      }
    }
    if (out.value !== undefined) ordered.value = out.value;
    if (out.current !== undefined) ordered.current = out.current;
    return ordered;
  }
  return out;
}

function depsChanged(prev: readonly unknown[] | undefined, next: readonly unknown[] | undefined): boolean {
  if (!prev || !next) return true;
  if (prev.length !== next.length) return true;
  for (let i = 0; i < prev.length; i++) {
    if (!Object.is(prev[i], next[i])) return true;
  }
  return false;
}

function scheduleFlush(): void {
  if (flushScheduled) return;
  flushScheduled = true;
  queueMicrotask(() => {
    flushScheduled = false;
    flushDirty();
  });
}

function runEffectsQueue(): void {
  if (effectsQueue.length === 0) return;
  const list = effectsQueue;
  effectsQueue = [];
  for (let i = 0; i < list.length; i++) {
    const comp = list[i];
    if (comp.unmounted) continue;
    const pending = comp.pendingEffects;
    comp.pendingEffects = [];
    for (let j = 0; j < pending.length; j++) {
      const idx = pending[j];
      const rec = comp.effects[idx];
      if (!rec) continue;
      if (typeof rec.cleanup === "function") {
        rec.cleanup();
      }
      rec.cleanup = rec.fn();
    }
  }
}

function emitSingleProp(nodeId: number, key: string, val: any, ackSeq: number): void {
  const meta = PROP_META[key];
  if (!meta || val === undefined || val === null) return;
  const n = nativeApi();
  if (meta.layout) {
    batchNeedsLayout = true;
  }
  const seq = meta.seqControlled ? ackSeq : 0;
  switch (meta.kind) {
    case PropKind.Num:
      n.setPropNum(nodeId, meta.id, Number(val), seq);
      break;
    case PropKind.Bool:
      n.setPropBool(nodeId, meta.id, Boolean(val), seq);
      break;
    case PropKind.Color: {
      const parsed = parseColor(val);
      if (typeof parsed === "number") {
        n.setPropColor(nodeId, meta.id, parsed);
      } else {
        n.setPropStr(nodeId, meta.id, parsed, 0);
      }
      break;
    }
    case PropKind.Thickness: {
      const [l, t, r, b] = normalizeThickness(val as ThicknessInput);
      n.setPropThickness(nodeId, meta.id, l, t, r, b);
      break;
    }
    case PropKind.Str:
      n.setPropStr(nodeId, meta.id, String(val), seq);
      break;
  }
}

function syncEvents(nodeId: number, prevProps: Record<string, any> | null, nextProps: Record<string, any>): void {
  const maskFor = (p: Record<string, any>): number => {
    let m = 0;
    if (typeof p.onClick === "function") m |= EventMask.Click;
    if (typeof p.onChange === "function") m |= EventMask.Change;
    if (typeof p.onTextChanged === "function") m |= EventMask.TextChanged;
    if (typeof p.onSubmit === "function") m |= EventMask.Submit;
    if (typeof p.onPageChange === "function") m |= EventMask.PageChange;
    if (typeof p.onSelect === "function") m |= EventMask.TreeSelect;
    if (typeof p.onCheck === "function") m |= EventMask.TreeCheck;
    return m;
  };
  const prevMask = prevProps ? maskFor(prevProps) : 0;
  const nextMask = maskFor(nextProps);

  if (nextMask !== 0) {
    nodeHandlers.set(nodeId, {
      onClick: nextProps.onClick,
      onChange: nextProps.onChange,
      onTextChanged: nextProps.onTextChanged,
      onSubmit: nextProps.onSubmit,
      onPageChange: nextProps.onPageChange,
      onSelect: nextProps.onSelect,
      onCheck: nextProps.onCheck,
    });
  } else {
    nodeHandlers.delete(nodeId);
  }

  if (prevProps === null ? nextMask !== 0 : prevMask !== nextMask) {
    nativeApi().setEventMask(nodeId, nextMask);
  }
}

function propValuesEqual(a: any, b: any): boolean {
  if (Object.is(a, b)) return true;
  if (
    a !== null &&
    b !== null &&
    typeof a === "object" &&
    typeof b === "object" &&
    !Array.isArray(a) &&
    !Array.isArray(b)
  ) {
    return (
      a.x === b.x &&
      a.y === b.y &&
      a.left === b.left &&
      a.top === b.top &&
      a.right === b.right &&
      a.bottom === b.bottom
    );
  }
  return false;
}

function syncChartSeriesProps(
  nodeId: number,
  prevProps: Record<string, any> | null,
  nextProps: Record<string, any>
): void {
  const n = nativeApi();
  if (nextProps.points !== undefined && !Object.is(prevProps?.points, nextProps.points)) {
    const prevPts = prevProps?.points as any[] | undefined;
    const nextPts = nextProps.points as any[];
    if (Array.isArray(prevPts) && prevPts.length > 0 && Array.isArray(nextPts) && nextPts.length > 0) {
      const L = prevPts.length;
      if (nextPts.length === L + 1 && nextPts[0] === prevPts[0] && nextPts[L - 1] === prevPts[L - 1]) {
        n.chartAppendPoint(nodeId, nextPts[L], false);
      } else if (
        nextPts.length === L &&
        nextPts[0] === prevPts[0] &&
        (L === 1 || nextPts[L - 2] === prevPts[L - 2])
      ) {
        n.chartAppendPoint(nodeId, nextPts[L - 1], true);
      } else {
        n.chartSetPoints(nodeId, nextPts);
      }
    } else if (Array.isArray(nextPts)) {
      n.chartSetPoints(nodeId, nextPts);
    }
  }
  if (nextProps.candles !== undefined && !Object.is(prevProps?.candles, nextProps.candles)) {
    if (Array.isArray(nextProps.candles)) {
      n.chartSetCandles(nodeId, nextProps.candles);
    }
  }
}

function applyPropsDiff(
  nodeId: number,
  nodeType: NodeType,
  prevProps: Record<string, any> | null,
  nextProps: Record<string, any>
): void {
  const ackSeq = nodeLastSeq.get(nodeId) ?? 0;
  if (!prevProps) {
    for (const k in nextProps) {
      if (PROP_META[k]) {
        emitSingleProp(nodeId, k, nextProps[k], ackSeq);
      }
    }
    if (nodeType === NodeType.FinancialChart) {
      syncChartSeriesProps(nodeId, null, nextProps);
    }
    syncEvents(nodeId, null, nextProps);
    return;
  }

  for (const k in nextProps) {
    if (!PROP_META[k]) continue;
    const pv = prevProps[k];
    const nv = nextProps[k];
    if (!propValuesEqual(pv, nv)) {
      emitSingleProp(nodeId, k, nv, ackSeq);
    }
  }

  // Handle cleared common defaults
  for (const k in prevProps) {
    if (!PROP_META[k] || nextProps[k] !== undefined) continue;
    if (k === "disabled") emitSingleProp(nodeId, k, false, 0);
    else if (k === "tooltip" || k === "text" || k === "placeholder") emitSingleProp(nodeId, k, "", ackSeq);
    else if (k === "active" && nodeType === NodeType.BreadcrumbItem) emitSingleProp(nodeId, k, false, 0);
  }

  if (nodeType === NodeType.FinancialChart) {
    syncChartSeriesProps(nodeId, prevProps, nextProps);
  }

  syncEvents(nodeId, prevProps, nextProps);
}

function flattenVNodes(input: ChildNode, out: { key: string | null; matchKey: string; vnode: VNode }[]): void {
  if (input === null || input === undefined || typeof input === "boolean") {
    return;
  }
  if (Array.isArray(input)) {
    for (let i = 0; i < input.length; i++) {
      flattenVNodes(input[i], out);
    }
    return;
  }
  if (typeof input === "string" || typeof input === "number") {
    out.push({
      key: null,
      matchKey: `pos:text:${out.length}`,
      vnode: {
        $$vnode: true,
        type: "Label",
        props: { text: String(input) },
        key: null,
      },
    });
    return;
  }
  if (input.$$vnode) {
    if (input.type === Fragment) {
      if (input.key !== null) {
        out.push({
          key: input.key,
          matchKey: `k:${input.key}`,
          vnode: input,
        });
      } else {
        flattenVNodes(input.props.children, out);
      }
      return;
    }
    const typeTag =
      typeof input.type === "string"
        ? input.type
        : typeof input.type === "function"
        ? input.type.name || "fn"
        : "sym";
    const mk = input.key !== null ? `k:${input.key}` : `pos:${typeTag}:${out.length}`;
    out.push({
      key: input.key,
      matchKey: mk,
      vnode: input,
    });
  }
}

function collectHostIds(mounted: MountedNode | null, out: number[]): void {
  if (!mounted) return;
  if (mounted.kind === "host") {
    out.push(mounted.nodeId);
  } else if (mounted.kind === "frag") {
    for (let i = 0; i < mounted.children.length; i++) {
      collectHostIds(mounted.children[i], out);
    }
  } else if (mounted.child) {
    collectHostIds(mounted.child, out);
  }
}

function getFirstHostId(mounted: MountedNode | null): number {
  if (!mounted) return 0;
  if (mounted.kind === "host") return mounted.nodeId;
  if (mounted.kind === "frag") {
    for (let i = 0; i < mounted.children.length; i++) {
      const id = getFirstHostId(mounted.children[i]);
      if (id !== 0) return id;
    }
    return 0;
  }
  return getFirstHostId(mounted.child);
}

function unmountTree(mounted: MountedNode | null, detachTopHostFromParentId: number): void {
  if (!mounted) return;
  if (mounted.kind === "host") {
    for (let i = 0; i < mounted.children.length; i++) {
      unmountTree(mounted.children[i], 0);
    }
    if (detachTopHostFromParentId !== 0) {
      nativeApi().remove(detachTopHostFromParentId, mounted.nodeId);
    }
    nativeApi().destroy(mounted.nodeId);
    nodeHandlers.delete(mounted.nodeId);
    nodeTypes.delete(mounted.nodeId);
    nodeLastSeq.delete(mounted.nodeId);
    return;
  }
  if (mounted.kind === "frag") {
    for (let i = 0; i < mounted.children.length; i++) {
      unmountTree(mounted.children[i], detachTopHostFromParentId);
    }
    return;
  }
  mounted.unmounted = true;
  dirtyQueue.delete(mounted);
  for (let i = 0; i < mounted.effects.length; i++) {
    const rec = mounted.effects[i];
    if (rec && typeof rec.cleanup === "function") {
      rec.cleanup();
      rec.cleanup = undefined;
    }
  }
  unmountTree(mounted.child, detachTopHostFromParentId);
}

function runFunctionComponent(comp: ComponentInstance): ChildNode {
  const prevComp = currentComp;
  const prevCursor = hookCursor;
  currentComp = comp;
  hookCursor = 0;
  comp.dirty = false;

  const provId = comp.fn.__providerId;
  let stackPushed = false;
  if (provId) {
    let stack = contextStacks.get(provId);
    if (!stack) {
      stack = [];
      contextStacks.set(provId, stack);
    }
    stack.push(comp.props.value);
    stackPushed = true;
  }

  try {
    return comp.fn(comp.props);
  } finally {
    if (stackPushed && provId) {
      contextStacks.get(provId)!.pop();
    }
    currentComp = prevComp;
    hookCursor = prevCursor;
  }
}

function mountVNode(
  vnode: VNode,
  matchKey: string,
  parentHostId: number,
  insertBeforeHostId: number
): MountedNode {
  if (vnode.type === Fragment) {
    const items: { key: string | null; matchKey: string; vnode: VNode }[] = [];
    flattenVNodes(vnode.props.children, items);
    const children: MountedNode[] = [];
    for (let i = 0; i < items.length; i++) {
      children.push(mountVNode(items[i].vnode, items[i].matchKey, parentHostId, insertBeforeHostId));
    }
    return { kind: "frag", key: vnode.key, matchKey, children };
  }

  if (typeof vnode.type === "function") {
    const inst: ComponentInstance = {
      kind: "comp",
      fn: vnode.type,
      props: vnode.props,
      key: vnode.key,
      matchKey,
      hooks: [],
      effects: [],
      pendingEffects: [],
      child: null,
      parentHostId,
      dirty: false,
      unmounted: false,
    };
    const rendered = runFunctionComponent(inst);
    const flat: { key: string | null; matchKey: string; vnode: VNode }[] = [];
    flattenVNodes(rendered, flat);
    if (flat.length === 1) {
      inst.child = mountVNode(flat[0].vnode, flat[0].matchKey, parentHostId, insertBeforeHostId);
    } else if (flat.length > 1) {
      const fragChildren: MountedNode[] = [];
      for (let i = 0; i < flat.length; i++) {
        fragChildren.push(mountVNode(flat[i].vnode, flat[i].matchKey, parentHostId, insertBeforeHostId));
      }
      inst.child = { kind: "frag", key: null, matchKey: "frag-root", children: fragChildren };
    }
    if (inst.pendingEffects.length > 0) {
      effectsQueue.push(inst);
    }
    return inst;
  }

  const tag = String(vnode.type);
  const nodeType = TAG_TO_NODE_TYPE[tag];
  if (!nodeType) {
    throw new Error(`Unknown JSX element <${tag}>`);
  }
  const nodeId = nextNodeId++;
  const normalized = normalizeHostProps(nodeType, vnode.props);
  nodeTypes.set(nodeId, nodeType);
  nativeApi().create(nodeId, nodeType);
  applyPropsDiff(nodeId, nodeType, null, normalized);

  const hostInst: HostInstance = {
    kind: "host",
    nodeId,
    nodeType,
    tag,
    props: normalized,
    key: vnode.key,
    matchKey,
    children: [],
  };

  if (!TEXT_CHILDREN_TYPES.has(nodeType) && vnode.props.children !== undefined) {
    const flat: { key: string | null; matchKey: string; vnode: VNode }[] = [];
    flattenVNodes(vnode.props.children, flat);
    for (let i = 0; i < flat.length; i++) {
      hostInst.children.push(mountVNode(flat[i].vnode, flat[i].matchKey, nodeId, 0));
    }
  }

  if (parentHostId !== 0) {
    if (insertBeforeHostId !== 0) {
      nativeApi().insertBefore(parentHostId, nodeId, insertBeforeHostId);
    } else {
      nativeApi().append(parentHostId, nodeId);
    }
    batchNeedsLayout = true;
  }

  return hostInst;
}

function canReuse(mounted: MountedNode, vnode: VNode): boolean {
  if (vnode.type === Fragment) return mounted.kind === "frag";
  if (typeof vnode.type === "function") return mounted.kind === "comp" && mounted.fn === vnode.type;
  return mounted.kind === "host" && mounted.tag === vnode.type;
}

function placeSubtree(mounted: MountedNode, parentHostId: number, beforeHostId: number): void {
  const ids: number[] = [];
  collectHostIds(mounted, ids);
  for (let i = 0; i < ids.length; i++) {
    if (beforeHostId !== 0) {
      nativeApi().insertBefore(parentHostId, ids[i], beforeHostId);
    } else {
      nativeApi().append(parentHostId, ids[i]);
    }
    batchNeedsLayout = true;
  }
}

function reconcileChildren(
  parentHostId: number,
  oldChildren: MountedNode[],
  rawNewChildren: ChildNode,
  nextSiblingHostId: number
): MountedNode[] {
  const flat: { key: string | null; matchKey: string; vnode: VNode }[] = [];
  flattenVNodes(rawNewChildren, flat);

  const oldByKey = new Map<string, { index: number; node: MountedNode }>();
  for (let i = 0; i < oldChildren.length; i++) {
    oldByKey.set(oldChildren[i].matchKey, { index: i, node: oldChildren[i] });
  }

  const reused: MountedNode[] = new Array(flat.length);
  const oldIndices: number[] = new Array(flat.length);
  const consumedOld = new Set<number>();

  let lastPlacedIndex = 0;
  const needsMove: boolean[] = new Array(flat.length);

  // First pass: match reusable nodes & create brand new nodes temporarily unplaced (or track)
  const pendingMounts: boolean[] = new Array(flat.length);

  for (let i = 0; i < flat.length; i++) {
    const item = flat[i];
    const match = oldByKey.get(item.matchKey);
    if (match && !consumedOld.has(match.index) && canReuse(match.node, item.vnode)) {
      consumedOld.add(match.index);
      oldIndices[i] = match.index;
      if (match.index < lastPlacedIndex) {
        needsMove[i] = true;
      } else {
        lastPlacedIndex = match.index;
        needsMove[i] = false;
      }
      pendingMounts[i] = false;
      reused[i] = match.node;
    } else {
      oldIndices[i] = -1;
      needsMove[i] = true;
      pendingMounts[i] = true;
    }
  }

  // Unmount unused old children before inserting new ones cleanly
  for (let i = 0; i < oldChildren.length; i++) {
    if (!consumedOld.has(i)) {
      unmountTree(oldChildren[i], parentHostId);
      batchNeedsLayout = true;
    }
  }

  // Update reused nodes & mount new nodes back-to-front so anchor beforeHostId is always known!
  let anchorHostId = nextSiblingHostId;
  for (let i = flat.length - 1; i >= 0; i--) {
    const item = flat[i];
    if (pendingMounts[i]) {
      reused[i] = mountVNode(item.vnode, item.matchKey, parentHostId, anchorHostId);
    } else {
      updateMountedNode(reused[i], item.vnode, parentHostId, anchorHostId);
      if (needsMove[i]) {
        placeSubtree(reused[i], parentHostId, anchorHostId);
      }
    }
    const first = getFirstHostId(reused[i]);
    if (first !== 0) {
      anchorHostId = first;
    }
  }

  return reused;
}

function updateComponentChildren(
  comp: ComponentInstance,
  rendered: ChildNode,
  nextSiblingHostId: number
): void {
  const flat: { key: string | null; matchKey: string; vnode: VNode }[] = [];
  flattenVNodes(rendered, flat);

  if (flat.length === 0) {
    if (comp.child) {
      unmountTree(comp.child, comp.parentHostId);
      comp.child = null;
      batchNeedsLayout = true;
    }
    return;
  }

  if (flat.length === 1 && comp.child && comp.child.kind !== "frag" && canReuse(comp.child, flat[0].vnode)) {
    updateMountedNode(comp.child, flat[0].vnode, comp.parentHostId, nextSiblingHostId);
    return;
  }

  if (comp.child && comp.child.kind === "frag") {
    comp.child.children = reconcileChildren(comp.parentHostId, comp.child.children, rendered, nextSiblingHostId);
    if (comp.child.children.length === 0) {
      comp.child = null;
    }
    return;
  }

  const prevOld = comp.child ? [comp.child] : [];
  const nextList = reconcileChildren(comp.parentHostId, prevOld, rendered, nextSiblingHostId);
  if (nextList.length === 0) {
    comp.child = null;
  } else if (nextList.length === 1) {
    comp.child = nextList[0];
  } else {
    comp.child = { kind: "frag", key: null, matchKey: "frag-root", children: nextList };
  }
}

function updateMountedNode(
  mounted: MountedNode,
  vnode: VNode,
  parentHostId: number,
  nextSiblingHostId: number
): void {
  if (mounted.kind === "host") {
    const nextProps = normalizeHostProps(mounted.nodeType, vnode.props);
    applyPropsDiff(mounted.nodeId, mounted.nodeType, mounted.props, nextProps);
    mounted.props = nextProps;
    if (!TEXT_CHILDREN_TYPES.has(mounted.nodeType)) {
      mounted.children = reconcileChildren(mounted.nodeId, mounted.children, vnode.props.children, 0);
    }
    return;
  }
  if (mounted.kind === "frag") {
    mounted.children = reconcileChildren(parentHostId, mounted.children, vnode.props.children, nextSiblingHostId);
    return;
  }
  mounted.parentHostId = parentHostId;
  mounted.props = vnode.props;
  const rendered = runFunctionComponent(mounted);
  updateComponentChildren(mounted, rendered, nextSiblingHostId);
  if (mounted.pendingEffects.length > 0) {
    effectsQueue.push(mounted);
  }
}

function findNextSiblingHostForComp(
  node: MountedNode | null,
  target: ComponentInstance,
  inheritedNextHostId: number
): { found: boolean; nextHostId: number } {
  if (!node) return { found: false, nextHostId: inheritedNextHostId };
  if (node === target) return { found: true, nextHostId: inheritedNextHostId };
  if (node.kind === "comp") {
    return findNextSiblingHostForComp(node.child, target, inheritedNextHostId);
  }
  const list = node.children;
  let after = node.kind === "host" ? 0 : inheritedNextHostId;
  for (let i = list.length - 1; i >= 0; i--) {
    const res = findNextSiblingHostForComp(list[i], target, after);
    if (res.found) return res;
    const first = getFirstHostId(list[i]);
    if (first !== 0) after = first;
  }
  return { found: false, nextHostId: inheritedNextHostId };
}

function flushDirty(): void {
  while (dirtyQueue.size > 0) {
    const snapshot = Array.from(dirtyQueue);
    dirtyQueue.clear();

    batchActive = true;
    batchNeedsLayout = false;
    nativeApi().beginBatch();

    for (let i = 0; i < snapshot.length; i++) {
      const comp = snapshot[i];
      if (comp.unmounted || !comp.dirty) continue;
      const anchor = findNextSiblingHostForComp(rootMount, comp, 0);
      const rendered = runFunctionComponent(comp);
      updateComponentChildren(comp, rendered, anchor.found ? anchor.nextHostId : 0);
      if (comp.pendingEffects.length > 0) {
        effectsQueue.push(comp);
      }
    }

    batchActive = false;
    nativeApi().endBatch(batchNeedsLayout);
    runEffectsQueue();
  }
}

// ============================================================================
// Public Hooks & React-Compatible API
// ============================================================================

export function useState<T>(initial: T | (() => T)): [T, (next: T | ((prev: T) => T)) => void] {
  const comp = currentComp!;
  const idx = hookCursor++;
  if (idx >= comp.hooks.length) {
    const val = typeof initial === "function" ? (initial as () => T)() : initial;
    comp.hooks.push(val);
  }
  const setState = (next: T | ((prev: T) => T)) => {
    if (comp.unmounted) return;
    const prevVal = comp.hooks[idx];
    const resolved = typeof next === "function" ? (next as (prev: T) => T)(prevVal) : next;
    if (!Object.is(prevVal, resolved)) {
      comp.hooks[idx] = resolved;
      comp.dirty = true;
      dirtyQueue.add(comp);
      scheduleFlush();
    }
  };
  return [comp.hooks[idx], setState];
}

export function useReducer<S, A>(
  reducer: (state: S, action: A) => S,
  initialArg: S,
  init?: (arg: S) => S
): [S, (action: A) => void] {
  const [state, setState] = useState<S>(() => (init ? init(initialArg) : initialArg));
  const dispatch = useCallback((action: A) => {
    setState((prev) => reducer(prev, action));
  }, []);
  return [state, dispatch];
}

export function useRef<T>(initial: T): { current: T } {
  const comp = currentComp!;
  const idx = hookCursor++;
  if (idx >= comp.hooks.length) {
    comp.hooks.push({ current: initial });
  }
  return comp.hooks[idx];
}

export function useMemo<T>(factory: () => T, deps: readonly unknown[]): T {
  const comp = currentComp!;
  const idx = hookCursor++;
  if (idx >= comp.hooks.length) {
    const val = factory();
    comp.hooks.push({ val, deps });
    return val;
  }
  const slot = comp.hooks[idx] as { val: T; deps: readonly unknown[] };
  if (depsChanged(slot.deps, deps)) {
    slot.val = factory();
    slot.deps = deps;
  }
  return slot.val;
}

export function useCallback<T extends (...args: any[]) => any>(fn: T, deps: readonly unknown[]): T {
  return useMemo(() => fn, deps);
}

export function useEffect(fn: () => CleanupFn, deps?: readonly unknown[]): void {
  const comp = currentComp!;
  const idx = hookCursor++;
  if (idx >= comp.hooks.length) {
    const rec: EffectRecord = { fn, deps, cleanup: undefined };
    comp.hooks.push(rec);
    comp.effects[idx] = rec;
    comp.pendingEffects.push(idx);
    return;
  }
  const rec = comp.hooks[idx] as EffectRecord;
  if (deps === undefined || depsChanged(rec.deps, deps)) {
    rec.fn = fn;
    rec.deps = deps;
    comp.effects[idx] = rec;
    comp.pendingEffects.push(idx);
  }
}

export const useLayoutEffect = useEffect;

export interface Context<T> {
  Provider: ComponentFn<{ value: T; children?: ChildNode }>;
  _id: symbol;
  _defaultValue: T;
}

export function createContext<T>(defaultValue: T): Context<T> {
  const id = Symbol("Context");
  const Provider: ComponentFn<{ value: T; children?: ChildNode }> = (props) => props.children;
  Provider.__providerId = id;
  return { Provider, _id: id, _defaultValue: defaultValue };
}

export function useContext<T>(ctx: Context<T>): T {
  const stack = contextStacks.get(ctx._id);
  if (stack && stack.length > 0) {
    return stack[stack.length - 1] as T;
  }
  return ctx._defaultValue;
}

export function render(element: VNode): void {
  const n = nativeApi();
  if (rootMount) {
    unmountTree(rootMount, 0);
    rootMount = null;
  }
  batchNeedsLayout = true;
  batchActive = true;
  n.beginBatch();

  const mounted = mountVNode(element, "root", 0, 0);
  rootMount = mounted;
  const rootId = getFirstHostId(mounted);
  if (rootId !== 0) {
    n.setRoot(rootId);
  }

  batchActive = false;
  n.endBatch(true);
  runEffectsQueue();
}

export function setWindowTitle(title: string): void {
  nativeApi().setTitle(title);
}

export function openDemo(layoutPath: string, chrome = "", size = ""): void {
  nativeApi().openDemo(layoutPath, chrome, size);
}

export function windowAction(action: "minimize" | "maximize" | "close"): void {
  nativeApi().windowAction(action);
}

export function useWindowKeyDown(handler: (keyCode: number, isTextInputFocused: boolean) => void): void {
  const fnRef = useRef(handler);
  fnRef.current = handler;
  useEffect(() => {
    const listener = (keyCode: number, isTextInputFocused: boolean) => {
      fnRef.current(keyCode, isTextInputFocused);
    };
    windowKeyListeners.add(listener);
    return () => {
      windowKeyListeners.delete(listener);
    };
  }, []);
}

// Single global event entry point called from C++ QuickJS worker thread.
(globalThis as any).__dispatchEvent = function __dispatchEvent(
  nodeId: number,
  eventType: number,
  eventSeq: number,
  payload: { bool: boolean; int: number; int2: number; num: number; str: string }
): void {
  if (eventSeq > 0) {
    nodeLastSeq.set(nodeId, eventSeq);
  }
  const h = nodeHandlers.get(nodeId);
  switch (eventType) {
    case EventType.Click:
      h?.onClick?.();
      break;
    case EventType.TextChanged:
      h?.onTextChanged?.(payload.str);
      break;
    case EventType.Submit:
      h?.onSubmit?.(payload.str);
      break;
    case EventType.Change:
      if (h?.onChange) {
        if (nodeTypes.get(nodeId) === NodeType.Slider) {
          h.onChange(payload.num);
        } else {
          h.onChange(payload.bool);
        }
      }
      break;
    case EventType.PageChange:
      h?.onPageChange?.(payload.int, payload.int2);
      break;
    case EventType.TreeSelect:
      h?.onSelect?.(payload.str, payload.int);
      if (payload.int > 0) {
        nodeHandlers.get(payload.int)?.onSelect?.(payload.str, payload.int);
      }
      break;
    case EventType.TreeCheck:
      h?.onCheck?.(payload.bool, payload.str, payload.int);
      if (payload.int > 0) {
        nodeHandlers.get(payload.int)?.onCheck?.(payload.bool, payload.str, payload.int);
      }
      break;
    case EventType.KeyDown:
      windowKeyListeners.forEach((fn) => fn(payload.int, payload.bool));
      break;
  }
};
