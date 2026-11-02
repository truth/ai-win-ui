// samples/stock_terminal/main.cpp
// High-fidelity Flush / Hithink Style Financial Terminal Prototype on ai-win-ui
// Features:
//  - Dual Chart Modes: Time-Sharing (分时走势) + Candlestick (日K/周K/月K蜡烛图)
//  - High-density Market Depth Grid (16项盘口真实Mock数据)
//  - Real-time 500ms Tick Generation & Trade Alerts (短线精灵)
//  - Contribution Rank Board (权重股贡献度)
//  - Dual Themes (Classic Black & Modern Dark, toggle with T key)
#include <ai_win_ui/host.h>
#include "mock_market_engine.h"
#include "keyboard_genie.h"

#include <Windows.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

struct AppState {
    HINSTANCE instance = nullptr;
    std::unique_ptr<ai_win_ui::Host> host;
    MockMarketEngine engine;
    KeyboardGenie genie;
    bool isModernTheme = false;
    int chartMode = 0; // 0 = TimeSharing, 1 = DayK, 2 = WeekK, 3 = MonthK
    std::vector<CandleBar> dayCandles;
    std::vector<CandleBar> weekCandles;
    std::vector<CandleBar> monthCandles;
};

static AppState* g_state = nullptr;

static void LoadCandlesToChart(ai_win_ui::Host* host, const std::vector<CandleBar>& candles) {
    if (!host) return;
    host->ClearFinancialChartCandles("stock_chart");
    for (const auto& c : candles) {
        host->AppendFinancialChartCandle(
            "stock_chart",
            c.date.c_str(),
            c.open, c.high, c.low, c.close,
            c.volume,
            c.ma5, c.ma10, c.ma20
        );
    }
}

static void SwitchToMode(AppState* state, int mode) {
    if (!state || !state->host) return;
    state->chartMode = mode;

    if (mode == 0) {
        // 分时图模式
        state->host->SetFinancialChartMode("stock_chart", 0);
        state->host->ResetFinancialChart("stock_chart", state->engine.GetSnapshot().preClose);
        for (const auto& pt : state->engine.GetPoints()) {
            state->host->AppendFinancialChartPoint(
                "stock_chart", pt.price, pt.leadPrice, pt.volume, pt.isUp, pt.timeStr.c_str());
        }
    } else if (mode == 1) {
        // 日K模式
        state->host->SetFinancialChartMode("stock_chart", 1);
        if (state->dayCandles.empty()) {
            state->dayCandles = state->engine.GenerateCandles(90);
        }
        LoadCandlesToChart(state->host.get(), state->dayCandles);
    } else if (mode == 2) {
        // 周K模式
        state->host->SetFinancialChartMode("stock_chart", 1);
        if (state->weekCandles.empty()) {
            state->weekCandles = state->engine.GenerateCandles(45);
            // 修改周K标签
            for (size_t i = 0; i < state->weekCandles.size(); ++i) {
                char buf[16];
                snprintf(buf, sizeof(buf), "W%02d", static_cast<int>(i + 1));
                state->weekCandles[i].date = buf;
            }
        }
        LoadCandlesToChart(state->host.get(), state->weekCandles);
    } else if (mode == 3) {
        // 月K模式
        state->host->SetFinancialChartMode("stock_chart", 1);
        if (state->monthCandles.empty()) {
            state->monthCandles = state->engine.GenerateCandles(24);
            for (size_t i = 0; i < state->monthCandles.size(); ++i) {
                char buf[16];
                snprintf(buf, sizeof(buf), "25/%02d", static_cast<int>((i % 12) + 1));
                state->monthCandles[i].date = buf;
            }
        }
        LoadCandlesToChart(state->host.get(), state->monthCandles);
    }

    state->host->Invalidate();
}

static void UpdateUiFromSnapshot(ai_win_ui::Host* host, const MarketSnapshot& snap) {
    if (!host) return;

    wchar_t buf[64];

    // 现价
    swprintf_s(buf, L"%.2f", snap.current);
    host->SetElementText("label_current_price", buf);
    host->SetElementText("bottom_sh_price", buf);

    // 涨跌
    swprintf_s(buf, L"%+.2f", snap.change);
    host->SetElementText("label_change", buf);
    host->SetElementText("grid_change", buf);

    // 涨跌幅
    swprintf_s(buf, L"%+.2f%%", snap.changePct);
    host->SetElementText("label_change_pct", buf);

    swprintf_s(buf, L"%+.2f (%+.2f%%)", snap.change, snap.changePct);
    host->SetElementText("bottom_sh_diff", buf);

    // 最高、最低与总手
    swprintf_s(buf, L"%.2f", snap.high);
    host->SetElementText("grid_high", buf);

    swprintf_s(buf, L"%.2f", snap.low);
    host->SetElementText("grid_low", buf);

    swprintf_s(buf, L"%.0f万", snap.totalHand);
    host->SetElementText("grid_total_hand", buf);

    swprintf_s(buf, L"%.0f亿", snap.totalAmount);
    host->SetElementText("grid_amount", buf);

    swprintf_s(buf, L"%.2f%%", snap.weiBi);
    host->SetElementText("grid_weibi", buf);
}

static void OnAction(const char* actionId, void* userData) {
    auto* state = static_cast<AppState*>(userData);
    if (!actionId || !state || !state->host) return;

    if (strcmp(actionId, "btn_toggle_theme") == 0) {
        state->isModernTheme = !state->isModernTheme;
        state->host->SetFinancialChartTheme("stock_chart", state->isModernTheme);
        state->host->Invalidate();
    } else if (strcmp(actionId, "btn_period_timeshare") == 0) {
        SwitchToMode(state, 0);
    } else if (strcmp(actionId, "btn_period_day_k") == 0) {
        SwitchToMode(state, 1);
    } else if (strcmp(actionId, "btn_period_week_k") == 0) {
        SwitchToMode(state, 2);
    } else if (strcmp(actionId, "btn_period_month_k") == 0) {
        SwitchToMode(state, 3);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    AppState state;
    state.instance = instance;
    g_state = &state;

    // 1. 初始化模拟行情与日 K 线数据
    state.engine.Initialize(205);
    state.dayCandles = state.engine.GenerateCandles(90);

    // 2. 创建金融终端主宿主 (ownMessagePump = false, 统一消息循环管理)
    ai_win_ui::HostCreateInfo info{};
    info.showCommand = SW_SHOWNORMAL;
    info.layoutPath = L"layouts/stock_quotes.xml";
    info.size = L"1280x820";
    info.ownMessagePump = false;
    info.onAction = OnAction;
    info.actionUser = &state;

    state.host = ai_win_ui::Host::Create(instance, info);
    if (!state.host) {
        MessageBoxW(nullptr, L"无法加载 layouts/stock_quotes.xml 启动金融终端。", L"错误", MB_ICONERROR);
        return -1;
    }

    HWND hwnd = state.host->GetHwnd();
    if (hwnd) {
        SetWindowTextW(hwnd, L"同花顺金融终端 (Flush Stock Terminal) - 上证指数 1A0001 [ai-win-ui]");
    }

    // 3. 初始灌入分时历史数据到 stock_chart
    state.host->ResetFinancialChart("stock_chart", state.engine.GetSnapshot().preClose);
    for (const auto& pt : state.engine.GetPoints()) {
        state.host->AppendFinancialChartPoint(
            "stock_chart", pt.price, pt.leadPrice, pt.volume, pt.isUp, pt.timeStr.c_str());
    }
    UpdateUiFromSnapshot(state.host.get(), state.engine.GetSnapshot());

    // 4. 行情数据更新回调
    state.engine.onDataUpdated = [&state]() {
        if (state.chartMode == 0) {
            const auto& pts = state.engine.GetPoints();
            if (!pts.empty()) {
                const auto& last = pts.back();
                state.host->AppendFinancialChartPoint(
                    "stock_chart", last.price, last.leadPrice, last.volume, last.isUp, last.timeStr.c_str());
            }
        }
        UpdateUiFromSnapshot(state.host.get(), state.engine.GetSnapshot());
    };

    // 5. 设置 500ms 定时器驱动行情 Tick 跳动
    SetTimer(hwnd, 1001, 500, [](HWND, UINT, UINT_PTR, DWORD) {
        if (g_state) {
            g_state->engine.OnTimerTick();
        }
    });

    // 6. 统一消息循环与热键监听 (T 键切换主题, F5 键刷新)
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == 'T') {
                state.isModernTheme = !state.isModernTheme;
                state.host->SetFinancialChartTheme("stock_chart", state.isModernTheme);
                state.host->Invalidate();
            } else if (msg.wParam == VK_F5) {
                if (state.chartMode == 0) {
                    state.host->ResetFinancialChart("stock_chart", state.engine.GetSnapshot().preClose);
                    for (const auto& pt : state.engine.GetPoints()) {
                        state.host->AppendFinancialChartPoint(
                            "stock_chart", pt.price, pt.leadPrice, pt.volume, pt.isUp, pt.timeStr.c_str());
                    }
                }
            }
        }
        if (!state.host->ProcessMessage(msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    g_state = nullptr;
    return static_cast<int>(msg.wParam);
}
