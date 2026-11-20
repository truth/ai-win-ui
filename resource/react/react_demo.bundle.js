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

  // react/demos/react_showcase.tsx
  var exports_react_showcase = {};
  __export(exports_react_showcase, {
    App: () => App
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
  // react/runtime/components.ts
  var Panel = "Panel";
  var ScrollViewer = "ScrollViewer";
  var Label = "Label";
  var Button = "Button";
  var TextInput = "TextInput";
  var Switch = "Switch";
  var Slider = "Slider";
  var Checkbox = "Checkbox";
  var ProgressBar = "ProgressBar";
  var Badge = "Badge";
  var Avatar = "Avatar";
  var Divider = "Divider";
  var Icon = "Icon";
  var Breadcrumb = "Breadcrumb";
  var BreadcrumbItem = "BreadcrumbItem";
  var Pagination = "Pagination";
  var TreeView = "TreeView";
  var TreeItem = "TreeItem";
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
  // react/demos/react_showcase.tsx
  var EXPLORER_SECTIONS = [
    { key: "overview", label: "Architecture" },
    { key: "form", label: "Optimistic Form" },
    { key: "tasks", label: "Keyed Lists" },
    { key: "perf", label: "144Hz Stress" }
  ];
  var INITIAL_TASKS = [
    { id: 1, title: "Zero RNW JS bridge bloat (QuickJS-ng C API)", done: true, priority: "high" },
    { id: 2, title: "Dual-thread CommitQueue + PostMessage batching", done: true, priority: "high" },
    { id: 3, title: "Optimistic UI input + eventSeq echo guard", done: true, priority: "normal" },
    { id: 4, title: "Typed Thickness {x, y} eliminating L,T,R,B bugs", done: true, priority: "normal" },
    { id: 5, title: "Keyed VNode diff straight onto C++ UIElement", done: false, priority: "high" },
    { id: 6, title: "Bun build millisecond TSX bundle pipeline", done: true, priority: "normal" },
    { id: 7, title: "Skia & Direct2D dual-backend validation", done: false, priority: "normal" }
  ];
  function App() {
    const [activeSection, setActiveSection] = useState("Architecture");
    const [selectedCount, setSelectedCount] = useState(3);
    const [inputText, setInputText] = useState("Type fast here - 0ms optimistic input!");
    const [lastSubmitted, setLastSubmitted] = useState("(press Enter to submit)");
    const [darkHighlight, setDarkHighlight] = useState(true);
    const [agreeFlag, setAgreeFlag] = useState(true);
    const [progress, setProgress] = useState(64);
    const [tasks, setTasks] = useState(INITIAL_TASKS);
    const [nextTaskId, setNextTaskId] = useState(8);
    const [onlyOpen, setOnlyOpen] = useState(false);
    const [page, setPage] = useState(1);
    const pageSize = 4;
    const [tickerOn, setTickerOn] = useState(false);
    const [tickCount, setTickCount] = useState(0);
    const [jsBusyMs, setJsBusyMs] = useState(0);
    useEffect(() => {
      setWindowTitle(`ai-win-ui · React JSX Showcase (${activeSection})`);
    }, [activeSection]);
    useEffect(() => {
      if (!tickerOn)
        return;
      const id = setInterval(() => {
        setTickCount((c) => c + 1);
        setProgress((p) => p >= 100 ? 0 : p + 2);
      }, 16);
      return () => clearInterval(id);
    }, [tickerOn]);
    const filteredTasks = useMemo(() => {
      return onlyOpen ? tasks.filter((t) => !t.done) : tasks;
    }, [tasks, onlyOpen]);
    const totalPages = Math.max(1, Math.ceil(filteredTasks.length / pageSize));
    const safePage = Math.min(page, totalPages);
    const pageTasks = useMemo(() => {
      const start = (safePage - 1) * pageSize;
      return filteredTasks.slice(start, start + pageSize);
    }, [filteredTasks, safePage]);
    const addTask = () => {
      const id = nextTaskId;
      setNextTaskId(id + 1);
      setTasks((prev) => [
        { id, title: `Dynamic Task #${id} (key=${id})`, done: false, priority: id % 2 === 0 ? "high" : "normal" },
        ...prev
      ]);
      setPage(1);
    };
    const toggleTask = (id) => {
      setTasks((prev) => prev.map((t) => t.id === id ? { ...t, done: !t.done } : t));
    };
    const removeTask = (id) => {
      setTasks((prev) => prev.filter((t) => t.id !== id));
    };
    const reverseTasks = () => {
      setTasks((prev) => [...prev].reverse());
    };
    const runJsCpuBurst = () => {
      const t0 = performance.now();
      let acc = 0;
      for (let i = 0;i < 800000; i++) {
        acc += i * 17 % 13;
      }
      const elapsed = Math.round(performance.now() - t0);
      setJsBusyMs(elapsed + (acc >= 0 ? 0 : 1));
    };
    const accentHex = darkHighlight ? "#3B82F6" : "#10B981";
    return /* @__PURE__ */ jsxDEV(Panel, {
      id: "reactRoot",
      direction: "column",
      background: "#0B121A",
      padding: { x: 18, y: 16 },
      spacing: 12,
      alignItems: "stretch",
      children: [
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          height: 48,
          background: "#121D2C",
          borderColor: "#22344A",
          border: 1,
          cornerRadius: 10,
          padding: { x: 16, y: 8 },
          alignItems: "center",
          justifyContent: "space-between",
          children: [
            /* @__PURE__ */ jsxDEV(Panel, {
              direction: "row",
              spacing: 10,
              alignItems: "center",
              children: [
                /* @__PURE__ */ jsxDEV(Icon, {
                  name: "code",
                  size: 18,
                  color: accentHex
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Label, {
                  fontSize: 15,
                  bold: true,
                  color: "#F8FAFC",
                  children: "ai-win-ui React TSX Runtime"
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Badge, {
                  text: "QuickJS-ng",
                  variant: "primary",
                  dot: true
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Badge, {
                  text: "Zero RNW Bloat",
                  variant: "success"
                }, undefined, false, undefined, this)
              ]
            }, undefined, true, undefined, this),
            /* @__PURE__ */ jsxDEV(Panel, {
              direction: "row",
              spacing: 12,
              alignItems: "center",
              children: [
                /* @__PURE__ */ jsxDEV(Breadcrumb, {
                  separator: "/",
                  spacing: 8,
                  fontSize: 12.5,
                  separatorColor: "#64748B",
                  children: [
                    /* @__PURE__ */ jsxDEV(BreadcrumbItem, {
                      text: "Workspace",
                      icon: "home",
                      color: "#94A3B8",
                      hoverColor: "#38BDF8",
                      onClick: () => setActiveSection("Workspace")
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(BreadcrumbItem, {
                      text: "React Engine",
                      icon: "folder",
                      color: "#94A3B8",
                      hoverColor: "#38BDF8",
                      onClick: () => setActiveSection("React Engine")
                    }, undefined, false, undefined, this),
                    /* @__PURE__ */ jsxDEV(BreadcrumbItem, {
                      text: activeSection,
                      color: "#F8FAFC",
                      active: true
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this),
                /* @__PURE__ */ jsxDEV(Divider, {
                  vertical: true,
                  height: 18,
                  color: "#263A50"
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(Avatar, {
                  initials: "QJ",
                  size: 28,
                  status: "online",
                  background: "#1E3A8A",
                  color: "#60A5FA"
                }, undefined, false, undefined, this)
              ]
            }, undefined, true, undefined, this)
          ]
        }, undefined, true, undefined, this),
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          flexGrow: 1,
          spacing: 12,
          alignItems: "stretch",
          children: [
            /* @__PURE__ */ jsxDEV(Panel, {
              direction: "column",
              width: 260,
              background: "#121D2C",
              borderColor: "#22344A",
              border: 1,
              cornerRadius: 10,
              padding: { x: 12, y: 12 },
              spacing: 8,
              alignItems: "stretch",
              children: [
                /* @__PURE__ */ jsxDEV(Panel, {
                  direction: "row",
                  justifyContent: "space-between",
                  alignItems: "center",
                  height: 24,
                  children: [
                    /* @__PURE__ */ jsxDEV(Panel, {
                      direction: "row",
                      spacing: 6,
                      alignItems: "center",
                      children: [
                        /* @__PURE__ */ jsxDEV(Icon, {
                          name: "folder",
                          size: 14,
                          color: "#F59E0B"
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(Label, {
                          fontSize: 12,
                          color: "#94A3B8",
                          bold: true,
                          children: "NATIVE TREE"
                        }, undefined, false, undefined, this)
                      ]
                    }, undefined, true, undefined, this),
                    /* @__PURE__ */ jsxDEV(Badge, {
                      text: `${selectedCount} checked`,
                      variant: "info"
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this),
                /* @__PURE__ */ jsxDEV(Divider, {
                  color: "#22344A"
                }, undefined, false, undefined, this),
                /* @__PURE__ */ jsxDEV(TreeView, {
                  id: "navTree",
                  flexGrow: 1,
                  showLines: true,
                  showCheckBox: true,
                  background: "transparent",
                  borderColor: "#22344A",
                  selectedBackground: "#1E3A5F",
                  hoverBackground: "#16273C",
                  color: "#E2E8F0",
                  lineColor: "#263A50",
                  glyphColor: "#94A3B8",
                  itemHeight: 28,
                  indentWidth: 18,
                  fontSize: 12.5,
                  onSelect: (label) => setActiveSection(label),
                  onCheck: (checked) => setSelectedCount((c) => Math.max(0, c + (checked ? 1 : -1))),
                  children: [
                    /* @__PURE__ */ jsxDEV(TreeItem, {
                      header: "react-runtime",
                      icon: "folder",
                      expanded: true,
                      children: [
                        /* @__PURE__ */ jsxDEV(TreeItem, {
                          header: "QuickJS Engine",
                          icon: "database",
                          checked: true
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(TreeItem, {
                          header: "MutationBatch",
                          icon: "code",
                          checked: true
                        }, undefined, false, undefined, this),
                        /* @__PURE__ */ jsxDEV(TreeItem, {
                          header: "Micro Reconciler",
                          icon: "code",
                          checked: true
                        }, undefined, false, undefined, this)
                      ]
                    }, undefined, true, undefined, this),
                    /* @__PURE__ */ jsxDEV(TreeItem, {
                      header: "Views & Demos",
                      icon: "folder",
                      expanded: true,
                      children: EXPLORER_SECTIONS.map((s) => /* @__PURE__ */ jsxDEV(TreeItem, {
                        header: s.label,
                        icon: "file"
                      }, s.key, false, undefined, this))
                    }, undefined, false, undefined, this)
                  ]
                }, undefined, true, undefined, this)
              ]
            }, undefined, true, undefined, this),
            /* @__PURE__ */ jsxDEV(ScrollViewer, {
              flexGrow: 1,
              scrollY: true,
              background: "transparent",
              children: /* @__PURE__ */ jsxDEV(Panel, {
                direction: "column",
                spacing: 12,
                padding: { right: 12 },
                alignItems: "stretch",
                children: [
                  /* @__PURE__ */ jsxDEV(Panel, {
                    direction: "column",
                    background: "#121D2C",
                    borderColor: "#22344A",
                    border: 1,
                    cornerRadius: 10,
                    padding: { x: 16, y: 14 },
                    spacing: 10,
                    alignItems: "stretch",
                    children: [
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        justifyContent: "space-between",
                        alignItems: "center",
                        height: 24,
                        children: [
                          /* @__PURE__ */ jsxDEV(Label, {
                            fontSize: 14,
                            bold: true,
                            color: "#F8FAFC",
                            children: "1. Optimistic UI Input + Dual-Thread Echo Suppression (eventSeq)"
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(Label, {
                            width: 140,
                            height: 22,
                            fontSize: 12,
                            color: "#60A5FA",
                            horizontalAlign: "end",
                            children: [
                              "Chars: ",
                              inputText.length
                            ]
                          }, undefined, true, undefined, this)
                        ]
                      }, undefined, true, undefined, this),
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        spacing: 10,
                        alignItems: "center",
                        children: [
                          /* @__PURE__ */ jsxDEV(TextInput, {
                            id: "optTextInput",
                            flexGrow: 1,
                            height: 34,
                            text: inputText,
                            placeholder: "Type anything...",
                            background: "#0E1724",
                            borderColor: "#2A3E59",
                            focusBorderColor: accentHex,
                            color: "#F1F5F9",
                            cornerRadius: 6,
                            fontSize: 13,
                            onTextChanged: (txt) => setInputText(txt),
                            onSubmit: (txt) => setLastSubmitted(txt)
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(Button, {
                            text: "Clear Text",
                            width: 100,
                            height: 34,
                            background: "#1E293B",
                            foreground: "#F8FAFC",
                            cornerRadius: 6,
                            fontSize: 12.5,
                            onClick: () => setInputText("")
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(Button, {
                            text: "Preset",
                            width: 84,
                            height: 34,
                            background: accentHex,
                            foreground: "#FFFFFF",
                            cornerRadius: 6,
                            fontSize: 12.5,
                            onClick: () => setInputText("Hello QuickJS + Yoga!")
                          }, undefined, false, undefined, this)
                        ]
                      }, undefined, true, undefined, this),
                      /* @__PURE__ */ jsxDEV(Label, {
                        fontSize: 12,
                        color: "#94A3B8",
                        children: [
                          "Last Enter Submit: ",
                          lastSubmitted
                        ]
                      }, undefined, true, undefined, this),
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        spacing: 16,
                        alignItems: "center",
                        height: 36,
                        children: [
                          /* @__PURE__ */ jsxDEV(Switch, {
                            checked: darkHighlight,
                            label: darkHighlight ? "Accent: Blue" : "Accent: Emerald",
                            activeColor: accentHex,
                            inactiveColor: "#334155",
                            color: "#E2E8F0",
                            onChange: (v) => setDarkHighlight(v)
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(Checkbox, {
                            checked: agreeFlag,
                            text: "Batch Zero-Layout Updates",
                            color: "#E2E8F0",
                            fontSize: 13,
                            onChange: (v) => setAgreeFlag(v)
                          }, undefined, false, undefined, this)
                        ]
                      }, undefined, true, undefined, this),
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        spacing: 12,
                        alignItems: "center",
                        height: 42,
                        children: [
                          /* @__PURE__ */ jsxDEV(Slider, {
                            flexGrow: 1,
                            min: 0,
                            max: 100,
                            step: 1,
                            value: progress,
                            text: "Slider",
                            background: "#0E1724",
                            borderColor: "#2A3E59",
                            highlightColor: accentHex,
                            color: "#E2E8F0",
                            onChange: (val) => setProgress(Math.round(val))
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(ProgressBar, {
                            width: 260,
                            height: 32,
                            min: 0,
                            max: 100,
                            value: progress,
                            showValueText: true,
                            background: "#0E1724",
                            borderColor: "#2A3E59",
                            trackColor: "#1E293B",
                            fillColor: accentHex,
                            color: "#F8FAFC"
                          }, undefined, false, undefined, this)
                        ]
                      }, undefined, true, undefined, this)
                    ]
                  }, undefined, true, undefined, this),
                  /* @__PURE__ */ jsxDEV(Panel, {
                    direction: "column",
                    background: "#121D2C",
                    borderColor: "#22344A",
                    border: 1,
                    cornerRadius: 10,
                    padding: { x: 16, y: 14 },
                    spacing: 10,
                    alignItems: "stretch",
                    children: [
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        justifyContent: "space-between",
                        alignItems: "center",
                        height: 32,
                        children: [
                          /* @__PURE__ */ jsxDEV(Label, {
                            fontSize: 14,
                            bold: true,
                            color: "#F8FAFC",
                            children: [
                              "2. Keyed Reconciliation & Pagination (",
                              filteredTasks.length,
                              " items)"
                            ]
                          }, undefined, true, undefined, this),
                          /* @__PURE__ */ jsxDEV(Panel, {
                            direction: "row",
                            spacing: 8,
                            alignItems: "center",
                            children: [
                              /* @__PURE__ */ jsxDEV(Checkbox, {
                                checked: onlyOpen,
                                text: "Only Open",
                                color: "#CBD5E1",
                                fontSize: 12.5,
                                onChange: (v) => {
                                  setOnlyOpen(v);
                                  setPage(1);
                                }
                              }, undefined, false, undefined, this),
                              /* @__PURE__ */ jsxDEV(Button, {
                                text: "Reverse",
                                width: 78,
                                height: 28,
                                background: "#1E293B",
                                foreground: "#E2E8F0",
                                cornerRadius: 6,
                                fontSize: 12,
                                onClick: reverseTasks
                              }, undefined, false, undefined, this),
                              /* @__PURE__ */ jsxDEV(Button, {
                                text: "+ Add Item",
                                width: 96,
                                height: 28,
                                background: accentHex,
                                foreground: "#FFFFFF",
                                cornerRadius: 6,
                                fontSize: 12,
                                onClick: addTask
                              }, undefined, false, undefined, this)
                            ]
                          }, undefined, true, undefined, this)
                        ]
                      }, undefined, true, undefined, this),
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "column",
                        spacing: 6,
                        alignItems: "stretch",
                        children: pageTasks.map((task) => /* @__PURE__ */ jsxDEV(Panel, {
                          direction: "row",
                          height: 36,
                          background: "#0E1724",
                          borderColor: "#1E2D40",
                          border: 1,
                          cornerRadius: 6,
                          padding: { x: 10, y: 4 },
                          alignItems: "center",
                          justifyContent: "space-between",
                          children: [
                            /* @__PURE__ */ jsxDEV(Panel, {
                              direction: "row",
                              spacing: 8,
                              alignItems: "center",
                              flexGrow: 1,
                              children: /* @__PURE__ */ jsxDEV(Checkbox, {
                                checked: task.done,
                                text: task.title,
                                color: task.done ? "#64748B" : "#E2E8F0",
                                fontSize: 12.5,
                                onChange: () => toggleTask(task.id)
                              }, undefined, false, undefined, this)
                            }, undefined, false, undefined, this),
                            /* @__PURE__ */ jsxDEV(Panel, {
                              direction: "row",
                              spacing: 8,
                              alignItems: "center",
                              children: [
                                /* @__PURE__ */ jsxDEV(Badge, {
                                  text: task.priority,
                                  variant: task.priority === "high" ? "warning" : "neutral"
                                }, undefined, false, undefined, this),
                                /* @__PURE__ */ jsxDEV(Button, {
                                  text: "Delete",
                                  width: 60,
                                  height: 24,
                                  background: "#3F1D2B",
                                  foreground: "#FDA4AF",
                                  cornerRadius: 5,
                                  fontSize: 11.5,
                                  onClick: () => removeTask(task.id)
                                }, undefined, false, undefined, this)
                              ]
                            }, undefined, true, undefined, this)
                          ]
                        }, task.id, true, undefined, this))
                      }, undefined, false, undefined, this),
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        justifyContent: "end",
                        alignItems: "center",
                        height: 34,
                        children: /* @__PURE__ */ jsxDEV(Pagination, {
                          current: safePage,
                          total: filteredTasks.length,
                          pageSize,
                          showTotal: true,
                          activeColor: accentHex,
                          color: "#CBD5E1",
                          borderColor: "#263A50",
                          hoverColor: "#1E293B",
                          onPageChange: (nextPage) => setPage(nextPage)
                        }, undefined, false, undefined, this)
                      }, undefined, false, undefined, this)
                    ]
                  }, undefined, true, undefined, this),
                  /* @__PURE__ */ jsxDEV(Panel, {
                    direction: "column",
                    background: "#121D2C",
                    borderColor: "#22344A",
                    border: 1,
                    cornerRadius: 10,
                    padding: { x: 16, y: 14 },
                    spacing: 10,
                    alignItems: "stretch",
                    children: [
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        justifyContent: "space-between",
                        alignItems: "center",
                        height: 28,
                        children: [
                          /* @__PURE__ */ jsxDEV(Label, {
                            fontSize: 14,
                            bold: true,
                            color: "#F8FAFC",
                            children: "3. Dual-Thread 60Hz Visual-Only Fast Path (Press F12 for Profiler)"
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(Label, {
                            width: 140,
                            height: 22,
                            fontSize: 12.5,
                            color: "#34D399",
                            horizontalAlign: "end",
                            children: [
                              "Ticks: ",
                              tickCount
                            ]
                          }, undefined, true, undefined, this)
                        ]
                      }, undefined, true, undefined, this),
                      /* @__PURE__ */ jsxDEV(Panel, {
                        direction: "row",
                        spacing: 12,
                        alignItems: "center",
                        children: [
                          /* @__PURE__ */ jsxDEV(Switch, {
                            checked: tickerOn,
                            label: tickerOn ? "60Hz Ticker RUNNING" : "Start 60Hz Ticker",
                            activeColor: "#10B981",
                            inactiveColor: "#334155",
                            color: "#E2E8F0",
                            onChange: (v) => setTickerOn(v)
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(Button, {
                            text: "Run Heavy JS Burst (UI Stays Smooth)",
                            width: 260,
                            height: 32,
                            background: "#1E293B",
                            foreground: "#F8FAFC",
                            cornerRadius: 6,
                            fontSize: 12,
                            onClick: runJsCpuBurst
                          }, undefined, false, undefined, this),
                          /* @__PURE__ */ jsxDEV(Label, {
                            width: 200,
                            height: 22,
                            fontSize: 12,
                            color: "#94A3B8",
                            children: [
                              "Last JS Burst: ",
                              jsBusyMs,
                              " ms"
                            ]
                          }, undefined, true, undefined, this)
                        ]
                      }, undefined, true, undefined, this)
                    ]
                  }, undefined, true, undefined, this)
                ]
              }, undefined, true, undefined, this)
            }, undefined, false, undefined, this)
          ]
        }, undefined, true, undefined, this)
      ]
    }, undefined, true, undefined, this);
  }
  render(/* @__PURE__ */ jsxDEV(App, {}, undefined, false, undefined, this));
})();
