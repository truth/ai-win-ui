#include <ai_win_ui/host.h>

#include <Windows.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

struct StressState {
    std::unique_ptr<ai_win_ui::Host> host;
    int tickCount = 0;
    std::vector<std::string> cellIds;
};

static StressState* g_stress = nullptr;

static void OnStressTick(HWND, UINT, UINT_PTR, DWORD) {
    if (!g_stress || !g_stress->host) return;
    ++g_stress->tickCount;
    const int tick = g_stress->tickCount;

    g_stress->host->BeginBatchUpdate();
    double sum = 0.0;
    wchar_t buf[32];
    for (int i = 0; i < 100; ++i) {
        const double val = 100.0 + ((tick * 3 + i) % 100) * 0.13;
        sum += val;
        swprintf_s(buf, L"%.2f", val);
        g_stress->host->SetElementText(g_stress->cellIds[static_cast<size_t>(i)].c_str(), buf);
    }
    wchar_t hdr[64];
    swprintf_s(hdr, L"Ticks: %d | Sum: %.2f", tick, sum);
    g_stress->host->SetElementText("stress_tick_label", hdr);
    g_stress->host->EndBatchUpdate(false);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    StressState state;
    state.cellIds.reserve(100);
    for (int i = 0; i < 100; ++i) {
        state.cellIds.push_back("cell_" + std::to_string(i));
    }
    g_stress = &state;

    ai_win_ui::HostCreateInfo info{};
    info.showCommand = cmdShow;
    info.layoutPath = L"layouts/bench_stress_xml.xml";
    info.size = L"1000x700";
    info.ownMessagePump = false;

    state.host = ai_win_ui::Host::Create(instance, info);
    if (!state.host) {
        MessageBoxW(nullptr, L"Failed to load layouts/bench_stress_xml.xml", L"Error", MB_ICONERROR);
        return -1;
    }

    HWND hwnd = state.host->GetHwnd();
    if (hwnd) {
        SetWindowTextW(hwnd, L"XML 60Hz Stress Board (100 Cells + Rapid Typing)");
        SetTimer(hwnd, 2001, 16, OnStressTick);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!state.host->ProcessMessage(msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    g_stress = nullptr;
    return static_cast<int>(msg.wParam);
}

