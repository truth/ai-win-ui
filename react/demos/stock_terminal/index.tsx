import {
  Button,
  FinancialChart,
  Label,
  Panel,
  Spacer,
  TextInput,
  render,
  setWindowTitle,
  useEffect,
  useMemo,
  useState,
  useWindowKeyDown,
} from "ai-win-ui";
import {
  CONTRIB_RANKS,
  NEWS_BY_TAB,
  NEWS_TABS,
  STOCK_DATABASE,
  createInitialSession,
  getOrGenerateCandles,
  querySymbols,
  stepMarketTick,
  type ChartPeriodKey,
  type StockSymbol,
  type SymbolSession,
} from "./market_engine.ts";
import { CLASSIC_THEME, MODERN_THEME } from "./theme.ts";

const TOOLBAR_ACTIONS = [
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
  "创/科",
];

const MARKET_CATEGORIES = ["全景", "个股", "指数", "期指", "期权", "基金", "外汇", "美股", "自定", "多窗"];

const PERIOD_BUTTONS: { key: ChartPeriodKey; label: string; width: number; capsule: string }[] = [
  { key: "timeshare", label: "分时", width: 40, capsule: "分时" },
  { key: "day", label: "日K", width: 36, capsule: "日线 ▼" },
  { key: "week", label: "周K", width: 36, capsule: "周线 ▼" },
  { key: "month", label: "月K", width: 36, capsule: "月线 ▼" },
  { key: "m1", label: "1分", width: 32, capsule: "1分 ▼" },
  { key: "m5", label: "5分", width: 32, capsule: "5分 ▼" },
  { key: "m15", label: "15分", width: 36, capsule: "15分 ▼" },
  { key: "m30", label: "30分", width: 36, capsule: "30分 ▼" },
  { key: "m60", label: "60分", width: 36, capsule: "60分 ▼" },
];

function DepthCellRow(props: {
  k1: string;
  v1: string;
  c1: string;
  k2: string;
  v2: string;
  c2: string;
  dimColor: string;
}) {
  return (
    <Panel direction="row" height={18} padding={0} spacing={0} alignItems="center">
      <Label text={props.k1} fontSize={11} color={props.dimColor} width={42} height={17} />
      <Label text={props.v1} fontSize={11} color={props.c1} width={112} height={17} />
      <Label text={props.k2} fontSize={11} color={props.dimColor} width={42} height={17} />
      <Label text={props.v2} fontSize={11} color={props.c2} width={118} height={17} />
    </Panel>
  );
}

export function StockTerminalApp() {
  const [isModernTheme, setIsModernTheme] = useState(false);
  const [period, setPeriod] = useState<ChartPeriodKey>("timeshare");
  const [livePaused, setLivePaused] = useState(false);
  const [activeMarketTab, setActiveMarketTab] = useState("指数");
  const [activeNewsTab, setActiveNewsTab] = useState<string>("股市直播");
  const [searchQuery, setSearchQuery] = useState("");
  const [statusNotice, setStatusNotice] = useState("实时行情引擎就绪 [T 切换主题 · F5 刷新]");
  const [session, setSession] = useState<SymbolSession>(() => createInitialSession(STOCK_DATABASE[0]));

  const T = isModernTheme ? MODERN_THEME : CLASSIC_THEME;

  // Sync native Win32 window title when symbol or theme changes (NOT on 500ms ticks).
  useEffect(() => {
    setWindowTitle(
      `同花顺金融终端 (React JSX) - ${session.symbol.name} ${session.symbol.code} [${isModernTheme ? "Modern" : "Classic"}]`
    );
  }, [session.symbol.code, session.symbol.name, isModernTheme]);

  // 500ms real-time tick generator.
  useEffect(() => {
    if (livePaused) return;
    const id = setInterval(() => {
      setSession((prev) => stepMarketTick(prev));
    }, 500);
    return () => clearInterval(id);
  }, [livePaused]);

  const selectSymbolByCode = (code: string) => {
    const found = STOCK_DATABASE.find((s) => s.code === code);
    if (!found) return;
    setSession(createInitialSession(found));
    setSearchQuery("");
    setStatusNotice(`已切换标的：${found.name} (${found.code})`);
  };

  // Window hotkeys: T (theme), F5 (reset intraday), Escape (clear search).
  useWindowKeyDown((keyCode, isTextInputFocused) => {
    if (keyCode === 0x54 && !isTextInputFocused) {
      setIsModernTheme((v) => !v);
    } else if (keyCode === 0x74) {
      setSession((prev) => createInitialSession(prev.symbol));
      setStatusNotice("已刷新日内分时基准数据 (F5)");
    } else if (keyCode === 0x1b) {
      setSearchQuery("");
    }
  });

  const genieMatches = useMemo(() => querySymbols(searchQuery, 6), [searchQuery]);

  const snap = session.snapshot;
  const priceUp = snap.change >= 0;
  const priceColor = priceUp ? T.upRed : T.downGreen;
  const changeSign = snap.change >= 0 ? "+" : "";
  const activePeriodMeta = PERIOD_BUTTONS.find((p) => p.key === period) ?? PERIOD_BUTTONS[0];
  const activeCandles =
    period === "timeshare" ? undefined : getOrGenerateCandles(session, period);
  const newsSet = NEWS_BY_TAB[activeNewsTab] ?? NEWS_BY_TAB["股市直播"];

  return (
    <Panel direction="column" padding={0} spacing={0} background={T.rootBg} alignItems="stretch">
      {/* 1. Top Menu Bar */}
      <Panel direction="row" height={28} padding={{ x: 10, y: 0 }} spacing={10} background={T.menuBg} alignItems="center">
        <Label text="同花顺" fontSize={13} color={T.upRed} bold />
        <Label text={`• ${session.symbol.name}`} fontSize={12} color={T.textPrimary} width={96} height={18} />
        <Spacer width={6} />
        {["系统", "报价", "行情", "分析", "交易", "智能", "工具", "资讯", "帮助"].map((m) => (
          <Label key={m} text={m} fontSize={12} color={T.textSecondary} />
        ))}
        <Label text="决策" fontSize={12} color={T.upRed} bold />
        <Spacer flexGrow={1} flexBasis={0} />
        <Label text={statusNotice} fontSize={11} color={T.accentGold} width={250} height={18} horizontalAlign="end" />
        <Spacer width={6} />
        <TextInput
          id="top_search"
          text={searchQuery}
          placeholder="代码/简拼"
          width={92}
          height={20}
          fontSize={11}
          background={T.inputBg}
          color={T.textPrimary}
          placeholderColor={T.btnDimText}
          borderColor={T.popupBorder}
          onTextChanged={setSearchQuery}
          onSubmit={() => {
            if (genieMatches.length > 0) selectSymbolByCode(genieMatches[0].code);
          }}
        />
        <Button
          text="资讯"
          width={38}
          height={20}
          fontSize={11}
          background={T.infoBtnBg}
          foreground={T.infoBtnFg}
          onClick={() => setActiveNewsTab("股市直播")}
        />
        <Button
          text="委托"
          width={38}
          height={20}
          fontSize={11}
          background={T.orderBtnBg}
          foreground={T.orderBtnFg}
          onClick={() => setStatusNotice("模拟委托通道已开启 [仅限演示]")}
        />
      </Panel>

      {/* 2. Quick Action Toolbar */}
      <Panel direction="row" height={30} padding={{ x: 8, y: 0 }} spacing={4} background={T.toolbarBg} alignItems="center">
        <Button
          text="买入"
          width={42}
          height={22}
          fontSize={11}
          background={T.buyBtnBg}
          foreground="#FFFFFF"
          onClick={() => setStatusNotice(`模拟买入申报: ${session.symbol.code} @ ${snap.current.toFixed(2)}`)}
        />
        <Button
          text="卖出"
          width={42}
          height={22}
          fontSize={11}
          background={T.sellBtnBg}
          foreground="#FFFFFF"
          onClick={() => setStatusNotice(`模拟卖出申报: ${session.symbol.code} @ ${snap.current.toFixed(2)}`)}
        />
        {TOOLBAR_ACTIONS.map((act) => (
          <Button
            key={act}
            text={act}
            width={act.length > 2 ? 44 : 40}
            height={22}
            fontSize={11}
            background={T.btnBg}
            foreground={T.btnText}
            onClick={() => setStatusNotice(`功能[${act}] - 当前标的 ${session.symbol.name}`)}
          />
        ))}
        <Spacer flexGrow={1} flexBasis={0} />
        <Button
          text={livePaused ? "继续行情" : "暂停Tick"}
          width={68}
          height={22}
          fontSize={11}
          background={livePaused ? T.orderBtnBg : T.btnBg}
          foreground={livePaused ? T.orderBtnFg : T.downGreen}
          onClick={() => setLivePaused((p) => !p)}
        />
        <Button
          id="btn_toggle_theme"
          text="双主题切换 (T)"
          width={98}
          height={22}
          fontSize={11}
          background={T.themeBtnBg}
          foreground={T.accentGold}
          onClick={() => setIsModernTheme((v) => !v)}
        />
      </Panel>

      {/* 3. Market Category Tabs */}
      <Panel direction="row" height={24} padding={{ x: 6, y: 0 }} spacing={3} background={T.tabBarBg} alignItems="center">
        {MARKET_CATEGORIES.map((tab) => {
          const active = tab === activeMarketTab;
          return (
            <Button
              key={tab}
              text={tab}
              width={38}
              height={20}
              fontSize={11}
              background={active ? T.buyBtnBg : T.btnBg}
              foreground={active ? "#FFFFFF" : T.textSecondary}
              onClick={() => {
                setActiveMarketTab(tab);
                if (tab === "指数") selectSymbolByCode("1A0001");
                else if (tab === "个股") selectSymbolByCode("600519");
              }}
            />
          );
        })}
      </Panel>

      {/* 4. Main Workspace */}
      <Panel direction="row" flexGrow={1} flexBasis={0} padding={0} spacing={0} alignItems="stretch">
        {/* Left Vertical Quick Strip */}
        <Panel direction="column" width={28} padding={2} spacing={2} background={T.sidebarStripBg} alignItems="center">
          <Button text="首页" width={24} height={26} fontSize={10} background={T.buyBtnBg} foreground="#FFFFFF" onClick={() => selectSymbolByCode("1A0001")} />
          <Button
            id="left_btn_timeshare"
            text="分时"
            width={24}
            height={26}
            fontSize={10}
            background={period === "timeshare" ? T.buyBtnBg : T.btnBg}
            foreground="#EEEEEE"
            onClick={() => setPeriod("timeshare")}
          />
          <Button
            id="left_btn_kline"
            text="K线"
            width={24}
            height={26}
            fontSize={10}
            background={period !== "timeshare" ? T.buyBtnBg : T.btnBg}
            foreground="#EEEEEE"
            onClick={() => setPeriod("day")}
          />
          {["应用", "指数", "大盘", "自选", "排名", "更多"].map((lbl) => (
            <Button
              key={lbl}
              text={lbl}
              width={24}
              height={26}
              fontSize={10}
              background={T.btnBg}
              foreground={T.btnDimText}
              onClick={() => setStatusNotice(`快捷入口：${lbl}`)}
            />
          ))}
        </Panel>

        {/* Center Chart + News Workspace */}
        <Panel direction="column" flexGrow={1} flexBasis={0} padding={0} spacing={0} alignItems="stretch">
          {/* Period Bar */}
          <Panel direction="row" height={26} padding={{ x: 6, y: 0 }} spacing={3} background={T.subBarBg} alignItems="center">
            {PERIOD_BUTTONS.map((p) => {
              const active = period === p.key;
              const btnId = p.key === "timeshare" ? "btn_timeshare" : p.key === "day" ? "btn_day_k" : undefined;
              return (
                <Button
                  key={p.key}
                  id={btnId}
                  text={p.label}
                  width={p.width}
                  height={20}
                  fontSize={11}
                  background={active ? T.buyBtnBg : T.btnBg}
                  foreground={active ? "#FFFFFF" : T.textSecondary}
                  onClick={() => setPeriod(p.key)}
                />
              );
            })}
            <Button
              text="多周期"
              width={46}
              height={20}
              fontSize={11}
              background={T.btnBg}
              foreground={T.textDim}
              onClick={() => setPeriod(period === "timeshare" ? "day" : "timeshare")}
            />
            <Spacer flexGrow={1} flexBasis={0} />
            <Label text="九转  数据看盘  大盘异动  加自选" fontSize={11} color={T.textMuted} />
          </Panel>

          {/* Keyboard Genie Interactive Match Bar (appears when typing query) */}
          {genieMatches.length > 0 ? (
            <Panel direction="row" height={28} padding={{ x: 8, y: 0 }} spacing={6} background={T.popupBg} borderColor={T.popupBorder} border={1} alignItems="center">
              <Label text="键盘精灵匹配：" fontSize={11} color={T.accentGold} bold />
              {genieMatches.map((m) => (
                <Button
                  key={m.code}
                  text={`${m.code} ${m.name} (${m.pinyin})`}
                  width={136}
                  height={20}
                  fontSize={11}
                  background={T.btnBg}
                  foreground={T.textPrimary}
                  onClick={() => selectSymbolByCode(m.code)}
                />
              ))}
              <Spacer flexGrow={1} flexBasis={0} />
              <Button text="关闭(ESC)" width={64} height={20} fontSize={10} background={T.orderBtnBg} foreground={T.orderBtnFg} onClick={() => setSearchQuery("")} />
            </Panel>
          ) : null}

          {/* Core High-Speed C++ FinancialChart (144Hz geometry cache + O(1) tick append) */}
          <FinancialChart
            id="stock_chart"
            mode={period === "timeshare" ? "timeshare" : "candlestick"}
            preClose={snap.preClose}
            modernTheme={isModernTheme}
            showCrosshair
            showHeaderInfo
            symbolName={session.symbol.name}
            periodLabel={activePeriodMeta.capsule}
            points={session.points}
            candles={activeCandles}
            flexGrow={1}
            flexBasis={0}
          />

          {/* News Category Bar */}
          <Panel direction="row" height={22} padding={{ x: 6, y: 0 }} spacing={4} background={T.menuBg} alignItems="center">
            {NEWS_TABS.map((nt) => {
              const active = nt === activeNewsTab;
              return (
                <Button
                  key={nt}
                  text={nt}
                  width={56}
                  height={18}
                  fontSize={11}
                  background={active ? T.buyBtnBg : T.menuBg}
                  foreground={active ? "#FFFFFF" : T.textDim}
                  onClick={() => setActiveNewsTab(nt)}
                />
              );
            })}
          </Panel>

          {/* Breaking News List */}
          <Panel direction="row" height={54} padding={{ x: 8, y: 2 }} spacing={12} background={T.newsListBg} alignItems="stretch">
            <Panel direction="column" flexGrow={1} flexBasis={0} padding={0} spacing={1} alignItems="start">
              {newsSet.left.map((item, idx) => (
                <Label
                  key={idx}
                  text={item.text}
                  fontSize={11}
                  color={item.color === "red" ? T.upRed : item.color === "blue" ? T.accentBlue : T.textSecondary}
                />
              ))}
            </Panel>
            <Panel direction="column" flexGrow={1} flexBasis={0} padding={0} spacing={1} alignItems="start">
              {newsSet.right.map((item, idx) => (
                <Label
                  key={idx}
                  text={item.text}
                  fontSize={11}
                  color={item.color === "red" ? T.upRed : item.color === "blue" ? T.accentBlue : T.textSecondary}
                />
              ))}
            </Panel>
          </Panel>
        </Panel>

        {/* Right High-Density Sidebar (width 326px) */}
        <Panel direction="column" width={326} padding={0} spacing={0} background={T.rightPanelBg} alignItems="stretch">
          {/* Symbol Title + Live Quote */}
          <Panel direction="row" height={32} padding={{ x: 8, y: 2 }} spacing={6} background={T.rightHeaderBg} alignItems="center">
            <Label text={`${session.symbol.name} ${session.symbol.code}`} fontSize={14} color={T.accentGold} bold width={145} height={22} />
            <Spacer flexGrow={1} flexBasis={0} />
            <Label id="label_current_price" text={snap.current.toFixed(2)} fontSize={18} color={priceColor} bold width={86} height={24} horizontalAlign="end" />
            <Panel direction="column" padding={0} spacing={0} width={58} alignItems="end">
              <Label id="label_change" text={`${changeSign}${snap.change.toFixed(2)}`} fontSize={11} color={priceColor} width={58} height={14} horizontalAlign="end" />
              <Label id="label_change_pct" text={`${changeSign}${snap.changePct.toFixed(2)}%`} fontSize={11} color={priceColor} width={58} height={14} horizontalAlign="end" />
            </Panel>
          </Panel>

          {/* 10-Row Market Depth Grid (all dynamic value cells have explicit width+height -> 0 relayout on tick) */}
          <Panel direction="column" padding={{ x: 6, y: 1 }} spacing={0} background={T.depthBg}>
            <DepthCellRow
              k1="委比"
              v1={`${snap.weiBi >= 0 ? "+" : ""}${snap.weiBi.toFixed(2)}%`}
              c1={snap.weiBi >= 0 ? T.upRed : T.downGreen}
              k2="昨收"
              v2={snap.preClose.toFixed(2)}
              c2={T.textPrimary}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="委差"
              v1={`${Math.round(snap.weiDiff)}`}
              c1={snap.weiDiff >= 0 ? T.upRed : T.downGreen}
              k2="今开"
              v2={snap.open.toFixed(2)}
              c2={snap.open >= snap.preClose ? T.upRed : T.downGreen}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="涨跌"
              v1={`${changeSign}${snap.change.toFixed(2)}`}
              c1={priceColor}
              k2="最高"
              v2={snap.high.toFixed(2)}
              c2={T.upRed}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="涨幅"
              v1={`${changeSign}${snap.changePct.toFixed(2)}%`}
              c1={priceColor}
              k2="最低"
              v2={snap.low.toFixed(2)}
              c2={T.downGreen}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="振幅"
              v1={`${snap.amplitude.toFixed(2)}%`}
              c1={T.textPrimary}
              k2="量比"
              v2={snap.volumeRatio.toFixed(2)}
              c2={T.textPrimary}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="总手"
              v1={`${Math.round(snap.totalHand)}万`}
              c1={T.accentGold}
              k2="金额"
              v2={`${Math.round(snap.totalAmount)}亿`}
              c2={T.accentBlue}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="两市额"
              v1={`${Math.round(snap.marketTurnover)}亿`}
              c1={T.accentBlue}
              k2="换手"
              v2={`${snap.turnoverRate.toFixed(2)}%`}
              c2={T.textPrimary}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="市盈动"
              v1={snap.peDynamic.toFixed(2)}
              c1={T.textPrimary}
              k2="市净率"
              v2={snap.pbRatio.toFixed(2)}
              c2={T.textPrimary}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="委买量"
              v1={`${snap.weiBuyHand}万`}
              c1={T.upRed}
              k2="委卖量"
              v2={`${snap.weiSellHand}万`}
              c2={T.downGreen}
              dimColor={T.textDim}
            />
            <DepthCellRow
              k1="上涨"
              v1={`${snap.upCount}家`}
              c1={T.upRed}
              k2="下跌"
              v2={`${snap.downCount}家`}
              c2={T.downGreen}
              dimColor={T.textDim}
            />
          </Panel>

          {/* Contribution Rank Header */}
          <Panel direction="row" height={20} padding={{ x: 6, y: 0 }} spacing={0} background={T.sectionHdrBg} alignItems="center">
            <Label text="个股贡献度 (点击切标的)" fontSize={11} color={T.accentGold} bold flexGrow={1} />
            <Label text="涨幅%" fontSize={11} color={T.textMuted} width={48} height={16} />
            <Label text="现价" fontSize={11} color={T.textMuted} width={52} height={16} />
            <Label text="贡献" fontSize={11} color={T.textMuted} width={42} height={16} />
          </Panel>

          {/* Contribution Rank Rows (8 rows * 17px) */}
          <Panel direction="column" padding={{ x: 4, y: 1 }} spacing={0} background={T.rankBg}>
            {CONTRIB_RANKS.map((item) => {
              const chgColor = item.changePct >= 0 ? T.upRed : T.downGreen;
              const ctrColor = item.contribution >= 0 ? T.upRed : T.downGreen;
              const isSelected = item.code === session.symbol.code;
              return (
                <Panel key={item.code} direction="row" height={17} padding={0} spacing={0} background={isSelected ? T.sectionHdrBg : "transparent"} alignItems="center">
                  <Button
                    text={item.name}
                    width={64}
                    height={16}
                    fontSize={11}
                    background="transparent"
                    foreground={isSelected ? T.accentGold : T.textPrimary}
                    onClick={() => selectSymbolByCode(item.code)}
                  />
                  <Spacer flexGrow={1} flexBasis={0} />
                  <Label
                    text={`${item.changePct >= 0 ? "+" : ""}${item.changePct.toFixed(2)}%`}
                    fontSize={11}
                    color={chgColor}
                    width={48}
                    height={16}
                  />
                  <Label text={item.price.toFixed(2)} fontSize={11} color={chgColor} width={52} height={16} />
                  <Label
                    text={`${item.contribution >= 0 ? "+" : ""}${item.contribution.toFixed(2)}`}
                    fontSize={11}
                    color={ctrColor}
                    width={42}
                    height={16}
                  />
                </Panel>
              );
            })}
          </Panel>

          {/* Trade Alerts Header */}
          <Panel direction="row" height={20} padding={{ x: 6, y: 0 }} spacing={0} background={T.sectionHdrBg} alignItems="center">
            <Label text="短线精灵" fontSize={11} color={T.upRed} bold flexGrow={1} />
            <Label text="A股逐笔" fontSize={11} color={T.textMuted} />
          </Panel>

          {/* Trade Alerts List (8 fixed keyed slots -> 0 node churn & 0 Yoga layout on 500ms tick) */}
          <Panel id="tick_container" direction="column" flexGrow={1} flexBasis={0} padding={{ x: 6, y: 1 }} spacing={0} background={T.tickBg}>
            {session.ticks.map((tk, idx) => {
              const tkCol = tk.isBuy ? T.upRed : T.downGreen;
              return (
                <Panel key={idx} direction="row" height={18} padding={0} spacing={0} alignItems="center">
                  <Label text={tk.timeStr} fontSize={10} color={tk.highlight ? T.upRed : T.textDim} bold={Boolean(tk.highlight)} width={56} height={16} />
                  <Label text={tk.typeStr} fontSize={10} color={tkCol} bold={Boolean(tk.highlight)} width={202} height={16} />
                  <Label text={tk.volumeLabel} fontSize={10} color={tkCol} width={56} height={16} horizontalAlign="end" />
                </Panel>
              );
            })}
          </Panel>
        </Panel>
      </Panel>

      {/* 5. Bottom Cross-Market Status Bar */}
      <Panel direction="row" height={26} padding={{ x: 8, y: 0 }} spacing={10} background={T.statusBarBg} alignItems="center">
        <Label text="沪" fontSize={11} color={T.textDim} />
        <Label id="bottom_sh_price" text={snap.current.toFixed(2)} fontSize={11} color={priceColor} bold width={58} height={18} />
        <Label
          id="bottom_sh_diff"
          text={`${changeSign}${snap.change.toFixed(2)} (${changeSign}${snap.changePct.toFixed(2)}%)`}
          fontSize={10}
          color={priceColor}
          width={104}
          height={18}
        />
        <Label text={`${Math.round(snap.totalAmount)}亿`} fontSize={10} color={T.accentBlue} width={48} height={18} />

        <Label text="深" fontSize={11} color={T.textDim} />
        <Label text="14718.66" fontSize={11} color={T.upRed} bold />
        <Label text="+47.95 (+0.33%)" fontSize={10} color={T.upRed} />
        <Label text="5606亿" fontSize={10} color={T.accentBlue} />

        <Label text="创" fontSize={11} color={T.textDim} />
        <Label text="3352.12" fontSize={11} color={T.upRed} bold />
        <Label text="+18.22 (+0.55%)" fontSize={10} color={T.upRed} />

        <Label text="科" fontSize={11} color={T.textDim} />
        <Label text="1575.89" fontSize={11} color={T.upRed} bold />
        <Label text="+37.73 (+2.45%)" fontSize={10} color={T.upRed} />

        <Spacer flexGrow={1} flexBasis={0} />
        <TextInput
          id="genie_input"
          text={searchQuery}
          placeholder="代码/简拼/功能"
          width={126}
          height={20}
          fontSize={11}
          background={T.inputBg}
          color={T.textPrimary}
          placeholderColor={T.btnDimText}
          borderColor={T.popupBorder}
          onTextChanged={setSearchQuery}
          onSubmit={() => {
            if (genieMatches.length > 0) selectSymbolByCode(genieMatches[0].code);
          }}
        />
      </Panel>
    </Panel>
  );
}

render(<StockTerminalApp />);

