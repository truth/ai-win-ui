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

  // react/demos/bench_stress/index.tsx
  var exports_bench_stress = {};
  __export(exports_bench_stress, {
    BenchStressApp: () => BenchStressApp
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
  var Label = "Label";
  var TextInput = "TextInput";
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
  // react/demos/bench_stress/index.tsx
  var ROWS = 10;
  var COLS = 10;
  function computeCells(tick) {
    const values = new Array(100);
    let sum = 0;
    for (let i = 0;i < 100; i++) {
      const v = 100 + (tick * 3 + i) % 100 * 0.13;
      sum += v;
      values[i] = v.toFixed(2);
    }
    return { values, sum };
  }
  function BenchStressApp() {
    const [tick, setTick] = useState(0);
    const [inputText, setInputText] = useState("");
    useEffect(() => {
      setWindowTitle("React 60Hz Stress Board (100 Cells + Rapid Typing)");
    }, []);
    useEffect(() => {
      const id = setInterval(() => {
        setTick((t) => t + 1);
      }, 16);
      return () => clearInterval(id);
    }, []);
    const board = computeCells(tick);
    return /* @__PURE__ */ jsxDEV(Panel, {
      direction: "column",
      padding: 10,
      spacing: 6,
      background: "#0B111A",
      alignItems: "stretch",
      children: [
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "row",
          height: 32,
          padding: 0,
          spacing: 10,
          alignItems: "center",
          children: [
            /* @__PURE__ */ jsxDEV(Label, {
              text: "XML vs React 60Hz Stress Board (100 Fixed Cells + Rapid Typing)",
              fontSize: 14,
              color: "#E2E8F0",
              bold: true,
              width: 430,
              height: 22
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(Label, {
              id: "stress_tick_label",
              text: `Ticks: ${tick} | Sum: ${board.sum.toFixed(2)}`,
              fontSize: 13,
              color: "#38BDF8",
              width: 210,
              height: 22
            }, undefined, false, undefined, this),
            /* @__PURE__ */ jsxDEV(TextInput, {
              id: "stress_input",
              text: inputText,
              placeholder: "Type rapid characters...",
              width: 300,
              height: 26,
              fontSize: 12,
              background: "#162032",
              color: "#F8FAFC",
              borderColor: "#334155",
              onTextChanged: setInputText
            }, undefined, false, undefined, this)
          ]
        }, undefined, true, undefined, this),
        /* @__PURE__ */ jsxDEV(Panel, {
          direction: "column",
          padding: 4,
          spacing: 4,
          background: "#0F172A",
          alignItems: "stretch",
          children: Array.from({ length: ROWS }, (_, r) => /* @__PURE__ */ jsxDEV(Panel, {
            direction: "row",
            height: 24,
            padding: 0,
            spacing: 4,
            alignItems: "center",
            children: Array.from({ length: COLS }, (_2, c) => {
              const idx = r * COLS + c;
              return /* @__PURE__ */ jsxDEV(Label, {
                id: `cell_${idx}`,
                text: board.values[idx],
                fontSize: 12,
                color: "#22C55E",
                width: 92,
                height: 22
              }, c, false, undefined, this);
            })
          }, r, false, undefined, this))
        }, undefined, false, undefined, this)
      ]
    }, undefined, true, undefined, this);
  }
  render(/* @__PURE__ */ jsxDEV(BenchStressApp, {}, undefined, false, undefined, this));
})();
