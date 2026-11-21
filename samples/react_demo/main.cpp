#include "ai_win_ui/host.h"
#include "react/react_host_bridge.h"

#include <Windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

namespace {

std::vector<std::wstring> ParseArgs() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args;
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        LocalFree(argv);
    }
    return args;
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    ai_win_ui::react::RegisterWithHost();

    std::wstring bundlePath = L"react/react_demo.bundle.js";
    std::wstring size = L"1280x840";
    std::wstring chrome;
    ai_win_ui::Backend backend = ai_win_ui::Backend::Default;
    bool watchMode = false;

    const auto args = ParseArgs();
    for (size_t i = 1; i < args.size(); ++i) {
        if ((args[i] == L"--bundle" || args[i] == L"--react") && i + 1 < args.size()) {
            bundlePath = args[++i];
        } else if (args[i] == L"--watch") {
            watchMode = true;
        } else if (args[i] == L"--size" && i + 1 < args.size()) {
            size = args[++i];
        } else if (args[i] == L"--chrome" && i + 1 < args.size()) {
            chrome = args[++i];
        } else if (args[i] == L"--renderer" && i + 1 < args.size()) {
            const auto& r = args[++i];
            if (r == L"skia") backend = ai_win_ui::Backend::Skia;
            else if (r == L"direct2d" || r == L"d2d") backend = ai_win_ui::Backend::Direct2D;
        }
    }

    ai_win_ui::HostCreateInfo info{};
    info.reactBundlePath = bundlePath.c_str();
    info.reactWatch = watchMode;
    info.chrome = chrome.empty() ? nullptr : chrome.c_str();
    info.size = size.c_str();
    info.renderer = backend;
    info.showCommand = cmdShow;

    auto host = ai_win_ui::Host::Create(instance, info);
    if (!host) {
        MessageBoxW(nullptr, L"Failed to initialize React UiHost.", L"Error", MB_ICONERROR);
        return -1;
    }
    return host->Run();
}

