// samples/stock_terminal/main.cpp
// High-fidelity Flush / Hithink Style Financial Terminal Prototype on ai-win-ui
#include <ai_win_ui/host.h>
#include "mock_market_engine.h"
#include "keyboard_genie.h"

#include <Windows.h>
#include <cstdio>
#include <memory>
#include <string>

namespace {

struct AppState {
    HINSTANCE instance = nullptr;
    std::unique_ptr<ai_win_ui::Host> host;
    MockMarketEngine engine;
    KeyboardGenie genie;
    bool isModernTheme = false;
};

static AppState* g_state = nullptr;

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

    // 最高与总手
    swprintf_s(buf, L"%.2f", snap.high);
    host->SetElementText("grid_high", buf);

    swprintf_s(buf, L"%.0f万", snap.totalHand);
    host->SetElementText("grid_total_hand", buf);
}

static void OnAction(const char* actionId, void* userData) {
    auto* state = static_cast<AppState*>(userData);
    if (!actionId || !state || !state->host) return;

    if (strcmp(actionId, "btn_toggle_theme") == 0) {
        state->isModernTheme = !state->isModernTheme;
        state->host->SetFinancialChartTheme("stock_chart", state->isModernTheme);
        state->host->Invalidate();
    }
}

static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN && g_state && g_state->host) {
        auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (kbd->vkCode == 'T') {
            // T 键切换主题
            g_state->isModernTheme = !g_state->isModernTheme;
            g_state->host->SetFinancialChartTheme("stock_chart", g_state->isModernTheme);
            g_state->host->Invalidate();
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    AppState state;
    state.instance = instance;
    g_state = &state;

    // 1. 初始化模拟行情 (开盘至尾盘走势)
    state.engine.Initialize(205);

    // 2. 创建金融终端主宿主
    ai_win_ui::HostCreateInfo info{};
    info.showCommand = cmdShow;
    info.layoutPath = L"layouts/stock_quotes.xml";
    info.size = L"1280x820";
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

    // 3. 灌入分时历史数据到 stock_chart
    state.host->ResetFinancialChart("stock_chart", state.engine.GetSnapshot().preClose);
    for (const auto& pt : state.engine.GetPoints()) {
        state.host->AppendFinancialChartPoint("stock_chart", pt.price, pt.leadPrice, pt.volume, pt.isUp, pt.timeStr.c_str());
    }
    UpdateUiFromSnapshot(state.host.get(), state.engine.GetSnapshot());

    // 4. 行情数据更新回调
    state.engine.onDataUpdated = [&state]() {
        const auto& pts = state.engine.GetPoints();
        if (!pts.empty()) {
            const auto& last = pts.back();
            state.host->AppendFinancialChartPoint("stock_chart", last.price, last.leadPrice, last.volume, last.isUp, last.timeStr.c_str());
            UpdateUiFromSnapshot(state.host.get(), state.engine.GetSnapshot());
        }
    };

    // 5. 设置 500ms 定时器驱动行情 Tick 跳动
    SetTimer(hwnd, 1001, 500, [](HWND, UINT, UINT_PTR, DWORD) {
        if (g_state) {
            g_state->engine.OnTimerTick();
        }
    });

    // 6. 安装轻量键盘按键监听 (支持快捷键 T 切换主题)
    HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, instance, 0);

    int result = state.host->Run();

    if (hook) {
        UnhookWindowsHookEx(hook);
    }
    g_state = nullptr;
    return result;
}
