import type { CandleBarData, FinancialPointData } from "ai-win-ui";

export interface StockSymbol {
  code: string;
  name: string;
  pinyin: string;
  market: string;
  basePrice: number;
  isIndex?: boolean;
}

export interface MarketSnapshot {
  code: string;
  name: string;
  current: number;
  preClose: number;
  open: number;
  high: number;
  low: number;
  change: number;
  changePct: number;
  amplitude: number;
  volumeRatio: number;
  totalHand: number;      // 万手
  totalAmount: number;    // 亿元
  marketTurnover: number; // 两市成交额 亿元
  weiBi: number;          // %
  weiDiff: number;        // 委差手
  weiBuyHand: number;     // 万手
  weiSellHand: number;    // 万手
  peDynamic: number;
  pbRatio: number;
  turnoverRate: number;
  upCount: number;
  downCount: number;
}

export interface TradeTick {
  timeStr: string;
  typeStr: string;
  volumeLabel: string;
  isBuy: boolean;
  highlight?: boolean;
}

export interface ContribRankItem {
  code: string;
  name: string;
  changePct: number;
  price: number;
  contribution: number;
}

export type ChartPeriodKey =
  | "timeshare"
  | "day"
  | "week"
  | "month"
  | "m1"
  | "m5"
  | "m15"
  | "m30"
  | "m60";

export interface SymbolSession {
  symbol: StockSymbol;
  snapshot: MarketSnapshot;
  points: FinancialPointData[];
  ticks: TradeTick[];
  candlesByPeriod: Partial<Record<Exclude<ChartPeriodKey, "timeshare">, CandleBarData[]>>;
  rngState: number;
}

export const STOCK_DATABASE: StockSymbol[] = [
  { code: "1A0001", name: "上证指数", pinyin: "SZZS", market: "上证", basePrice: 3534.32, isIndex: true },
  { code: "399001", name: "深证成指", pinyin: "SZCZ", market: "深证", basePrice: 14718.66, isIndex: true },
  { code: "399006", name: "创业板指", pinyin: "CYBZ", market: "创业", basePrice: 3352.12, isIndex: true },
  { code: "000688", name: "科创50", pinyin: "KC50", market: "科创", basePrice: 1575.89, isIndex: true },
  { code: "600519", name: "贵州茅台", pinyin: "GZMT", market: "沪A", basePrice: 1994.0 },
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
  { code: "300750", name: "宁德时代", pinyin: "NDSD", market: "创业", basePrice: 245.6 },
];

export const CONTRIB_RANKS: ContribRankItem[] = [
  { code: "600519", name: "贵州茅台", changePct: -0.94, price: 1994.0, contribution: +0.17 },
  { code: "600036", name: "招商银行", changePct: -0.21, price: 52.94, contribution: +0.07 },
  { code: "601166", name: "兴业银行", changePct: +1.2, price: 21.04, contribution: +0.06 },
  { code: "600196", name: "复星医药", changePct: +0.68, price: 72.49, contribution: +0.05 },
  { code: "601088", name: "中国神华", changePct: +3.53, price: 19.96, contribution: +0.03 },
  { code: "601318", name: "中国平安", changePct: -0.63, price: 61.91, contribution: +0.03 },
  { code: "600703", name: "北方稀土", changePct: +9.99, price: 22.68, contribution: +0.02 },
  { code: "603833", name: "欧派家居", changePct: -2.36, price: 135.9, contribution: -0.03 },
];

export const INITIAL_TICKS: TradeTick[] = [
  { timeStr: "14:58:33", typeStr: "大笔买入", volumeLabel: "171手", isBuy: true },
  { timeStr: "14:58:35", typeStr: "大笔买入", volumeLabel: "11006手", isBuy: true },
  { timeStr: "14:58:38", typeStr: "大笔买入", volumeLabel: "2438手", isBuy: true },
  { timeStr: "14:58:39", typeStr: "大笔卖出", volumeLabel: "1920手", isBuy: false },
  { timeStr: "14:58:45", typeStr: "大笔卖出", volumeLabel: "5612手", isBuy: false },
  { timeStr: "14:58:48", typeStr: "大笔买入", volumeLabel: "23357手", isBuy: true },
  { timeStr: "14:58:51", typeStr: "急速拉升", volumeLabel: "1.66%", isBuy: true, highlight: true },
  { timeStr: "14:58:53", typeStr: "区间放量(涨)", volumeLabel: "6.02倍", isBuy: true },
  { timeStr: "14:59:01", typeStr: "大笔买入", volumeLabel: "4235手", isBuy: true },
  { timeStr: "14:59:12", typeStr: "大笔卖出", volumeLabel: "788手", isBuy: false },
];

export const NEWS_TABS = [
  "股市直播",
  "投资机会",
  "观点评论",
  "资金流向",
  "主力增仓",
  "短线涨跌",
  "板块热点",
  "关联指数",
  "相关ETF",
] as const;

export const NEWS_BY_TAB: Record<string, { left: { text: string; color: "normal" | "red" | "blue" }[]; right: { text: string; color: "normal" | "red" | "blue" }[] }> = {
  股市直播: {
    left: [
      { text: "[解盘] 港股收评：恒指收跌0.59% 腾讯控股跌3.57%     16:09", color: "normal" },
      { text: "[解盘] 沪深两市成交额突破1万亿元                    15:01", color: "red" },
      { text: "[解盘] 第三代半导体板块持续走强                      14:22", color: "red" },
    ],
    right: [
      { text: "[要闻] 华银电力：公司与张家界市签订能源开发战略协议  16:20", color: "blue" },
      { text: "[要闻] 凯撒文化：预计2021年前半年盈利同比增长50%    16:19", color: "blue" },
      { text: "[要闻] 华鲁恒升：预计上半年净利润同比增长314.82%    16:13", color: "red" },
    ],
  },
  投资机会: {
    left: [
      { text: "[研报] 新能源车产业链销量超预期，关注锂电隔膜龙头    15:40", color: "red" },
      { text: "[研报] 银行板块低估值修复空间显现，股息率保持高位    14:55", color: "blue" },
      { text: "[研报] AI 端侧硬件加速渗透，关注封装测试产能景气度    13:45", color: "normal" },
    ],
    right: [
      { text: "[机会] 稀土永磁下游需求放量，北方稀土涨停带动板块    15:12", color: "red" },
      { text: "[机会] 高端白酒批价坚挺，中秋旺季备货周期渐近        14:08", color: "blue" },
      { text: "[机会] 光伏逆变器出海订单同比翻倍，盈利能力稳中向好  11:20", color: "normal" },
    ],
  },
  资金流向: {
    left: [
      { text: "[资金] 主力净流入电子板块 +48.6亿，有色金属 +31.2亿  15:00", color: "red" },
      { text: "[资金] 北向资金全天净买入 A 股 62.4亿元              15:01", color: "red" },
      { text: "[资金] 医药生物小幅净流出 -6.3亿元                   14:30", color: "normal" },
    ],
    right: [
      { text: "[北向] 贵州茅台获净买入 +8.42亿，招商银行 +4.15亿    15:05", color: "blue" },
      { text: "[两融] 沪深两市融资余额单日增加 +74.8亿元            14:50", color: "blue" },
      { text: "[大宗] 宁德时代机构专用席位溢价成交 2.1亿元          15:15", color: "red" },
    ],
  },
};

function nextRand(state: number): [number, number] {
  const next = (Math.imul(state, 1664525) + 1013904223) >>> 0;
  return [next / 4294967296, next];
}

function hashSeed(code: string): number {
  let h = 1337;
  for (let i = 0; i < code.length; i++) {
    h = (Math.imul(h, 31) + code.charCodeAt(i)) >>> 0;
  }
  return h || 1337;
}

function pad2(n: number): string {
  return n < 10 ? `0${n}` : `${n}`;
}

export function minuteToTimeStr(m: number): string {
  let hour: number;
  let min: number;
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

export function generateCandles(
  endPrice: number,
  count: number,
  seedOffset: number,
  labelMode: "day" | "week" | "month" | "min"
): CandleBarData[] {
  const bars: CandleBarData[] = [];
  let rng = (1337 + seedOffset * 97) >>> 0;
  let cur = endPrice * 0.956;

  for (let i = 0; i < count; i++) {
    let dateStr: string;
    if (labelMode === "week") {
      dateStr = `W${pad2(i + 1)}`;
    } else if (labelMode === "month") {
      dateStr = `${24 + Math.floor(i / 12)}/${pad2((i % 12) + 1)}`;
    } else if (labelMode === "min") {
      dateStr = minuteToTimeStr(Math.min(240, i * 4));
    } else {
      const m = 1 + Math.floor(i / 22);
      const d = 1 + (i % 22);
      dateStr = `${pad2(m)}/${pad2(d)}`;
    }

    let r1: number;
    let r2: number;
    let r3: number;
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
      ma20: nextPrice,
    });
    cur = nextPrice;
  }

  for (let i = 0; i < bars.length; i++) {
    if (i >= 4) {
      let s = 0;
      for (let k = i - 4; k <= i; k++) s += bars[k].close;
      bars[i].ma5 = s / 5;
    } else {
      bars[i].ma5 = bars[i].close;
    }
    if (i >= 9) {
      let s = 0;
      for (let k = i - 9; k <= i; k++) s += bars[k].close;
      bars[i].ma10 = s / 10;
    } else {
      bars[i].ma10 = bars[i].ma5;
    }
    if (i >= 19) {
      let s = 0;
      for (let k = i - 19; k <= i; k++) s += bars[k].close;
      bars[i].ma20 = s / 20;
    } else {
      bars[i].ma20 = bars[i].ma10;
    }
  }

  return bars;
}

export function createInitialSession(symbol: StockSymbol, initialMinutes = 205): SymbolSession {
  let rng = hashSeed(symbol.code);
  const preClose = symbol.code === "1A0001" ? 3518.76 : symbol.basePrice * 0.9956;
  const open = symbol.code === "1A0001" ? 3516.92 : preClose * 0.9994;
  const pts: FinancialPointData[] = [];
  let prevPrice = preClose;
  let hi = open;
  let lo = open;

  for (let m = 0; m <= initialMinutes; m++) {
    let rNoise: number;
    let rVol: number;
    [rNoise, rng] = nextRand(rng);
    [rVol, rng] = nextRand(rng);
    const progress = m / 240;
    const wave = Math.sin(progress * Math.PI * 1.5) * (preClose * 0.0045);
    const noise = (-0.8 + rNoise * 1.8) * (preClose / 3518.76);
    const price = m === initialMinutes ? symbol.basePrice : preClose + wave + noise;
    const leadPrice = price + (1.2 + 0.5 * Math.cos(progress * 6.28)) * (preClose / 3518.76);
    const vol = (500 + rVol * 3000) * (m < 15 || m > 220 ? 2.5 : 1.0);
    if (price > hi) hi = price;
    if (price < lo) lo = price;

    pts.push({
      price,
      leadPrice,
      volume: vol,
      indicator1: 60 + 25 * progress + noise * 2,
      indicator2: 40 - 15 * progress + noise * 1.5,
      isUp: price >= prevPrice,
      timeStr: minuteToTimeStr(m),
    });
    prevPrice = price;
  }

  const current = pts[pts.length - 1].price;
  const change = current - preClose;
  const changePct = (change / preClose) * 100;
  const isMain = symbol.code === "1A0001";

  const snapshot: MarketSnapshot = {
    code: symbol.code,
    name: symbol.name,
    current,
    preClose,
    open,
    high: isMain ? Math.max(hi, 3534.32) : hi,
    low: isMain ? Math.min(lo, 3510.58) : lo,
    change,
    changePct,
    amplitude: ((hi - lo) / preClose) * 100,
    volumeRatio: isMain ? 0.97 : 1.12,
    totalHand: isMain ? 28809 : Math.round(420 + (hashSeed(symbol.code) % 3500)),
    totalAmount: isMain ? 4454 : Math.round(28 + (hashSeed(symbol.code) % 120)),
    marketTurnover: 10060,
    weiBi: isMain ? -2.11 : 4.35,
    weiDiff: isMain ? -665486 : 14820,
    weiBuyHand: isMain ? 1543 : 182,
    weiSellHand: isMain ? 1610 : 149,
    peDynamic: isMain ? 14.83 : 24.6,
    pbRatio: isMain ? 1.61 : 2.85,
    turnoverRate: isMain ? 0.77 : 1.34,
    upCount: 1311,
    downCount: 597,
  };

  return {
    symbol,
    snapshot,
    points: pts,
    ticks: INITIAL_TICKS.slice(0, 8),
    candlesByPeriod: {},
    rngState: rng,
  };
}

export function getOrGenerateCandles(
  session: SymbolSession,
  period: Exclude<ChartPeriodKey, "timeshare">
): CandleBarData[] {
  const cached = session.candlesByPeriod[period];
  if (cached) return cached;

  let count = 90;
  let mode: "day" | "week" | "month" | "min" = "day";
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

  const bars = generateCandles(
    session.snapshot.current,
    count,
    hashSeed(session.symbol.code) + seedBias,
    mode
  );
  session.candlesByPeriod[period] = bars;
  return bars;
}

export function stepMarketTick(session: SymbolSession): SymbolSession {
  let rng = session.rngState;
  let r1: number;
  let r2: number;
  let r3: number;
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

  let nextPoints: FinancialPointData[];
  const nextMinute = session.points.length;
  if (nextMinute <= 240) {
    const newPt: FinancialPointData = {
      price: newPrice,
      leadPrice: newPrice + (1.0 + (-0.1 + r2 * 0.25)) * scale,
      volume: Math.max(250, prev.volume * 0.4 + Math.abs(delta) * 900),
      indicator1: prev.indicator1 ?? 68,
      indicator2: prev.indicator2 ?? 32,
      isUp,
      timeStr: minuteToTimeStr(nextMinute),
    };
    nextPoints = [...session.points, newPt];
  } else {
    const updatedLast: FinancialPointData = {
      ...prev,
      price: newPrice,
      leadPrice: newPrice + (1.0 + (-0.1 + r2 * 0.25)) * scale,
      volume: prev.volume + Math.abs(delta) * 800,
      isUp,
    };
    nextPoints = [...session.points.slice(0, -1), updatedLast];
  }

  const snap = session.snapshot;
  const nextHigh = Math.max(snap.high, newPrice);
  const nextLow = Math.min(snap.low, newPrice);
  const nextChange = newPrice - snap.preClose;
  const nextPct = (nextChange / snap.preClose) * 100;
  const nextWeiBi = Math.max(-15, Math.min(15, snap.weiBi + (-0.35 + r2 * 0.72)));

  const now = new Date();
  const timeStr = `${pad2(now.getHours())}:${pad2(now.getMinutes())}:${pad2(now.getSeconds())}`;
  const kindPick = Math.floor(r3 * 5);
  const volHand = Math.floor(120 + r2 * 14500);
  const newTick: TradeTick =
    kindPick === 0
      ? { timeStr, typeStr: "急速拉升", volumeLabel: `${Math.abs(nextPct).toFixed(2)}%`, isBuy: true, highlight: true }
      : kindPick <= 2
      ? { timeStr, typeStr: "大笔买入", volumeLabel: `${volHand}手`, isBuy: true }
      : { timeStr, typeStr: "大笔卖出", volumeLabel: `${volHand}手`, isBuy: false };

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
      amplitude: ((nextHigh - nextLow) / snap.preClose) * 100,
      totalHand: snap.totalHand + 1.2,
      totalAmount: snap.totalAmount + 0.4,
      marketTurnover: snap.marketTurnover + 0.9,
      weiBi: nextWeiBi,
      weiDiff: Math.round(snap.weiDiff + (r2 - 0.48) * 12000),
    },
  };
}

export function querySymbols(query: string, maxResults = 6): StockSymbol[] {
  const q = query.trim().toUpperCase();
  if (!q) return [];
  const out: StockSymbol[] = [];
  for (const s of STOCK_DATABASE) {
    if (s.code.includes(q) || s.pinyin.includes(q) || s.name.includes(q)) {
      out.push(s);
      if (out.length >= maxResults) break;
    }
  }
  return out;
}

