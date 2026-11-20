(() => {
  var __defProp = Object.defineProperty;
  var __getOwnPropNames = Object.getOwnPropertyNames;
  var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
  var __hasOwnProp = Object.prototype.hasOwnProperty;
  var __moduleCache = /* @__PURE__ */ new WeakMap;
  var __toCommonJS = (from) => {
    var entry = __moduleCache.get(from), desc;
    if (entry)
      return entry;
    entry = __defProp({}, "__esModule", { value: true });
    if (from && typeof from === "object" || typeof from === "function")
      __getOwnPropNames(from).map((key) => !__hasOwnProp.call(entry, key) && __defProp(entry, key, {
        get: () => from[key],
        enumerable: !(desc = __getOwnPropDesc(from, key)) || desc.enumerable
      }));
    __moduleCache.set(from, entry);
    return entry;
  };
  var __export = (target, all) => {
    for (var name in all)
      __defProp(target, name, {
        get: all[name],
        enumerable: true,
        configurable: true,
        set: (newValue) => all[name] = () => newValue
      });
  };

  // react/demos/stock_terminal/index.tsx
  var exports_stock_terminal = {};
  __export(exports_stock_terminal, {
    StockTerminalApp: () => StockTerminalApp
  });

  // react/runtime/jsx-runtime.ts
  var Fragment = Symbol.for("ai-win-ui.Fragment");
  function jsx(type, props, key) {
    const safeProps = props ?? {};
    const resolvedKey = key !== undefined && key !== null ? String(key) : safeProps.key !== undefined && safeProps.key !== null ? String(safeProps.key) : null;
    return {
      $$vnode: true,
      type,
      props: safeProps,
      key: resolvedKey
    };
  }
  var jsxDEV = jsx;
  function createElement(type, props, ...children) {
    const nextProps = props ? { ...props } : {};
    if (children.length === 1) {
      nextProps.children = children[0];
    } else if (children.length > 1) {
      nextProps.children = children;
    }
    return jsx(type, nextProps, nextProps.key);
  }
  // react/runtime/components.ts
  var Panel = "Panel";
  var Label = "Label";
  var Button = "Button";
  var TextInput = "TextInput";
  var FinancialChart = "FinancialChart";
  function Spacer(props) {
    return createElement("Panel", {
      width: props.width,
      height: props.height,
      flexGrow: props.flexGrow,
      flexBasis: props.flexBasis,
      padding: 0
    });
  }
  // react/runtime/protocol.ts
  var TAG_TO_NODE_TYPE = {
    Panel: 1 /* Panel */,
    ScrollViewer: 2 /* ScrollViewer */,
    Label: 3 /* Label */,
    Button: 4 /* Button */,
    TextInput: 5 /* TextInput */,
    Switch: 6 /* Switch */,
    Slider: 7 /* Slider */,
    Checkbox: 8 /* Checkbox */,
    ProgressBar: 9 /* ProgressBar */,
    Badge: 10 /* Badge */,
    Avatar: 11 /* Avatar */,
    Divider: 12 /* Divider */,
    Icon: 13 /* Icon */,
    Breadcrumb: 14 /* Breadcrumb */,
    BreadcrumbItem: 15 /* BreadcrumbItem */,
    Pagination: 16 /* Pagination */,
    TreeView: 17 /* TreeView */,
    TreeItem: 18 /* TreeItem */,
    FinancialChart: 19 /* FinancialChart */
  };
  function parseColor(value) {
    if (typeof value === "number") {
      return value >>> 0;
    }
    const s = value.trim();
    if (s.startsWith("$color.")) {
      return s;
    }
    const lower = s.toLowerCase();
    if (lower === "transparent" || lower === "none") {
      return 0;
    }
    if (s[0] === "#") {
      const hex = s.slice(1);
      if (hex.length === 3) {
        const r = parseInt(hex[0] + hex[0], 16);
        const g = parseInt(hex[1] + hex[1], 16);
        const b = parseInt(hex[2] + hex[2], 16);
        return (255 << 24 | r << 16 | g << 8 | b) >>> 0;
      }
      if (hex.length === 4) {
        const r = parseInt(hex[0] + hex[0], 16);
        const g = parseInt(hex[1] + hex[1], 16);
        const b = parseInt(hex[2] + hex[2], 16);
        const a = parseInt(hex[3] + hex[3], 16);
        return (a << 24 | r << 16 | g << 8 | b) >>> 0;
      }
      if (hex.length === 6) {
        const rgb = parseInt(hex, 16) >>> 0;
        return (4278190080 | rgb) >>> 0;
      }
      if (hex.length === 8) {
        const r = parseInt(hex.slice(0, 2), 16);
        const g = parseInt(hex.slice(2, 4), 16);
        const b = parseInt(hex.slice(4, 6), 16);
        const a = parseInt(hex.slice(6, 8), 16);
        return (a << 24 | r << 16 | g << 8 | b) >>> 0;
      }
    }
    return s;
  }
  function normalizeThickness(v) {
    if (typeof v === "number") {
      return [v, v, v, v];
    }
    const hx = v.x ?? 0;
    const hy = v.y ?? 0;
    return [v.left ?? hx, v.top ?? hy, v.right ?? hx, v.bottom ?? hy];
  }

  // react/runtime/reconciler.ts
  function nativeApi() {
    return globalThis.__aiNative;
  }
  var PROP_META = {
    id: { id: 1 /* Name */, kind: 5 /* Str */ },
    width: { id: 2 /* Width */, kind: 1 /* Num */, layout: true },
    height: { id: 3 /* Height */, kind: 1 /* Num */, layout: true },
    minWidth: { id: 4 /* MinWidth */, kind: 1 /* Num */, layout: true },
    maxWidth: { id: 5 /* MaxWidth */, kind: 1 /* Num */, layout: true },
    minHeight: { id: 6 /* MinHeight */, kind: 1 /* Num */, layout: true },
    maxHeight: { id: 7 /* MaxHeight */, kind: 1 /* Num */, layout: true },
    flexGrow: { id: 8 /* FlexGrow */, kind: 1 /* Num */, layout: true },
    flexShrink: { id: 9 /* FlexShrink */, kind: 1 /* Num */, layout: true },
    flexBasis: { id: 10 /* FlexBasis */, kind: 1 /* Num */, layout: true },
    alignSelf: { id: 11 /* AlignSelf */, kind: 5 /* Str */, layout: true },
    margin: { id: 12 /* Margin */, kind: 4 /* Thickness */, layout: true },
    border: { id: 13 /* Border */, kind: 4 /* Thickness */, layout: true },
    disabled: { id: 14 /* Disabled */, kind: 2 /* Bool */ },
    tooltip: { id: 15 /* Tooltip */, kind: 5 /* Str */ },
    styleRef: { id: 16 /* StyleRef */, kind: 5 /* Str */ },
    hitTest: { id: 17 /* HitTestRole */, kind: 5 /* Str */ },
    direction: { id: 20 /* Direction */, kind: 5 /* Str */, layout: true },
    alignItems: { id: 21 /* AlignItems */, kind: 5 /* Str */, layout: true },
    justifyContent: { id: 22 /* JustifyContent */, kind: 5 /* Str */, layout: true },
    wrap: { id: 23 /* Wrap */, kind: 5 /* Str */, layout: true },
    padding: { id: 24 /* Padding */, kind: 4 /* Thickness */, layout: true },
    spacing: { id: 25 /* Spacing */, kind: 1 /* Num */, layout: true },
    cornerRadius: { id: 26 /* CornerRadius */, kind: 1 /* Num */ },
    background: { id: 27 /* Background */, kind: 3 /* Color */ },
    borderColor: { id: 28 /* BorderColor */, kind: 3 /* Color */ },
    shadowBlur: { id: 29 /* ShadowBlur */, kind: 1 /* Num */ },
    shadowOffsetX: { id: 30 /* ShadowOffsetX */, kind: 1 /* Num */ },
    shadowOffsetY: { id: 31 /* ShadowOffsetY */, kind: 1 /* Num */ },
    shadowColor: { id: 32 /* ShadowColor */, kind: 3 /* Color */ },
    gradientStart: { id: 33 /* GradientStart */, kind: 3 /* Color */ },
    gradientEnd: { id: 34 /* GradientEnd */, kind: 3 /* Color */ },
    gradientAngle: { id: 35 /* GradientAngle */, kind: 1 /* Num */ },
    text: { id: 40 /* Text */, kind: 5 /* Str */, seqControlled: true },
    fontSize: { id: 41 /* FontSize */, kind: 1 /* Num */, layout: true },
    color: { id: 42 /* Color */, kind: 3 /* Color */ },
    foreground: { id: 43 /* Foreground */, kind: 3 /* Color */ },
    fontWeight: { id: 44 /* FontWeight */, kind: 1 /* Num */ },
    bold: { id: 45 /* Bold */, kind: 2 /* Bool */ },
    italic: { id: 46 /* Italic */, kind: 2 /* Bool */ },
    ellipsis: { id: 47 /* Ellipsis */, kind: 2 /* Bool */ },
    horizontalAlign: { id: 48 /* HorizontalAlign */, kind: 5 /* Str */ },
    verticalAlign: { id: 49 /* VerticalAlign */, kind: 5 /* Str */ },
    placeholder: { id: 50 /* Placeholder */, kind: 5 /* Str */ },
    placeholderColor: { id: 51 /* PlaceholderColor */, kind: 3 /* Color */ },
    focusBorderColor: { id: 52 /* FocusBorderColor */, kind: 3 /* Color */ },
    checked: { id: 60 /* Checked */, kind: 2 /* Bool */, seqControlled: true },
    value: { id: 61 /* Value */, kind: 1 /* Num */, seqControlled: true },
    min: { id: 62 /* Min */, kind: 1 /* Num */ },
    max: { id: 63 /* Max */, kind: 1 /* Num */ },
    step: { id: 64 /* Step */, kind: 1 /* Num */ },
    showValueText: { id: 65 /* ShowValueText */, kind: 2 /* Bool */ },
    barHeight: { id: 66 /* BarHeight */, kind: 1 /* Num */ },
    trackColor: { id: 67 /* TrackColor */, kind: 3 /* Color */ },
    fillColor: { id: 68 /* FillColor */, kind: 3 /* Color */ },
    thumbColor: { id: 69 /* ThumbColor */, kind: 3 /* Color */ },
    activeColor: { id: 70 /* ActiveColor */, kind: 3 /* Color */ },
    inactiveColor: { id: 71 /* InactiveColor */, kind: 3 /* Color */ },
    hoverColor: { id: 72 /* HoverColor */, kind: 3 /* Color */ },
    highlightColor: { id: 73 /* HighlightColor */, kind: 3 /* Color */ },
    variant: { id: 80 /* Variant */, kind: 5 /* Str */ },
    dot: { id: 81 /* Dot */, kind: 2 /* Bool */, layout: true },
    icon: { id: 82 /* Icon */, kind: 5 /* Str */ },
    size: { id: 83 /* Size */, kind: 1 /* Num */, layout: true },
    circle: { id: 84 /* Circle */, kind: 2 /* Bool */ },
    status: { id: 85 /* Status */, kind: 5 /* Str */ },
    initials: { id: 86 /* Initials */, kind: 5 /* Str */ },
    vertical: { id: 87 /* Vertical */, kind: 2 /* Bool */, layout: true },
    dashed: { id: 88 /* Dashed */, kind: 2 /* Bool */ },
    thickness: { id: 89 /* ThicknessVal */, kind: 1 /* Num */, layout: true },
    strokeWidth: { id: 90 /* StrokeWidth */, kind: 1 /* Num */ },
    fill: { id: 91 /* Fill */, kind: 3 /* Color */ },
    separator: { id: 100 /* Separator */, kind: 5 /* Str */, layout: true },
    separatorColor: { id: 101 /* SeparatorColor */, kind: 3 /* Color */ },
    active: { id: 102 /* Active */, kind: 2 /* Bool */ },
    href: { id: 103 /* Href */, kind: 5 /* Str */ },
    current: { id: 104 /* Current */, kind: 1 /* Num */, seqControlled: true },
    total: { id: 105 /* Total */, kind: 1 /* Num */, layout: true },
    pageSize: { id: 106 /* PageSize */, kind: 1 /* Num */, layout: true },
    showTotal: { id: 107 /* ShowTotal */, kind: 2 /* Bool */, layout: true },
    itemSize: { id: 108 /* ItemSize */, kind: 1 /* Num */, layout: true },
    activeTextColor: { id: 109 /* ActiveTextColor */, kind: 3 /* Color */ },
    disabledColor: { id: 110 /* DisabledColor */, kind: 3 /* Color */ },
    showLines: { id: 111 /* ShowLines */, kind: 2 /* Bool */ },
    showCheckBox: { id: 112 /* ShowCheckBox */, kind: 2 /* Bool */ },
    itemHeight: { id: 113 /* ItemHeight */, kind: 1 /* Num */ },
    indentWidth: { id: 114 /* IndentWidth */, kind: 1 /* Num */ },
    expanded: { id: 115 /* Expanded */, kind: 2 /* Bool */ },
    selected: { id: 116 /* Selected */, kind: 2 /* Bool */ },
    header: { id: 117 /* Header */, kind: 5 /* Str */ },
    selectedBackground: { id: 118 /* SelectedBackground */, kind: 3 /* Color */ },
    hoverBackground: { id: 119 /* HoverBackground */, kind: 3 /* Color */ },
    lineColor: { id: 120 /* LineColor */, kind: 3 /* Color */ },
    glyphColor: { id: 121 /* GlyphColor */, kind: 3 /* Color */ },
    scrollX: { id: 130 /* ScrollX */, kind: 2 /* Bool */, layout: true },
    scrollY: { id: 131 /* ScrollY */, kind: 2 /* Bool */, layout: true },
    showScrollbars: { id: 132 /* ShowScrollbars */, kind: 2 /* Bool */ },
    barThickness: { id: 133 /* BarThickness */, kind: 1 /* Num */ },
    onText: { id: 134 /* OnText */, kind: 5 /* Str */ },
    offText: { id: 135 /* OffText */, kind: 5 /* Str */ },
    label: { id: 136 /* LabelText */, kind: 5 /* Str */ },
    mode: { id: 140 /* ChartMode */, kind: 5 /* Str */ },
    preClose: { id: 141 /* PreClose */, kind: 1 /* Num */ },
    modernTheme: { id: 142 /* ModernTheme */, kind: 2 /* Bool */ },
    showCrosshair: { id: 143 /* ShowCrosshair */, kind: 2 /* Bool */ },
    showHeaderInfo: { id: 144 /* ShowHeaderInfo */, kind: 2 /* Bool */ },
    symbolName: { id: 145 /* SymbolName */, kind: 5 /* Str */ },
    periodLabel: { id: 146 /* PeriodLabel */, kind: 5 /* Str */ }
  };
  var TEXT_CHILDREN_TYPES = new Set([
    3 /* Label */,
    4 /* Button */,
    10 /* Badge */,
    15 /* BreadcrumbItem */,
    8 /* Checkbox */,
    12 /* Divider */,
    6 /* Switch */
  ]);
  var nextNodeId = 1;
  var currentComp = null;
  var hookCursor = 0;
  var batchNeedsLayout = false;
  var batchActive = false;
  var dirtyQueue = new Set;
  var flushScheduled = false;
  var effectsQueue = [];
  var nodeHandlers = new Map;
  var nodeTypes = new Map;
  var nodeLastSeq = new Map;
  var contextStacks = new Map;
  var windowKeyListeners = new Set;
  var rootMount = null;
  function extractInlineText(children) {
    if (children === undefined || children === null || typeof children === "boolean") {
      return;
    }
    if (typeof children === "string")
      return children;
    if (typeof children === "number")
      return String(children);
    if (Array.isArray(children)) {
      let out = "";
      let hasPrimitive = false;
      for (let i = 0;i < children.length; i++) {
        const part = extractInlineText(children[i]);
        if (part !== undefined) {
          out += part;
          hasPrimitive = true;
        }
      }
      return hasPrimitive ? out : undefined;
    }
    return;
  }
  function normalizeHostProps(nodeType, raw) {
    const out = {};
    for (const k in raw) {
      if (k === "key" || k === "children")
        continue;
      if (nodeType === 13 /* Icon */ && k === "name") {
        out.icon = raw[k];
      } else if (nodeType === 18 /* TreeItem */ && k === "header") {
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
    if (out.min !== undefined || out.max !== undefined || out.value !== undefined || out.current !== undefined) {
      const ordered = {};
      const priority = ["min", "max", "total", "pageSize", "width", "height"];
      for (const p of priority) {
        if (out[p] !== undefined)
          ordered[p] = out[p];
      }
      for (const k in out) {
        if (ordered[k] === undefined && k !== "value" && k !== "current") {
          ordered[k] = out[k];
        }
      }
      if (out.value !== undefined)
        ordered.value = out.value;
      if (out.current !== undefined)
        ordered.current = out.current;
      return ordered;
    }
    return out;
  }
  function depsChanged(prev, next) {
    if (!prev || !next)
      return true;
    if (prev.length !== next.length)
      return true;
    for (let i = 0;i < prev.length; i++) {
      if (!Object.is(prev[i], next[i]))
        return true;
    }
    return false;
  }
  function scheduleFlush() {
    if (flushScheduled)
      return;
    flushScheduled = true;
    queueMicrotask(() => {
      flushScheduled = false;
      flushDirty();
    });
  }
  function runEffectsQueue() {
    if (effectsQueue.length === 0)
      return;
    const list = effectsQueue;
    effectsQueue = [];
    for (let i = 0;i < list.length; i++) {
      const comp = list[i];
      if (comp.unmounted)
        continue;
      const pending = comp.pendingEffects;
      comp.pendingEffects = [];
      for (let j = 0;j < pending.length; j++) {
        const idx = pending[j];
        const rec = comp.effects[idx];
        if (!rec)
          continue;
        if (typeof rec.cleanup === "function") {
          rec.cleanup();
        }
        rec.cleanup = rec.fn();
      }
    }
  }
  function emitSingleProp(nodeId, key, val, ackSeq) {
    const meta = PROP_META[key];
    if (!meta || val === undefined || val === null)
      return;
    const n = nativeApi();
    if (meta.layout) {
      batchNeedsLayout = true;
    }
    const seq = meta.seqControlled ? ackSeq : 0;
    switch (meta.kind) {
      case 1 /* Num */:
        n.setPropNum(nodeId, meta.id, Number(val), seq);
        break;
      case 2 /* Bool */:
        n.setPropBool(nodeId, meta.id, Boolean(val), seq);
        break;
      case 3 /* Color */: {
        const parsed = parseColor(val);
        if (typeof parsed === "number") {
          n.setPropColor(nodeId, meta.id, parsed);
        } else {
          n.setPropStr(nodeId, meta.id, parsed, 0);
        }
        break;
      }
      case 4 /* Thickness */: {
        const [l, t, r, b] = normalizeThickness(val);
        n.setPropThickness(nodeId, meta.id, l, t, r, b);
        break;
      }
      case 5 /* Str */:
        n.setPropStr(nodeId, meta.id, String(val), seq);
        break;
    }
  }
  function syncEvents(nodeId, prevProps, nextProps) {
    const maskFor = (p) => {
      let m = 0;
      if (typeof p.onClick === "function")
        m |= 1 /* Click */;
      if (typeof p.onChange === "function")
        m |= 2 /* Change */;
      if (typeof p.onTextChanged === "function")
        m |= 4 /* TextChanged */;
      if (typeof p.onSubmit === "function")
        m |= 8 /* Submit */;
      if (typeof p.onPageChange === "function")
        m |= 16 /* PageChange */;
      if (typeof p.onSelect === "function")
        m |= 32 /* TreeSelect */;
      if (typeof p.onCheck === "function")
        m |= 64 /* TreeCheck */;
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
        onCheck: nextProps.onCheck
      });
    } else {
      nodeHandlers.delete(nodeId);
    }
    if (prevProps === null ? nextMask !== 0 : prevMask !== nextMask) {
      nativeApi().setEventMask(nodeId, nextMask);
    }
  }
  function propValuesEqual(a, b) {
    if (Object.is(a, b))
      return true;
    if (a !== null && b !== null && typeof a === "object" && typeof b === "object" && !Array.isArray(a) && !Array.isArray(b)) {
      return a.x === b.x && a.y === b.y && a.left === b.left && a.top === b.top && a.right === b.right && a.bottom === b.bottom;
    }
    return false;
  }
  function syncChartSeriesProps(nodeId, prevProps, nextProps) {
    const n = nativeApi();
    if (nextProps.points !== undefined && !Object.is(prevProps?.points, nextProps.points)) {
      const prevPts = prevProps?.points;
      const nextPts = nextProps.points;
      if (Array.isArray(prevPts) && prevPts.length > 0 && Array.isArray(nextPts) && nextPts.length > 0) {
        const L = prevPts.length;
        if (nextPts.length === L + 1 && nextPts[0] === prevPts[0] && nextPts[L - 1] === prevPts[L - 1]) {
          n.chartAppendPoint(nodeId, nextPts[L], false);
        } else if (nextPts.length === L && nextPts[0] === prevPts[0] && (L === 1 || nextPts[L - 2] === prevPts[L - 2])) {
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
  function applyPropsDiff(nodeId, nodeType, prevProps, nextProps) {
    const ackSeq = nodeLastSeq.get(nodeId) ?? 0;
    if (!prevProps) {
      for (const k in nextProps) {
        if (PROP_META[k]) {
          emitSingleProp(nodeId, k, nextProps[k], ackSeq);
        }
      }
      if (nodeType === 19 /* FinancialChart */) {
        syncChartSeriesProps(nodeId, null, nextProps);
      }
      syncEvents(nodeId, null, nextProps);
      return;
    }
    for (const k in nextProps) {
      if (!PROP_META[k])
        continue;
      const pv = prevProps[k];
      const nv = nextProps[k];
      if (!propValuesEqual(pv, nv)) {
        emitSingleProp(nodeId, k, nv, ackSeq);
      }
    }
    for (const k in prevProps) {
      if (!PROP_META[k] || nextProps[k] !== undefined)
        continue;
      if (k === "disabled")
        emitSingleProp(nodeId, k, false, 0);
      else if (k === "tooltip" || k === "text" || k === "placeholder")
        emitSingleProp(nodeId, k, "", ackSeq);
      else if (k === "active" && nodeType === 15 /* BreadcrumbItem */)
        emitSingleProp(nodeId, k, false, 0);
    }
    if (nodeType === 19 /* FinancialChart */) {
      syncChartSeriesProps(nodeId, prevProps, nextProps);
    }
    syncEvents(nodeId, prevProps, nextProps);
  }
  function flattenVNodes(input, out) {
    if (input === null || input === undefined || typeof input === "boolean") {
      return;
    }
    if (Array.isArray(input)) {
      for (let i = 0;i < input.length; i++) {
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
          key: null
        }
      });
      return;
    }
    if (input.$$vnode) {
      if (input.type === Fragment) {
        if (input.key !== null) {
          out.push({
            key: input.key,
            matchKey: `k:${input.key}`,
            vnode: input
          });
        } else {
          flattenVNodes(input.props.children, out);
        }
        return;
      }
      const typeTag = typeof input.type === "string" ? input.type : typeof input.type === "function" ? input.type.name || "fn" : "sym";
      const mk = input.key !== null ? `k:${input.key}` : `pos:${typeTag}:${out.length}`;
      out.push({
        key: input.key,
        matchKey: mk,
        vnode: input
      });
    }
  }
  function collectHostIds(mounted, out) {
    if (!mounted)
      return;
    if (mounted.kind === "host") {
      out.push(mounted.nodeId);
    } else if (mounted.kind === "frag") {
      for (let i = 0;i < mounted.children.length; i++) {
        collectHostIds(mounted.children[i], out);
      }
    } else if (mounted.child) {
      collectHostIds(mounted.child, out);
    }
  }
  function getFirstHostId(mounted) {
    if (!mounted)
      return 0;
    if (mounted.kind === "host")
      return mounted.nodeId;
    if (mounted.kind === "frag") {
      for (let i = 0;i < mounted.children.length; i++) {
        const id = getFirstHostId(mounted.children[i]);
        if (id !== 0)
          return id;
      }
      return 0;
    }
    return getFirstHostId(mounted.child);
  }
  function unmountTree(mounted, detachTopHostFromParentId) {
    if (!mounted)
      return;
    if (mounted.kind === "host") {
      for (let i = 0;i < mounted.children.length; i++) {
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
      for (let i = 0;i < mounted.children.length; i++) {
        unmountTree(mounted.children[i], detachTopHostFromParentId);
      }
      return;
    }
    mounted.unmounted = true;
    dirtyQueue.delete(mounted);
    for (let i = 0;i < mounted.effects.length; i++) {
      const rec = mounted.effects[i];
      if (rec && typeof rec.cleanup === "function") {
        rec.cleanup();
        rec.cleanup = undefined;
      }
    }
    unmountTree(mounted.child, detachTopHostFromParentId);
  }
  function runFunctionComponent(comp) {
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
        contextStacks.get(provId).pop();
      }
      currentComp = prevComp;
      hookCursor = prevCursor;
    }
  }
  function mountVNode(vnode, matchKey, parentHostId, insertBeforeHostId) {
    if (vnode.type === Fragment) {
      const items = [];
      flattenVNodes(vnode.props.children, items);
      const children = [];
      for (let i = 0;i < items.length; i++) {
        children.push(mountVNode(items[i].vnode, items[i].matchKey, parentHostId, insertBeforeHostId));
      }
      return { kind: "frag", key: vnode.key, matchKey, children };
    }
    if (typeof vnode.type === "function") {
      const inst = {
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
        unmounted: false
      };
      const rendered = runFunctionComponent(inst);
      const flat = [];
      flattenVNodes(rendered, flat);
      if (flat.length === 1) {
        inst.child = mountVNode(flat[0].vnode, flat[0].matchKey, parentHostId, insertBeforeHostId);
      } else if (flat.length > 1) {
        const fragChildren = [];
        for (let i = 0;i < flat.length; i++) {
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
    const hostInst = {
      kind: "host",
      nodeId,
      nodeType,
      tag,
      props: normalized,
      key: vnode.key,
      matchKey,
      children: []
    };
    if (!TEXT_CHILDREN_TYPES.has(nodeType) && vnode.props.children !== undefined) {
      const flat = [];
      flattenVNodes(vnode.props.children, flat);
      for (let i = 0;i < flat.length; i++) {
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
  function canReuse(mounted, vnode) {
    if (vnode.type === Fragment)
      return mounted.kind === "frag";
    if (typeof vnode.type === "function")
      return mounted.kind === "comp" && mounted.fn === vnode.type;
    return mounted.kind === "host" && mounted.tag === vnode.type;
  }
  function placeSubtree(mounted, parentHostId, beforeHostId) {
    const ids = [];
    collectHostIds(mounted, ids);
    for (let i = 0;i < ids.length; i++) {
      if (beforeHostId !== 0) {
        nativeApi().insertBefore(parentHostId, ids[i], beforeHostId);
      } else {
        nativeApi().append(parentHostId, ids[i]);
      }
      batchNeedsLayout = true;
    }
  }
  function reconcileChildren(parentHostId, oldChildren, rawNewChildren, nextSiblingHostId) {
    const flat = [];
    flattenVNodes(rawNewChildren, flat);
    const oldByKey = new Map;
    for (let i = 0;i < oldChildren.length; i++) {
      oldByKey.set(oldChildren[i].matchKey, { index: i, node: oldChildren[i] });
    }
    const reused = new Array(flat.length);
    const oldIndices = new Array(flat.length);
    const consumedOld = new Set;
    let lastPlacedIndex = 0;
    const needsMove = new Array(flat.length);
    const pendingMounts = new Array(flat.length);
    for (let i = 0;i < flat.length; i++) {
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
    for (let i = 0;i < oldChildren.length; i++) {
      if (!consumedOld.has(i)) {
        unmountTree(oldChildren[i], parentHostId);
        batchNeedsLayout = true;
      }
    }
    let anchorHostId = nextSiblingHostId;
    for (let i = flat.length - 1;i >= 0; i--) {
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
  function updateComponentChildren(comp, rendered, nextSiblingHostId) {
    const flat = [];
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
  function updateMountedNode(mounted, vnode, parentHostId, nextSiblingHostId) {
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
  function findNextSiblingHostForComp(node, target, inheritedNextHostId) {
    if (!node)
      return { found: false, nextHostId: inheritedNextHostId };
    if (node === target)
      return { found: true, nextHostId: inheritedNextHostId };
    if (node.kind === "comp") {
      return findNextSiblingHostForComp(node.child, target, inheritedNextHostId);
    }
    const list = node.children;
    let after = node.kind === "host" ? 0 : inheritedNextHostId;
    for (let i = list.length - 1;i >= 0; i--) {
      const res = findNextSiblingHostForComp(list[i], target, after);
      if (res.found)
        return res;
      const first = getFirstHostId(list[i]);
      if (first !== 0)
        after = first;
    }
    return { found: false, nextHostId: inheritedNextHostId };
  }
  function flushDirty() {
    while (dirtyQueue.size > 0) {
      const snapshot = Array.from(dirtyQueue);
      dirtyQueue.clear();
      batchActive = true;
      batchNeedsLayout = false;
      nativeApi().beginBatch();
      for (let i = 0;i < snapshot.length; i++) {
        const comp = snapshot[i];
        if (comp.unmounted || !comp.dirty)
          continue;
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
  function useState(initial) {
    const comp = currentComp;
    const idx = hookCursor++;
    if (idx >= comp.hooks.length) {
      const val = typeof initial === "function" ? initial() : initial;
      comp.hooks.push(val);
    }
    const setState = (next) => {
      if (comp.unmounted)
        return;
      const prevVal = comp.hooks[idx];
      const resolved = typeof next === "function" ? next(prevVal) : next;
      if (!Object.is(prevVal, resolved)) {
        comp.hooks[idx] = resolved;
        comp.dirty = true;
        dirtyQueue.add(comp);
        scheduleFlush();
      }
    };
    return [comp.hooks[idx], setState];
  }
  function useRef(initial) {
    const comp = currentComp;
    const idx = hookCursor++;
    if (idx >= comp.hooks.length) {
      comp.hooks.push({ current: initial });
    }
    return comp.hooks[idx];
  }
  function useMemo(factory, deps) {
    const comp = currentComp;
    const idx = hookCursor++;
    if (idx >= comp.hooks.length) {
      const val = factory();
      comp.hooks.push({ val, deps });
      return val;
    }
    const slot = comp.hooks[idx];
    if (depsChanged(slot.deps, deps)) {
      slot.val = factory();
      slot.deps = deps;
    }
    return slot.val;
  }
  function useEffect(fn, deps) {
    const comp = currentComp;
    const idx = hookCursor++;
    if (idx >= comp.hooks.length) {
      const rec2 = { fn, deps, cleanup: undefined };
      comp.hooks.push(rec2);
      comp.effects[idx] = rec2;
      comp.pendingEffects.push(idx);
      return;
    }
    const rec = comp.hooks[idx];
    if (deps === undefined || depsChanged(rec.deps, deps)) {
      rec.fn = fn;
      rec.deps = deps;
      comp.effects[idx] = rec;
      comp.pendingEffects.push(idx);
    }
  }
  function render(element) {
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
  function setWindowTitle(title) {
    nativeApi().setTitle(title);
  }
  function useWindowKeyDown(handler) {
    const fnRef = useRef(handler);
    fnRef.current = handler;
    useEffect(() => {
      const listener = (keyCode, isTextInputFocused) => {
        fnRef.current(keyCode, isTextInputFocused);
      };
      windowKeyListeners.add(listener);
      return () => {
        windowKeyListeners.delete(listener);
      };
    }, []);
  }
  globalThis.__dispatchEvent = function __dispatchEvent(nodeId, eventType, eventSeq, payload) {
    if (eventSeq > 0) {
      nodeLastSeq.set(nodeId, eventSeq);
    }
    const h = nodeHandlers.get(nodeId);
    switch (eventType) {
      case 1 /* Click */:
        h?.onClick?.();
        break;
      case 3 /* TextChanged */:
        h?.onTextChanged?.(payload.str);
        break;
      case 4 /* Submit */:
        h?.onSubmit?.(payload.str);
        break;
      case 2 /* Change */:
        if (h?.onChange) {
          if (nodeTypes.get(nodeId) === 7 /* Slider */) {
            h.onChange(payload.num);
          } else {
            h.onChange(payload.bool);
          }
        }
        break;
      case 5 /* PageChange */:
        h?.onPageChange?.(payload.int, payload.int2);
        break;
      case 6 /* TreeSelect */:
        h?.onSelect?.(payload.str, payload.int);
        if (payload.int > 0) {
          nodeHandlers.get(payload.int)?.onSelect?.(payload.str, payload.int);
        }
        break;
      case 7 /* TreeCheck */:
        h?.onCheck?.(payload.bool, payload.str, payload.int);
        if (payload.int > 0) {
          nodeHandlers.get(payload.int)?.onCheck?.(payload.bool, payload.str, payload.int);
        }
        break;
      case 8 /* KeyDown */:
        windowKeyListeners.forEach((fn) => fn(payload.int, payload.bool));
        break;
    }
  };
  // react/demos/stock_terminal/market_engine.ts
  var STOCK_DATABASE = [
    { code: "1A0001", name: "上证指数", pinyin: "SZZS", market: "上证", basePrice: 3534.32, isIndex: true },
    { code: "399001", name: "深证成指", pinyin: "SZCZ", market: "深证", basePrice: 14718.66, isIndex: true },
    { code: "399006", name: "创业板指", pinyin: "CYBZ", market: "创业", basePrice: 3352.12, isIndex: true },
    { code: "000688", name: "科创50", pinyin: "KC50", market: "科创", basePrice: 1575.89, isIndex: true },
    { code: "600519", name: "贵州茅台", pinyin: "GZMT", market: "沪A", basePrice: 1994 },
    { code: "600036", name: "招商银行", pinyin: "ZSYH", market: "沪A", basePrice: 52.94 },
    { code: "601166", name: "兴业银行", pinyin: "XYYH", market: "沪A", basePrice: 21.04 },
    { code: "600196", name: "复星医药", pinyin: "FXYY", market: "沪A", basePrice: 72.49 },
    { code: "600019", name: "宝钢股份", pinyin: "BGGF", market: "沪A", basePrice: 7.5 },
    { code: "603833", name: "欧派家居", pinyin: "OPJJ", market: "沪A", basePrice: 135.9 },
    { code: "601088", name: "中国神华", pinyin: "ZGSH", market: "沪A", basePrice: 19.96 },
    { code: "603259", name: "药明康德", pinyin: "YMKD", market: "沪A", basePrice: 142.12 },
    { code: "601318", name: "中国平安", pinyin: "ZGPA", market: "沪A", basePrice: 61.91 },
    { code: "600809", name: "山西汾酒", pinyin: "SXFJ", market: "沪A", basePrice: 85.25 },
    { code: "600703", name: "北方稀土", pinyin: "BFXT", market: "沪A", basePrice: 22.68 },
    { code: "600919", name: "江苏银行", pinyin: "JSYH", market: "沪A", basePrice: 7.07 },
    { code: "600000", name: "浦发银行", pinyin: "PFYH", market: "沪A", basePrice: 9.92 },
    { code: "600585", name: "海螺水泥", pinyin: "HLSN", market: "沪A", basePrice: 40.18 },
    { code: "000001", name: "平安银行", pinyin: "PAYH", market: "深A", basePrice: 15.84 },
    { code: "002594", name: "比亚迪", pinyin: "BYD", market: "深A", basePrice: 268.4 },
    { code: "300750", name: "宁德时代", pinyin: "NDSD", market: "创业", basePrice: 245.6 }
  ];
  var CONTRIB_RANKS = [
    { code: "600519", name: "贵州茅台", changePct: -0.94, price: 1994, contribution: 0.17 },
    { code: "600036", name: "招商银行", changePct: -0.21, price: 52.94, contribution: 0.07 },
    { code: "601166", name: "兴业银行", changePct: 1.2, price: 21.04, contribution: 0.06 },
    { code: "600196", name: "复星医药", changePct: 0.68, price: 72.49, contribution: 0.05 },
    { code: "601088", name: "中国神华", changePct: 3.53, price: 19.96, contribution: 0.03 },
    { code: "601318", name: "中国平安", changePct: -0.63, price: 61.91, contribution: 0.03 },
    { code: "600703", name: "北方稀土", changePct: 9.99, price: 22.68, contribution: 0.02 },
    { code: "603833", name: "欧派家居", changePct: -2.36, price: 135.9, contribution: -0.03 }
  ];
  var INITIAL_TICKS = [
    { timeStr: "14:58:33", typeStr: "大笔买入", volumeLabel: "171手", isBuy: true },
    { timeStr: "14:58:35", typeStr: "大笔买入", volumeLabel: "11006手", isBuy: true },
    { timeStr: "14:58:38", typeStr: "大笔买入", volumeLabel: "2438手", isBuy: true },
    { timeStr: "14:58:39", typeStr: "大笔卖出", volumeLabel: "1920手", isBuy: false },
    { timeStr: "14:58:45", typeStr: "大笔卖出", volumeLabel: "5612手", isBuy: false },
    { timeStr: "14:58:48", typeStr: "大笔买入", volumeLabel: "23357手", isBuy: true },
    { timeStr: "14:58:51", typeStr: "急速拉升", volumeLabel: "1.66%", isBuy: true, highlight: true },
    { timeStr: "14:58:53", typeStr: "区间放量(涨)", volumeLabel: "6.02倍", isBuy: true },
    { timeStr: "14:59:01", typeStr: "大笔买入", volumeLabel: "4235手", isBuy: true },
    { timeStr: "14:59:12", typeStr: "大笔卖出", volumeLabel: "788手", isBuy: false }
  ];
  var NEWS_TABS = [
    "股市直播",
    "投资机会",
    "观点评论",
    "资金流向",
    "主力增仓",
    "短线涨跌",
    "板块热点",
    "关联指数",
    "相关ETF"
  ];
  var NEWS_BY_TAB = {
    股市直播: {
      left: [
        { text: "[解盘] 港股收评：恒指收跌0.59% 腾讯控股跌3.57%     16:09", color: "normal" },
        { text: "[解盘] 沪深两市成交额突破1万亿元                    15:01", color: "red" },
        { text: "[解盘] 第三代半导体板块持续走强                      14:22", color: "red" }
      ],
      right: [
        { text: "[要闻] 华银电力：公司与张家界市签订能源开发战略协议  16:20", color: "blue" },
        { text: "[要闻] 凯撒文化：预计2021年前半年盈利同比增长50%    16:19", color: "blue" },
        { text: "[要闻] 华鲁恒升：预计上半年净利润同比增长314.82%    16:13", color: "red" }
      ]
    },
    投资机会: {
      left: [
        { text: "[研报] 新能源车产业链销量超预期，关注锂电隔膜龙头    15:40", color: "red" },
        { text: "[研报] 银行板块低估值修复空间显现，股息率保持高位    14:55", color: "blue" },
        { text: "[研报] AI 端侧硬件加速渗透，关注封装测试产能景气度    13:45", color: "normal" }
      ],
      right: [
        { text: "[机会] 稀土永磁下游需求放量，北方稀土涨停带动板块    15:12", color: "red" },
        { text: "[机会] 高端白酒批价坚挺，中秋旺季备货周期渐近        14:08", color: "blue" },
        { text: "[机会] 光伏逆变器出海订单同比翻倍，盈利能力稳中向好  11:20", color: "normal" }
      ]
    },
    资金流向: {
      left: [
        { text: "[资金] 主力净流入电子板块 +48.6亿，有色金属 +31.2亿  15:00", color: "red" },
        { text: "[资金] 北向资金全天净买入 A 股 62.4亿元              15:01", color: "red" },
        { text: "[资金] 医药生物小幅净流出 -6.3亿元                   14:30", color: "normal" }
      ],
      right: [
        { text: "[北向] 贵州茅台获净买入 +8.42亿，招商银行 +4.15亿    15:05", color: "blue" },
        { text: "[两融] 沪深两市融资余额单日增加 +74.8亿元            14:50", color: "blue" },
        { text: "[大宗] 宁德时代机构专用席位溢价成交 2.1亿元          15:15", color: "red" }
      ]
    }
  };
  function nextRand(state) {
    const next = Math.imul(state, 1664525) + 1013904223 >>> 0;
    return [next / 4294967296, next];
  }
  function hashSeed(code) {
    let h = 1337;
    for (let i = 0;i < code.length; i++) {
      h = Math.imul(h, 31) + code.charCodeAt(i) >>> 0;
    }
    return h || 1337;
  }
  function pad2(n) {
    return n < 10 ? `0${n}` : `${n}`;
  }
  function minuteToTimeStr(m) {
    let hour;
    let min;
    if (m <= 120) {
      hour = 9 + Math.floor((30 + m) / 60);
      min = (30 + m) % 60;
    } else {
      const pm = m - 120;
      hour = 13 + Math.floor(pm / 60);
      min = pm % 60;
    }
    return `${pad2(hour)}:${pad2(min)}`;
  }
  function generateCandles(endPrice, count, seedOffset, labelMode) {
    const bars = [];
    let rng = 1337 + seedOffset * 97 >>> 0;
    let cur = endPrice * 0.956;
    for (let i = 0;i < count; i++) {
      let dateStr;
      if (labelMode === "week") {
        dateStr = `W${pad2(i + 1)}`;
      } else if (labelMode === "month") {
        dateStr = `${24 + Math.floor(i / 12)}/${pad2(i % 12 + 1)}`;
      } else if (labelMode === "min") {
        dateStr = minuteToTimeStr(Math.min(240, i * 4));
      } else {
        const m = 1 + Math.floor(i / 22);
        const d = 1 + i % 22;
        dateStr = `${pad2(m)}/${pad2(d)}`;
      }
      let r1;
      let r2;
      let r3;
      [r1, rng] = nextRand(rng);
      [r2, rng] = nextRand(rng);
      [r3, rng] = nextRand(rng);
      const pct = -0.016 + r1 * 0.034;
      const nextPrice = i === count - 1 ? endPrice : cur * (1 + pct);
      const bodyHi = Math.max(cur, nextPrice);
      const bodyLo = Math.min(cur, nextPrice);
      const shadowScale = endPrice * 0.004;
      const high = bodyHi + r2 * shadowScale;
      const low = Math.max(0.5, bodyLo - r3 * shadowScale);
      const vol = 12000 + r1 * 36000;
      bars.push({
        date: dateStr,
        open: cur,
        high,
        low,
        close: nextPrice,
        volume: vol,
        ma5: nextPrice,
        ma10: nextPrice,
        ma20: nextPrice
      });
      cur = nextPrice;
    }
    for (let i = 0;i < bars.length; i++) {
      if (i >= 4) {
        let s = 0;
        for (let k = i - 4;k <= i; k++)
          s += bars[k].close;
        bars[i].ma5 = s / 5;
      } else {
        bars[i].ma5 = bars[i].close;
      }
      if (i >= 9) {
        let s = 0;
        for (let k = i - 9;k <= i; k++)
          s += bars[k].close;
        bars[i].ma10 = s / 10;
      } else {
        bars[i].ma10 = bars[i].ma5;
      }
      if (i >= 19) {
        let s = 0;
        for (let k = i - 19;k <= i; k++)
          s += bars[k].close;
        bars[i].ma20 = s / 20;
      } else {
        bars[i].ma20 = bars[i].ma10;
      }
    }
    return bars;
  }
  function createInitialSession(symbol, initialMinutes = 205) {
    let rng = hashSeed(symbol.code);
    const preClose = symbol.code === "1A0001" ? 3518.76 : symbol.basePrice * 0.9956;
    const open = symbol.code === "1A0001" ? 3516.92 : preClose * 0.9994;
    const pts = [];
    let prevPrice = preClose;
    let hi = open;
    let lo = open;
    for (let m = 0;m <= initialMinutes; m++) {
      let rNoise;
      let rVol;
      [rNoise, rng] = nextRand(rng);
      [rVol, rng] = nextRand(rng);
      const progress = m / 240;
      const wave = Math.sin(progress * Math.PI * 1.5) * (preClose * 0.0045);
      const noise = (-0.8 + rNoise * 1.8) * (preClose / 3518.76);
      const price = m === initialMinutes ? symbol.basePrice : preClose + wave + noise;
      const leadPrice = price + (1.2 + 0.5 * Math.cos(progress * 6.28)) * (preClose / 3518.76);
      const vol = (500 + rVol * 3000) * (m < 15 || m > 220 ? 2.5 : 1);
      if (price > hi)
        hi = price;
      if (price < lo)
        lo = price;
      pts.push({
        price,
        leadPrice,
        volume: vol,
        indicator1: 60 + 25 * progress + noise * 2,
        indicator2: 40 - 15 * progress + noise * 1.5,
        isUp: price >= prevPrice,
        timeStr: minuteToTimeStr(m)
      });
      prevPrice = price;
    }
    const current = pts[pts.length - 1].price;
    const change = current - preClose;
    const changePct = change / preClose * 100;
    const isMain = symbol.code === "1A0001";
    const snapshot = {
      code: symbol.code,
      name: symbol.name,
      current,
      preClose,
      open,
      high: isMain ? Math.max(hi, 3534.32) : hi,
      low: isMain ? Math.min(lo, 3510.58) : lo,
      change,
      changePct,
      amplitude: (hi - lo) / preClose * 100,
      volumeRatio: isMain ? 0.97 : 1.12,
      totalHand: isMain ? 28809 : Math.round(420 + hashSeed(symbol.code) % 3500),
      totalAmount: isMain ? 4454 : Math.round(28 + hashSeed(symbol.code) % 120),
      marketTurnover: 10060,
      weiBi: isMain ? -2.11 : 4.35,
      weiDiff: isMain ? -665486 : 14820,
      weiBuyHand: isMain ? 1543 : 182,
      weiSellHand: isMain ? 1610 : 149,
      peDynamic: isMain ? 14.83 : 24.6,
      pbRatio: isMain ? 1.61 : 2.85,
      turnoverRate: isMain ? 0.77 : 1.34,
      upCount: 1311,
      downCount: 597
    };
    return {
      symbol,
      snapshot,
      points: pts,
      ticks: INITIAL_TICKS.slice(0, 8),
      candlesByPeriod: {},
      rngState: rng
    };
  }
  function getOrGenerateCandles(session, period) {
    const cached = session.candlesByPeriod[period];
    if (cached)
      return cached;
    let count = 90;
    let mode = "day";
    let seedBias = 1;
    if (period === "week") {
      count = 45;
      mode = "week";
      seedBias = 2;
    } else if (period === "month") {
      count = 24;
      mode = "month";
      seedBias = 3;
    } else if (period !== "day") {
      count = 60;
      mode = "min";
      seedBias = period === "m1" ? 4 : period === "m5" ? 5 : period === "m15" ? 6 : period === "m30" ? 7 : 8;
    }
    const bars = generateCandles(session.snapshot.current, count, hashSeed(session.symbol.code) + seedBias, mode);
    session.candlesByPeriod[period] = bars;
    return bars;
  }
  function stepMarketTick(session) {
    let rng = session.rngState;
    let r1;
    let r2;
    let r3;
    [r1, rng] = nextRand(rng);
    [r2, rng] = nextRand(rng);
    [r3, rng] = nextRand(rng);
    const prev = session.points[session.points.length - 1];
    const scale = session.snapshot.preClose / 3518.76;
    const delta = (-0.25 + r1 * 0.6) * scale;
    const minP = session.snapshot.preClose * 0.985;
    const maxP = session.snapshot.preClose * 1.022;
    const newPrice = Math.min(maxP, Math.max(minP, prev.price + delta));
    const isUp = newPrice >= prev.price;
    let nextPoints;
    const nextMinute = session.points.length;
    if (nextMinute <= 240) {
      const newPt = {
        price: newPrice,
        leadPrice: newPrice + (1 + (-0.1 + r2 * 0.25)) * scale,
        volume: Math.max(250, prev.volume * 0.4 + Math.abs(delta) * 900),
        indicator1: prev.indicator1 ?? 68,
        indicator2: prev.indicator2 ?? 32,
        isUp,
        timeStr: minuteToTimeStr(nextMinute)
      };
      nextPoints = [...session.points, newPt];
    } else {
      const updatedLast = {
        ...prev,
        price: newPrice,
        leadPrice: newPrice + (1 + (-0.1 + r2 * 0.25)) * scale,
        volume: prev.volume + Math.abs(delta) * 800,
        isUp
      };
      nextPoints = [...session.points.slice(0, -1), updatedLast];
    }
    const snap = session.snapshot;
    const nextHigh = Math.max(snap.high, newPrice);
    const nextLow = Math.min(snap.low, newPrice);
    const nextChange = newPrice - snap.preClose;
    const nextPct = nextChange / snap.preClose * 100;
    const nextWeiBi = Math.max(-15, Math.min(15, snap.weiBi + (-0.35 + r2 * 0.72)));
    const now = new Date;
    const timeStr = `${pad2(now.getHours())}:${pad2(now.getMinutes())}:${pad2(now.getSeconds())}`;
    const kindPick = Math.floor(r3 * 5);
    const volHand = Math.floor(120 + r2 * 14500);
    const newTick = kindPick === 0 ? { timeStr, typeStr: "急速拉升", volumeLabel: `${Math.abs(nextPct).toFixed(2)}%`, isBuy: true, highlight: true } : kindPick <= 2 ? { timeStr, typeStr: "大笔买入", volumeLabel: `${volHand}手`, isBuy: true } : { timeStr, typeStr: "大笔卖出", volumeLabel: `${volHand}手`, isBuy: false };
    return {
      ...session,
      rngState: rng,
      points: nextPoints,
      ticks: [...session.ticks.slice(1), newTick],
      snapshot: {
        ...snap,
        current: newPrice,
        high: nextHigh,
        low: nextLow,
        change: nextChange,
        changePct: nextPct,
        amplitude: (nextHigh - nextLow) / snap.preClose * 100,
        totalHand: snap.totalHand + 1.2,
        totalAmount: snap.totalAmount + 0.4,
        marketTurnover: snap.marketTurnover + 0.9,
        weiBi: nextWeiBi,
        weiDiff: Math.round(snap.weiDiff + (r2 - 0.48) * 12000)
      }
    };
  }
  function querySymbols(query, maxResults = 6) {
    const q = query.trim().toUpperCase();
    if (!q)
      return [];
    const out = [];
    for (const s of STOCK_DATABASE) {
      if (s.code.includes(q) || s.pinyin.includes(q) || s.name.includes(q)) {
        out.push(s);
        if (out.length >= maxResults)
          break;
      }
    }
    return out;
  }

  // react/demos/stock_terminal/theme.ts
  var CLASSIC_THEME = {
    isModern: false,
    rootBg: "#050505",
    menuBg: "#121212",
    toolbarBg: "#181818",
    tabBarBg: "#0E0E0E",
    sidebarStripBg: "#0E0E0E",
    subBarBg: "#0C0C0C",
    newsListBg: "#080808",
    rightPanelBg: "#080808",
    rightHeaderBg: "#0B0B0B",
    depthBg: "#0E0E0E",
    sectionHdrBg: "#161616",
    rankBg: "#0A0A0A",
    tickBg: "#080808",
    statusBarBg: "#121212",
    popupBg: "#11161F",
    popupBorder: "#2D3B4E",
    inputBg: "#222222",
    btnBg: "#282828",
    btnDimText: "#888888",
    btnText: "#DDDDDD",
    textPrimary: "#EEEEEE",
    textSecondary: "#B0B0B0",
    textDim: "#777777",
    textMuted: "#555555",
    upRed: "#EE2222",
    downGreen: "#00CC00",
    accentGold: "#E5B800",
    accentBlue: "#4499FF",
    buyBtnBg: "#EE2222",
    sellBtnBg: "#00AA00",
    infoBtnBg: "#1C355E",
    infoBtnFg: "#88CCFF",
    orderBtnBg: "#4A1818",
    orderBtnFg: "#FF8888",
    themeBtnBg: "#333333"
  };
  var MODERN_THEME = {
    isModern: true,
    rootBg: "#0B0E14",
    menuBg: "#131722",
    toolbarBg: "#181D2B",
    tabBarBg: "#10141E",
    sidebarStripBg: "#10141E",
    subBarBg: "#0F131C",
    newsListBg: "#0D1119",
    rightPanelBg: "#0D1119",
    rightHeaderBg: "#131722",
    depthBg: "#151A26",
    sectionHdrBg: "#1B2130",
    rankBg: "#111622",
    tickBg: "#0D1119",
    statusBarBg: "#131722",
    popupBg: "#1B2333",
    popupBorder: "#34425E",
    inputBg: "#1E2536",
    btnBg: "#232A3B",
    btnDimText: "#787B86",
    btnText: "#D1D4DC",
    textPrimary: "#E2E6F0",
    textSecondary: "#9CA3B8",
    textDim: "#787B86",
    textMuted: "#565B6B",
    upRed: "#F6465D",
    downGreen: "#0ECB81",
    accentGold: "#FFA000",
    accentBlue: "#2962FF",
    buyBtnBg: "#F6465D",
    sellBtnBg: "#0ECB81",
    infoBtnBg: "#1D3254",
    infoBtnFg: "#82B1FF",
    orderBtnBg: "#4B1E28",
    orderBtnFg: "#FF94A2",
    themeBtnBg: "#263046"
  };

  // react/demos/stock_terminal/index.tsx
  var TOOLBAR_ACTIONS = [
    "撤单",
    "自选",
    "F10",
    "周期",
    "画线",
    "选股",
    "研报",
    "资金",
    "两融",
    "热点",
    "龙虎",
    "数据",
    "创/科"
  ];
  var MARKET_CATEGORIES = ["全景", "个股", "指数", "期指", "期权", "基金", "外汇", "美股", "自定", "多窗"];
  var PERIOD_BUTTONS = [
    { key: "timeshare", label: "分时", width: 40, capsule: "分时" },
    { key: "day", label: "日K", width: 36, capsule: "日线 ▼" },
    { key: "week", label: "周K", width: 36, capsule: "周线 ▼" },
    { key: "month", label: "月K", width: 36, capsule: "月线 ▼" },
    { key: "m1", label: "1分", width: 32, capsule: "1分 ▼" },
    { key: "m5", label: "5分", width: 32, capsule: "5分 ▼" },
    { key: "m15", label: "15分", width: 36, capsule: "15分 ▼" },
    { key: "m30", label: "30分", width: 36, capsule: "30分 ▼" },
    { key: "m60", label: "60分", width: 36, capsule: "60分 ▼" }
  ];
  function DepthCellRow(props) {
    return /* @__PURE__ */ jsxDEV(Panel, {
      direction: "row",
      height: 18,
      padding: 0,
      spacing: 0,
      alignItems: "center",
      children: [
        /* @__PURE__ */ jsxDEV(Label, {
          text: props.k1,
          fontSize: 11,
          color: props.dimColor,
          width: 42,
          height: 17
        }, undefined, false, undefined, this),
        /* @__PURE__ */ jsxDEV(Label, {
          text: props.v1,
          fontSize: 11,
          color: props.c1,
          width: 112,
          height: 17
        }, undefined, false, undefined, this),
        /* @__PURE__ */ jsxDEV(Label, {
          text: props.k2,
          fontSize: 11,
          color: props.dimColor,
          width: 42,
          height: 17
        }, undefined, false, undefined, this),
        /* @__PURE__ */ jsxDEV(Label, {
          text: props.v2,
          fontSize: 11,
          color: props.c2,
          width: 118,
          height: 17
        }, undefined, false, undefined, this)
      ]
    }, undefined, true, undefined, this);
  }
  function StockTerminalApp() {
    const [isModernTheme, setIsModernTheme] = useState(false);
    const [period, setPeriod] = useState("timeshare");
    const [livePaused, setLivePaused] = useState(false);
    const [activeMarketTab, setActiveMarketTab] = useState("指数");
    const [activeNewsTab, setActiveNewsTab] = useState("股市直播");
    const [searchQuery, setSearchQuery] = useState("");
    const [statusNotice, setStatusNotice] = useState("实时行情引擎就绪 [T 切换主题 · F5 刷新]");
    const [session, setSession] = useState(() => createInitialSession(STOCK_DATABASE[0]));
    const T = isModernTheme ? MODERN_THEME : CLASSIC_THEME;
    useEffect(() => {
      setWindowTitle(`同花顺金融终端 (React JSX) - ${session.symbol.name} ${session.symbol.code} [${isModernTheme ? "Modern" : "Classic"}]`);
    }, [session.symbol.code, session.symbol.name, isModernTheme]);
    useEffect(() => {
      if (livePaused)
        return;
      const id = setInterval(() => {
        setSession((prev) => stepMarketTick(prev));
      }, 500);
      return () => clearInterval(id);
    }, [livePaused]);
    const selectSymbolByCode = (code) => {
      const found = STOCK_DATABASE.find((s) => s.code === code);
      if (!found)
        return;
      setSession(createInitialSession(found));
      setSearchQuery("");
      setStatusNotice(`已切换标的：${found.name} (${found.code})`);
    };
    useWindowKeyDown((keyCode, isTextInputFocused) => {
      if (keyCode === 84 && !isTextInputFocused) {
        setIsModernTheme((v) => !v);
      } else if (keyCode === 116) {
        setSession((prev) => createInitialSession(prev.symbol));
        setStatusNotice("已刷新日内分时基准数据 (F5)");
      } else if (keyCode === 27) {
        setSearchQuery("");
      }
    });
    const genieMatches = useMemo(() => querySymbols(searchQuery, 6), [searchQuery]);
    const snap = session.snapshot;
    const priceUp = snap.change >= 0;
    const priceColor = priceUp ? T.upRed : T.downGreen;
    const changeSign = snap.change >= 0 ? "+" : "";
    const activePeriodMeta = PERIOD_BUTTONS.find((p) => p.key === period) ?? PERIOD_BUTTONS[0];
    const activeCandles = period === "timeshare" ? undefined : getOrGenerateCandles(session, period);
    const newsSet = NEWS_BY_TAB[activeNewsTab] ?? NEWS_BY_TAB["股市直播"];
    return /* @__PURE__ */ jsxDEV(Panel, {
      direction: "column",
      padding: 0,
      spacing: 0,
      background: T.rootBg,
      alignItems: "stretch",
      children: [
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          height: 28,
          padding: { x: 10, y: 0 },
          spacing: 10,
          background: T.menuBg,
          alignItems: "center",
          children: [
            /* @__PURE__ */ jsxDEV(Label, {
              text: "同花顺",
              fontSize: 13,
              color: T.upRed,
              bold: true
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: `• ${session.symbol.name}`,
              fontSize: 12,
              color: T.textPrimary,
              width: 96,
              height: 18
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Spacer, {
              width: 6
            }, undefined, false, undefined, this),
            ["系统", "报价", "行情", "分析", "交易", "智能", "工具", "资讯", "帮助"].map((m) => /* @__PURE__ */ jsxDEV(Label, {
              text: m,
              fontSize: 12,
              color: T.textSecondary
            }, m, false, undefined, this)),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "决策",
              fontSize: 12,
              color: T.upRed,
              bold: true
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Spacer, {
              flexGrow: 1,
              flexBasis: 0
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: statusNotice,
              fontSize: 11,
              color: T.accentGold,
              width: 250,
              height: 18,
              horizontalAlign: "end"
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Spacer, {
              width: 6
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(TextInput, {
              id: "top_search",
              text: searchQuery,
              placeholder: "代码/简拼",
              width: 92,
              height: 20,
              fontSize: 11,
              background: T.inputBg,
              color: T.textPrimary,
              placeholderColor: T.btnDimText,
              borderColor: T.popupBorder,
              onTextChanged: setSearchQuery,
              onSubmit: () => {
                if (genieMatches.length > 0)
                  selectSymbolByCode(genieMatches[0].code);
              }
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Button, {
              text: "资讯",
              width: 38,
              height: 20,
              fontSize: 11,
              background: T.infoBtnBg,
              foreground: T.infoBtnFg,
              onClick: () => setActiveNewsTab("股市直播")
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Button, {
              text: "委托",
              width: 38,
              height: 20,
              fontSize: 11,
              background: T.orderBtnBg,
              foreground: T.orderBtnFg,
              onClick: () => setStatusNotice("模拟委托通道已开启 [仅限演示]")
            }, undefined, false, undefined, this)
          ]
        }, undefined, true, undefined, this),
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          height: 30,
          padding: { x: 8, y: 0 },
          spacing: 4,
          background: T.toolbarBg,
          alignItems: "center",
          children: [
            /* @__PURE__ */ jsxDEV(Button, {
              text: "买入",
              width: 42,
              height: 22,
              fontSize: 11,
              background: T.buyBtnBg,
              foreground: "#FFFFFF",
              onClick: () => setStatusNotice(`模拟买入申报: ${session.symbol.code} @ ${snap.current.toFixed(2)}`)
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Button, {
              text: "卖出",
              width: 42,
              height: 22,
              fontSize: 11,
              background: T.sellBtnBg,
              foreground: "#FFFFFF",
              onClick: () => setStatusNotice(`模拟卖出申报: ${session.symbol.code} @ ${snap.current.toFixed(2)}`)
            }, undefined, false, undefined, this),
            TOOLBAR_ACTIONS.map((act) => /* @__PURE__ */ jsxDEV(Button, {
              text: act,
              width: act.length > 2 ? 44 : 40,
              height: 22,
              fontSize: 11,
              background: T.btnBg,
              foreground: T.btnText,
              onClick: () => setStatusNotice(`功能[${act}] - 当前标的 ${session.symbol.name}`)
            }, act, false, undefined, this)),
            /* @__PURE__ */ jsxDEV(Spacer, {
              flexGrow: 1,
              flexBasis: 0
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Button, {
              text: livePaused ? "继续行情" : "暂停Tick",
              width: 68,
              height: 22,
              fontSize: 11,
              background: livePaused ? T.orderBtnBg : T.btnBg,
              foreground: livePaused ? T.orderBtnFg : T.downGreen,
              onClick: () => setLivePaused((p) => !p)
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Button, {
              id: "btn_toggle_theme",
              text: "双主题切换 (T)",
              width: 98,
              height: 22,
              fontSize: 11,
              background: T.themeBtnBg,
              foreground: T.accentGold,
              onClick: () => setIsModernTheme((v) => !v)
            }, undefined, false, undefined, this)
          ]
        }, undefined, true, undefined, this),
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          height: 24,
          padding: { x: 6, y: 0 },
          spacing: 3,
          background: T.tabBarBg,
          alignItems: "center",
          children: MARKET_CATEGORIES.map((tab) => {
            const active = tab === activeMarketTab;
            return /* @__PURE__ */ jsxDEV(Button, {
              text: tab,
              width: 38,
              height: 20,
              fontSize: 11,
              background: active ? T.buyBtnBg : T.btnBg,
              foreground: active ? "#FFFFFF" : T.textSecondary,
              onClick: () => {
                setActiveMarketTab(tab);
                if (tab === "指数")
                  selectSymbolByCode("1A0001");
                else if (tab === "个股")
                  selectSymbolByCode("600519");
              }
            }, tab, false, undefined, this);
          })
        }, undefined, false, undefined, this),
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          flexGrow: 1,
          flexBasis: 0,
          padding: 0,
          spacing: 0,
          alignItems: "stretch",
          children: [
            /* @__PURE__ */ jsxDEV(Panel, {
              direction: "column",
              width: 28,
              padding: 2,
              spacing: 2,
              background: T.sidebarStripBg,
              alignItems: "center",
              children: [
                /* @__PURE__ */ jsxDEV(Button, {
                  text: "首页",
                  width: 24,
                  height: 26,
                  fontSize: 10,
                  background: T.buyBtnBg,
                  foreground: "#FFFFFF",
                  onClick: () => selectSymbolByCode("1A0001")
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Button, {
                  id: "left_btn_timeshare",
                  text: "分时",
                  width: 24,
                  height: 26,
                  fontSize: 10,
                  background: period === "timeshare" ? T.buyBtnBg : T.btnBg,
                  foreground: "#EEEEEE",
                  onClick: () => setPeriod("timeshare")
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Button, {
                  id: "left_btn_kline",
                  text: "K线",
                  width: 24,
                  height: 26,
                  fontSize: 10,
                  background: period !== "timeshare" ? T.buyBtnBg : T.btnBg,
                  foreground: "#EEEEEE",
                  onClick: () => setPeriod("day")
                }, undefined, false, undefined, this),
                ["应用", "指数", "大盘", "自选", "排名", "更多"].map((lbl) => /* @__PURE__ */ jsxDEV(Button, {
                  text: lbl,
                  width: 24,
                  height: 26,
                  fontSize: 10,
                  background: T.btnBg,
                  foreground: T.btnDimText,
                  onClick: () => setStatusNotice(`快捷入口：${lbl}`)
                }, lbl, false, undefined, this))
              ]
            }, undefined, true, undefined, this),
            /* @__PURE__ */ jsxDEV(Panel, {
              direction: "column",
              flexGrow: 1,
              flexBasis: 0,
              padding: 0,
              spacing: 0,
              alignItems: "stretch",
              children: [
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  height: 26,
                  padding: { x: 6, y: 0 },
                  spacing: 3,
                  background: T.subBarBg,
                  alignItems: "center",
                  children: [
                    PERIOD_BUTTONS.map((p) => {
                      const active = period === p.key;
                      const btnId = p.key === "timeshare" ? "btn_timeshare" : p.key === "day" ? "btn_day_k" : undefined;
                      return /* @__PURE__ */ jsxDEV(Button, {
                        id: btnId,
                        text: p.label,
                        width: p.width,
                        height: 20,
                        fontSize: 11,
                        background: active ? T.buyBtnBg : T.btnBg,
                        foreground: active ? "#FFFFFF" : T.textSecondary,
                        onClick: () => setPeriod(p.key)
                      }, p.key, false, undefined, this);
                    }),
                    /* @__PURE__ */ jsxDEV(Button, {
                      text: "多周期",
                      width: 46,
                      height: 20,
                      fontSize: 11,
                      background: T.btnBg,
                      foreground: T.textDim,
                      onClick: () => setPeriod(period === "timeshare" ? "day" : "timeshare")
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Spacer, {
                      flexGrow: 1,
                      flexBasis: 0
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "九转  数据看盘  大盘异动  加自选",
                      fontSize: 11,
                      color: T.textMuted
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this),
                genieMatches.length > 0 ? /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  height: 28,
                  padding: { x: 8, y: 0 },
                  spacing: 6,
                  background: T.popupBg,
                  borderColor: T.popupBorder,
                  border: 1,
                  alignItems: "center",
                  children: [
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "键盘精灵匹配：",
                      fontSize: 11,
                      color: T.accentGold,
                      bold: true
                    }, undefined, false, undefined, this),
                    genieMatches.map((m) => /* @__PURE__ */ jsxDEV(Button, {
                      text: `${m.code} ${m.name} (${m.pinyin})`,
                      width: 136,
                      height: 20,
                      fontSize: 11,
                      background: T.btnBg,
                      foreground: T.textPrimary,
                      onClick: () => selectSymbolByCode(m.code)
                    }, m.code, false, undefined, this)),
                    /* @__PURE__ */ jsxDEV(Spacer, {
                      flexGrow: 1,
                      flexBasis: 0
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Button, {
                      text: "关闭(ESC)",
                      width: 64,
                      height: 20,
                      fontSize: 10,
                      background: T.orderBtnBg,
                      foreground: T.orderBtnFg,
                      onClick: () => setSearchQuery("")
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this) : null,
                /* @__PURE__ */ jsxDEV(FinancialChart, {
                  id: "stock_chart",
                  mode: period === "timeshare" ? "timeshare" : "candlestick",
                  preClose: snap.preClose,
                  modernTheme: isModernTheme,
                  showCrosshair: true,
                  showHeaderInfo: true,
                  symbolName: session.symbol.name,
                  periodLabel: activePeriodMeta.capsule,
                  points: session.points,
                  candles: activeCandles,
                  flexGrow: 1,
                  flexBasis: 0
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  height: 22,
                  padding: { x: 6, y: 0 },
                  spacing: 4,
                  background: T.menuBg,
                  alignItems: "center",
                  children: NEWS_TABS.map((nt) => {
                    const active = nt === activeNewsTab;
                    return /* @__PURE__ */ jsxDEV(Button, {
                      text: nt,
                      width: 56,
                      height: 18,
                      fontSize: 11,
                      background: active ? T.buyBtnBg : T.menuBg,
                      foreground: active ? "#FFFFFF" : T.textDim,
                      onClick: () => setActiveNewsTab(nt)
                    }, nt, false, undefined, this);
                  })
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  height: 54,
                  padding: { x: 8, y: 2 },
                  spacing: 12,
                  background: T.newsListBg,
                  alignItems: "stretch",
                  children: [
                    /* @__PURE__ */ jsxDEV(Panel, {
                      direction: "column",
                      flexGrow: 1,
                      flexBasis: 0,
                      padding: 0,
                      spacing: 1,
                      alignItems: "start",
                      children: newsSet.left.map((item, idx) => /* @__PURE__ */ jsxDEV(Label, {
                        text: item.text,
                        fontSize: 11,
                        color: item.color === "red" ? T.upRed : item.color === "blue" ? T.accentBlue : T.textSecondary
                      }, idx, false, undefined, this))
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Panel, {
                      direction: "column",
                      flexGrow: 1,
                      flexBasis: 0,
                      padding: 0,
                      spacing: 1,
                      alignItems: "start",
                      children: newsSet.right.map((item, idx) => /* @__PURE__ */ jsxDEV(Label, {
                        text: item.text,
                        fontSize: 11,
                        color: item.color === "red" ? T.upRed : item.color === "blue" ? T.accentBlue : T.textSecondary
                      }, idx, false, undefined, this))
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this)
              ]
            }, undefined, true, undefined, this),
            /* @__PURE__ */ jsxDEV(Panel, {
              direction: "column",
              width: 326,
              padding: 0,
              spacing: 0,
              background: T.rightPanelBg,
              alignItems: "stretch",
              children: [
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  height: 32,
                  padding: { x: 8, y: 2 },
                  spacing: 6,
                  background: T.rightHeaderBg,
                  alignItems: "center",
                  children: [
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: `${session.symbol.name} ${session.symbol.code}`,
                      fontSize: 14,
                      color: T.accentGold,
                      bold: true,
                      width: 145,
                      height: 22
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Spacer, {
                      flexGrow: 1,
                      flexBasis: 0
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Label, {
                      id: "label_current_price",
                      text: snap.current.toFixed(2),
                      fontSize: 18,
                      color: priceColor,
                      bold: true,
                      width: 86,
                      height: 24,
                      horizontalAlign: "end"
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Panel, {
                      direction: "column",
                      padding: 0,
                      spacing: 0,
                      width: 58,
                      alignItems: "end",
                      children: [
                        /* @__PURE__ */ jsxDEV(Label, {
                          id: "label_change",
                          text: `${changeSign}${snap.change.toFixed(2)}`,
                          fontSize: 11,
                          color: priceColor,
                          width: 58,
                          height: 14,
                          horizontalAlign: "end"
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Label, {
                          id: "label_change_pct",
                          text: `${changeSign}${snap.changePct.toFixed(2)}%`,
                          fontSize: 11,
                          color: priceColor,
                          width: 58,
                          height: 14,
                          horizontalAlign: "end"
                        }, undefined, false, undefined, this)
                      ]
                    }, undefined, true, undefined, this)
                  ]
                }, undefined, true, undefined, this),
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "column",
                  padding: { x: 6, y: 1 },
                  spacing: 0,
                  background: T.depthBg,
                  children: [
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "委比",
                      v1: `${snap.weiBi >= 0 ? "+" : ""}${snap.weiBi.toFixed(2)}%`,
                      c1: snap.weiBi >= 0 ? T.upRed : T.downGreen,
                      k2: "昨收",
                      v2: snap.preClose.toFixed(2),
                      c2: T.textPrimary,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "委差",
                      v1: `${Math.round(snap.weiDiff)}`,
                      c1: snap.weiDiff >= 0 ? T.upRed : T.downGreen,
                      k2: "今开",
                      v2: snap.open.toFixed(2),
                      c2: snap.open >= snap.preClose ? T.upRed : T.downGreen,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "涨跌",
                      v1: `${changeSign}${snap.change.toFixed(2)}`,
                      c1: priceColor,
                      k2: "最高",
                      v2: snap.high.toFixed(2),
                      c2: T.upRed,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "涨幅",
                      v1: `${changeSign}${snap.changePct.toFixed(2)}%`,
                      c1: priceColor,
                      k2: "最低",
                      v2: snap.low.toFixed(2),
                      c2: T.downGreen,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "振幅",
                      v1: `${snap.amplitude.toFixed(2)}%`,
                      c1: T.textPrimary,
                      k2: "量比",
                      v2: snap.volumeRatio.toFixed(2),
                      c2: T.textPrimary,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "总手",
                      v1: `${Math.round(snap.totalHand)}万`,
                      c1: T.accentGold,
                      k2: "金额",
                      v2: `${Math.round(snap.totalAmount)}亿`,
                      c2: T.accentBlue,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "两市额",
                      v1: `${Math.round(snap.marketTurnover)}亿`,
                      c1: T.accentBlue,
                      k2: "换手",
                      v2: `${snap.turnoverRate.toFixed(2)}%`,
                      c2: T.textPrimary,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "市盈动",
                      v1: snap.peDynamic.toFixed(2),
                      c1: T.textPrimary,
                      k2: "市净率",
                      v2: snap.pbRatio.toFixed(2),
                      c2: T.textPrimary,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "委买量",
                      v1: `${snap.weiBuyHand}万`,
                      c1: T.upRed,
                      k2: "委卖量",
                      v2: `${snap.weiSellHand}万`,
                      c2: T.downGreen,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(DepthCellRow, {
                      k1: "上涨",
                      v1: `${snap.upCount}家`,
                      c1: T.upRed,
                      k2: "下跌",
                      v2: `${snap.downCount}家`,
                      c2: T.downGreen,
                      dimColor: T.textDim
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this),
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  height: 20,
                  padding: { x: 6, y: 0 },
                  spacing: 0,
                  background: T.sectionHdrBg,
                  alignItems: "center",
                  children: [
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "个股贡献度 (点击切标的)",
                      fontSize: 11,
                      color: T.accentGold,
                      bold: true,
                      flexGrow: 1
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "涨幅%",
                      fontSize: 11,
                      color: T.textMuted,
                      width: 48,
                      height: 16
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "现价",
                      fontSize: 11,
                      color: T.textMuted,
                      width: 52,
                      height: 16
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "贡献",
                      fontSize: 11,
                      color: T.textMuted,
                      width: 42,
                      height: 16
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this),
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "column",
                  padding: { x: 4, y: 1 },
                  spacing: 0,
                  background: T.rankBg,
                  children: CONTRIB_RANKS.map((item) => {
                    const chgColor = item.changePct >= 0 ? T.upRed : T.downGreen;
                    const ctrColor = item.contribution >= 0 ? T.upRed : T.downGreen;
                    const isSelected = item.code === session.symbol.code;
                    return /* @__PURE__ */ jsxDEV(Panel, {
                      direction: "row",
                      height: 17,
                      padding: 0,
                      spacing: 0,
                      background: isSelected ? T.sectionHdrBg : "transparent",
                      alignItems: "center",
                      children: [
                        /* @__PURE__ */ jsxDEV(Button, {
                          text: item.name,
                          width: 64,
                          height: 16,
                          fontSize: 11,
                          background: "transparent",
                          foreground: isSelected ? T.accentGold : T.textPrimary,
                          onClick: () => selectSymbolByCode(item.code)
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Spacer, {
                          flexGrow: 1,
                          flexBasis: 0
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Label, {
                          text: `${item.changePct >= 0 ? "+" : ""}${item.changePct.toFixed(2)}%`,
                          fontSize: 11,
                          color: chgColor,
                          width: 48,
                          height: 16
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Label, {
                          text: item.price.toFixed(2),
                          fontSize: 11,
                          color: chgColor,
                          width: 52,
                          height: 16
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Label, {
                          text: `${item.contribution >= 0 ? "+" : ""}${item.contribution.toFixed(2)}`,
                          fontSize: 11,
                          color: ctrColor,
                          width: 42,
                          height: 16
                        }, undefined, false, undefined, this)
                      ]
                    }, item.code, true, undefined, this);
                  })
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  height: 20,
                  padding: { x: 6, y: 0 },
                  spacing: 0,
                  background: T.sectionHdrBg,
                  alignItems: "center",
                  children: [
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "短线精灵",
                      fontSize: 11,
                      color: T.upRed,
                      bold: true,
                      flexGrow: 1
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(Label, {
                      text: "A股逐笔",
                      fontSize: 11,
                      color: T.textMuted
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this),
                /* @__PURE__ */ jsxDEV(Panel, {
                  id: "tick_container",
                  direction: "column",
                  flexGrow: 1,
                  flexBasis: 0,
                  padding: { x: 6, y: 1 },
                  spacing: 0,
                  background: T.tickBg,
                  children: session.ticks.map((tk, idx) => {
                    const tkCol = tk.isBuy ? T.upRed : T.downGreen;
                    return /* @__PURE__ */ jsxDEV(Panel, {
                      direction: "row",
                      height: 18,
                      padding: 0,
                      spacing: 0,
                      alignItems: "center",
                      children: [
                        /* @__PURE__ */ jsxDEV(Label, {
                          text: tk.timeStr,
                          fontSize: 10,
                          color: tk.highlight ? T.upRed : T.textDim,
                          bold: Boolean(tk.highlight),
                          width: 56,
                          height: 16
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Label, {
                          text: tk.typeStr,
                          fontSize: 10,
                          color: tkCol,
                          bold: Boolean(tk.highlight),
                          width: 202,
                          height: 16
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Label, {
                          text: tk.volumeLabel,
                          fontSize: 10,
                          color: tkCol,
                          width: 56,
                          height: 16,
                          horizontalAlign: "end"
                        }, undefined, false, undefined, this)
                      ]
                    }, idx, true, undefined, this);
                  })
                }, undefined, false, undefined, this)
              ]
            }, undefined, true, undefined, this)
          ]
        }, undefined, true, undefined, this),
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          height: 26,
          padding: { x: 8, y: 0 },
          spacing: 10,
          background: T.statusBarBg,
          alignItems: "center",
          children: [
            /* @__PURE__ */ jsxDEV(Label, {
              text: "沪",
              fontSize: 11,
              color: T.textDim
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              id: "bottom_sh_price",
              text: snap.current.toFixed(2),
              fontSize: 11,
              color: priceColor,
              bold: true,
              width: 58,
              height: 18
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              id: "bottom_sh_diff",
              text: `${changeSign}${snap.change.toFixed(2)} (${changeSign}${snap.changePct.toFixed(2)}%)`,
              fontSize: 10,
              color: priceColor,
              width: 104,
              height: 18
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: `${Math.round(snap.totalAmount)}亿`,
              fontSize: 10,
              color: T.accentBlue,
              width: 48,
              height: 18
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "深",
              fontSize: 11,
              color: T.textDim
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "14718.66",
              fontSize: 11,
              color: T.upRed,
              bold: true
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "+47.95 (+0.33%)",
              fontSize: 10,
              color: T.upRed
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "5606亿",
              fontSize: 10,
              color: T.accentBlue
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "创",
              fontSize: 11,
              color: T.textDim
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "3352.12",
              fontSize: 11,
              color: T.upRed,
              bold: true
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "+18.22 (+0.55%)",
              fontSize: 10,
              color: T.upRed
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "科",
              fontSize: 11,
              color: T.textDim
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "1575.89",
              fontSize: 11,
              color: T.upRed,
              bold: true
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              text: "+37.73 (+2.45%)",
              fontSize: 10,
              color: T.upRed
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Spacer, {
              flexGrow: 1,
              flexBasis: 0
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(TextInput, {
              id: "genie_input",
              text: searchQuery,
              placeholder: "代码/简拼/功能",
              width: 126,
              height: 20,
              fontSize: 11,
              background: T.inputBg,
              color: T.textPrimary,
              placeholderColor: T.btnDimText,
              borderColor: T.popupBorder,
              onTextChanged: setSearchQuery,
              onSubmit: () => {
                if (genieMatches.length > 0)
                  selectSymbolByCode(genieMatches[0].code);
              }
            }, undefined, false, undefined, this)
          ]
        }, undefined, true, undefined, this)
      ]
    }, undefined, true, undefined, this);
  }
  render(/* @__PURE__ */ jsxDEV(StockTerminalApp, {}, undefined, false, undefined, this));
})();
