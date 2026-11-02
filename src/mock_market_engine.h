#pragma once
// mock_market_engine.h - Realistic simulated market data feed for stock terminal
#include "ui_financial_chart.h"
#include <string>
#include <vector>
#include <functional>
#include <random>

struct MarketSnapshot {
    std::wstring code = L"1A0001";
    std::wstring name = L"上证指数";
    float current = 3534.32f;
    float preClose = 3518.76f;
    float open = 3516.92f;
    float high = 3534.32f;
    float low = 3510.58f;
    float change = 15.56f;
    float changePct = 0.44f;
    float amplitude = 0.67f;
    float currentHand = 121680.0f; // 现手
    float volumeRatio = 0.97f;    // 量比
    float totalHand = 28809.0f;   // 万手
    float totalAmount = 4454.0f;  // 亿元
    float marketTurnover = 10060.0f; // 两市成交额 亿元
    float weiBi = -2.11f;         // %
    float weiDiff = -665486.0f;   // 委差手
    float weiBuyAmount = 88.32f;  // 亿元
    float weiSellAmount = 79.63f; // 亿元
    float totalCap = 484140.0f;   // 总市值 亿元
    float flowCap = 402781.0f;    // 流通市值 亿元
    float peDynamic = 14.83f;     // 市盈(动)
    float pbRatio = 1.61f;        // 市净率
    float turnoverRate = 0.77f;   // 换手率 %
    float avgPrice = 31.36f;      // 均价
    int upCount = 1311;           // 上涨家数
    int flatCount = 65;           // 平盘家数
    int downCount = 597;          // 下跌家数
    int limitUpCount = 84;        // 涨停家数
    int limitDownCount = 7;       // 跌停家数
};

struct TradeTick {
    std::string timeStr; // "14:58:33"
    std::string typeStr; // "大笔买入", "大笔卖出", "急速拉升"
    float price = 0.0f;
    int volumeHand = 0;
    bool isBuy = true;
};

struct StockRankItem {
    std::wstring code;
    std::wstring name;
    float changePct;
    float price;
    float contribution;
};

class MockMarketEngine {
public:
    MockMarketEngine();

    // 初始化生成历史分时点 (默认生成到约 14:00~15:00)
    void Initialize(int initialMinutes = 200);

    // 生成历史日 K 线数据 (包含真实的 MA5/10/20)
    std::vector<CandleBar> GenerateCandles(int count = 90);

    // 时钟推进一个 Tick (每隔 500ms~1s 调用)
    void OnTimerTick();

    const MarketSnapshot& GetSnapshot() const { return m_snapshot; }
    const std::vector<FinancialPoint>& GetPoints() const { return m_points; }
    const std::vector<TradeTick>& GetRecentTicks() const { return m_recentTicks; }
    const std::vector<StockRankItem>& GetContributionRanks() const { return m_contribRanks; }

    // 回调通知
    std::function<void()> onDataUpdated;

private:
    void GenerateRanks();
    void PushNewTick();

    MarketSnapshot m_snapshot;
    std::vector<FinancialPoint> m_points;
    std::vector<TradeTick> m_recentTicks;
    std::vector<StockRankItem> m_contribRanks;

    std::mt19937 m_rng;
    int m_currentMinute = 0;
};
