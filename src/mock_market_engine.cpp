#include "mock_market_engine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

static std::string MinuteToTimeStr(int m) {
    int hour = 0;
    int min = 0;
    if (m <= 120) {
        hour = 9 + (30 + m) / 60;
        min = (30 + m) % 60;
    } else {
        int pmM = m - 120;
        hour = 13 + pmM / 60;
        min = pmM % 60;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, min);
    return std::string(buf);
}

MockMarketEngine::MockMarketEngine()
    : m_rng(1337) {
    GenerateRanks();
}

void MockMarketEngine::GenerateRanks() {
    m_contribRanks = {
        { L"600519", L"贵州茅台", -0.94f, 1994.00f, +0.17f },
        { L"600036", L"招商银行", -0.21f,   52.94f, +0.07f },
        { L"601166", L"兴业银行", +1.20f,   21.04f, +0.06f },
        { L"600196", L"复星医药", +0.68f,   72.49f, +0.05f },
        { L"600019", L"宝钢股份", -1.70f,    7.50f, +0.05f },
        { L"603833", L"欧派家居", -2.36f,  135.92f, +0.03f },
        { L"601088", L"中国神华", +3.53f,   19.96f, +0.03f },
        { L"603259", L"药明康德", +0.05f,  142.12f, +0.03f },
        { L"601318", L"中国平安", -0.63f,   61.91f, +0.03f },
        { L"600809", L"山西汾酒", -1.02f,   85.25f, +0.02f },
        { L"600703", L"三安光电", +9.99f,   22.68f, +0.02f },
        { L"600919", L"江苏银行", -0.56f,    7.07f, +0.02f },
        { L"600000", L"浦发银行", -0.10f,    9.92f, +0.02f },
        { L"600585", L"海螺水泥", -0.37f,   40.18f, +0.02f },
    };

    // 初始逐笔
    m_recentTicks = {
        { "14:58:33", "大笔买入", 3533.80f, 171,   true },
        { "14:58:35", "大笔买入", 3533.95f, 11006, true },
        { "14:58:38", "大笔买入", 3534.10f, 2438,  true },
        { "14:58:38", "大笔买入", 3534.12f, 444,   true },
        { "14:58:39", "大笔卖出", 3534.00f, 1920,  false },
        { "14:58:39", "大笔买入", 3534.05f, 648,   true },
        { "14:58:42", "大笔买入", 3534.15f, 2769,  true },
        { "14:58:45", "大笔卖出", 3533.90f, 5612,  false },
        { "14:58:48", "大笔买入", 3534.20f, 23357, true },
        { "14:58:51", "急速拉升", 3534.30f, 8500,  true },
        { "14:58:53", "大笔买入", 3534.32f, 12410, true }
    };
}

void MockMarketEngine::Initialize(int initialMinutes) {
    m_points.clear();
    m_currentMinute = std::clamp(initialMinutes, 10, 240);

    float base = m_snapshot.preClose;
    float currentPrice = m_snapshot.open;
    float currentLead = currentPrice + 2.0f;
    float prevPrice = base;

    std::uniform_real_distribution<float> noiseDist(-0.8f, 1.0f);
    std::uniform_real_distribution<float> volDist(500.0f, 3500.0f);
    std::uniform_real_distribution<float> indDist(20.0f, 85.0f);

    for (int m = 0; m <= m_currentMinute; ++m) {
        FinancialPoint pt;
        pt.timeStr = MinuteToTimeStr(m);

        // 模拟平滑趋势：早盘探底后震荡拉升
        float progress = static_cast<float>(m) / 240.0f;
        float trend = std::sin(progress * 3.14159f * 1.5f) * 16.0f;
        float noise = noiseDist(m_rng);

        currentPrice = base + trend + noise;
        // 确保最终点接近 3534.32
        if (m == m_currentMinute) {
            currentPrice = m_snapshot.current;
        }

        currentLead = currentPrice + 1.2f + 0.5f * std::cos(progress * 6.28f);

        pt.price = currentPrice;
        pt.leadPrice = currentLead;
        pt.volume = volDist(m_rng);
        if (m < 15 || m > 220) pt.volume *= 2.5f; // 开盘与尾盘放量

        pt.isUp = (pt.price >= prevPrice);
        prevPrice = pt.price;

        pt.indicator1 = 60.0f + 25.0f * progress + noise * 3.0f; // 红线
        pt.indicator2 = 40.0f - 15.0f * progress + noise * 2.0f; // 黄线

        m_points.push_back(pt);
    }

    m_snapshot.current = m_points.back().price;
    m_snapshot.change = m_snapshot.current - m_snapshot.preClose;
    m_snapshot.changePct = (m_snapshot.change / m_snapshot.preClose) * 100.0f;
}

void MockMarketEngine::OnTimerTick() {
    if (m_points.empty()) return;

    std::uniform_real_distribution<float> tickDelta(-0.25f, 0.35f);
    float delta = tickDelta(m_rng);

    FinancialPoint& last = m_points.back();
    float newPrice = last.price + delta;
    newPrice = std::clamp(newPrice, 3510.0f, 3550.0f);

    bool isUp = (newPrice >= last.price);
    last.price = newPrice;
    last.leadPrice = newPrice + 1.0f + tickDelta(m_rng) * 0.3f;
    last.volume += std::abs(delta) * 800.0f;
    last.isUp = isUp;

    m_snapshot.current = newPrice;
    m_snapshot.high = std::max(m_snapshot.high, newPrice);
    m_snapshot.low = std::min(m_snapshot.low, newPrice);
    m_snapshot.change = m_snapshot.current - m_snapshot.preClose;
    m_snapshot.changePct = (m_snapshot.change / m_snapshot.preClose) * 100.0f;
    m_snapshot.totalHand += 1.2f;
    m_snapshot.totalAmount += 0.4f;

    PushNewTick();

    if (onDataUpdated) {
        onDataUpdated();
    }
}

void MockMarketEngine::PushNewTick() {
    std::uniform_int_distribution<int> volRand(100, 15000);
    std::uniform_int_distribution<int> typeRand(0, 4);

    TradeTick tick;
    char timeBuf[16];
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm localTm;
    localtime_s(&localTm, &now);
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", localTm.tm_hour, localTm.tm_min, localTm.tm_sec);
    tick.timeStr = timeBuf;
    tick.price = m_snapshot.current;
    tick.volumeHand = volRand(m_rng);

    int t = typeRand(m_rng);
    if (t == 0) {
        tick.typeStr = "急速拉升";
        tick.isBuy = true;
    } else if (t == 1 || t == 2) {
        tick.typeStr = "大笔买入";
        tick.isBuy = true;
    } else {
        tick.typeStr = "大笔卖出";
        tick.isBuy = false;
    }

    m_recentTicks.push_back(tick);
    if (m_recentTicks.size() > 14) {
        m_recentTicks.erase(m_recentTicks.begin());
    }
}
