// 便签 — a sticky-notes desktop app built on ai_win_ui.
//
// Features: CRUD notes, system tray + icon, layered floating note windows,
// pin-on-top (置顶钉在桌面), drag to arrange (title-bar move, positions persist),
// selectable background colors, a manager/list window, and hover preview popups.
//
// Build:  cmake --build build --config Debug --target sticky_notes
// Run:    build/Debug/sticky_notes.exe

#include <ai_win_ui/host.h>

#include "layout_gen.h"
#include "note_store.h"
#include "resource.h"

#include <Windows.h>
#include <shellapi.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

using ai_win_ui::Backend;
using ai_win_ui::Host;
using ai_win_ui::HostCreateInfo;

namespace {

constexpr wchar_t kAppWndClass[] = L"StickyNotesAppWindow";
constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT WM_APP_ACTION = WM_APP + 2;
constexpr UINT kTrayId = 1;

constexpr UINT IDM_NEW = 100;
constexpr UINT IDM_MANAGER = 101;
constexpr UINT IDM_SHOWALL = 102;
constexpr UINT IDM_EXIT = 103;

std::wstring GetModuleDir() {
    wchar_t path[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return L".";
    }
    std::wstring full(path, path + n);
    const auto slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : full.substr(0, slash);
}

std::wstring LowerAscii(const std::wstring& s) {
    std::wstring out = s;
    for (auto& c : out) {
        if (c >= L'A' && c <= L'Z') {
            c = static_cast<wchar_t>(c - L'A' + L'a');
        }
    }
    return out;
}

RECT WorkArea() {
    RECT wa{0, 0, 1280, 800};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    return wa;
}

void SetTopmost(HWND hwnd, bool topmost) {
    if (hwnd) {
        SetWindowPos(hwnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

// Load the compiled app icon (samples/sticky_notes/app.ico via app.rc) at a
// specific pixel size, matching the artwork used for the .exe's own file icon.
HICON LoadAppIcon(HINSTANCE instance, int size) {
    return static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
                                         size, size, LR_DEFAULTCOLOR));
}

// ---------------------------------------------------------------------------

class App {
public:
    explicit App(HINSTANCE instance) : m_instance(instance) {}

    bool Initialize();
    int Run();

    // Queue a custom onClick / hover id (called from a host action) for deferred,
    // re-entrancy-safe handling on the app window.
    void EnqueueAction(const std::string& id) {
        m_pending.push_back(id);
        if (m_appWnd) {
            PostMessageW(m_appWnd, WM_APP_ACTION, 0, 0);
        }
    }

    void DrainActions();
    void NewNote();
    void ShowManager();
    void ShowAllNotes();
    void RequestExit();
    void ShowTrayMenu();
    void OnAppDestroy();

private:
    static LRESULT CALLBACK AppWndProc(HWND, UINT, WPARAM, LPARAM);
    static void HostActionThunk(const char* id, void* user) {
        if (user) {
            static_cast<App*>(user)->EnqueueAction(id ? id : "");
        }
    }

    std::unique_ptr<Host> MakeHost(const std::string& xml, int w, int h,
                                   bool hasPos, int x, int y);
    void ApplyIcon(HWND hwnd) const;

    void HandleAction(const std::string& id);

    void OpenNoteWindow(const std::string& id, bool focus);
    void RecreateNoteWindow(const std::string& id);
    void SaveNoteFromWindow(const std::string& id);
    void CloseNoteWindow(const std::string& id);
    void DeleteNote(const std::string& id);
    void TogglePin(const std::string& id);
    void SetColor(const std::string& id, const std::string& colorKey);

    void RefreshManager();
    void CloseManager();
    void PopulateManagerTitles();
    void ApplySearch();

    void ShowPreviewFor(const std::string& hoverName);
    void HidePreview();

    void SaveOpenNotePositions();
    void PurgeClosedHosts();
    bool DispatchToHosts(MSG& msg);

    HINSTANCE m_instance = nullptr;
    HWND m_appWnd = nullptr;
    HICON m_iconBig = nullptr;
    HICON m_iconSmall = nullptr;
    NOTIFYICONDATAW m_nid{};

    notes::NoteStore m_store;
    std::map<std::string, std::unique_ptr<Host>> m_noteHosts;
    std::unique_ptr<Host> m_manager;
    std::unique_ptr<Host> m_preview;
    std::string m_previewNoteId;
    std::wstring m_filter;

    std::vector<std::string> m_pending;
};

std::unique_ptr<Host> App::MakeHost(const std::string& xml, int w, int h,
                                    bool hasPos, int x, int y) {
    HostCreateInfo info{};
    info.inlineLayoutXml = xml.c_str();
    info.chrome = L"layered";
    info.ownMessagePump = false;
    info.renderer = Backend::Skia; // crisp text + real SVG icons
    info.onAction = &App::HostActionThunk;
    info.actionUser = this;
    const std::wstring size = std::to_wstring(w) + L"x" + std::to_wstring(h);
    info.size = size.c_str();
    if (hasPos) {
        info.hasPosition = true;
        info.posX = x;
        info.posY = y;
    }
    auto host = Host::Create(m_instance, info);
    if (host) {
        ApplyIcon(host->GetHwnd());
    }
    return host;
}

void App::ApplyIcon(HWND hwnd) const {
    if (!hwnd) {
        return;
    }
    if (m_iconBig) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_iconBig));
    }
    if (m_iconSmall) {
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_iconSmall));
    }
}

bool App::Initialize() {
    m_store.Load();

    m_iconBig = LoadAppIcon(m_instance, GetSystemMetrics(SM_CXICON));
    m_iconSmall = LoadAppIcon(m_instance, GetSystemMetrics(SM_CXSMICON));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &App::AppWndProc;
    wc.hInstance = m_instance;
    wc.lpszClassName = kAppWndClass;
    wc.hIcon = m_iconBig;
    wc.hIconSm = m_iconSmall;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    // Hidden top-level window: owns the tray icon + deferred-action pump.
    m_appWnd = CreateWindowExW(0, kAppWndClass, L"便签", WS_OVERLAPPED,
                               0, 0, 0, 0, nullptr, nullptr, m_instance, this);
    if (!m_appWnd) {
        return false;
    }

    m_nid.cbSize = sizeof(m_nid);
    m_nid.hWnd = m_appWnd;
    m_nid.uID = kTrayId;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = m_iconSmall ? m_iconSmall : LoadIcon(nullptr, IDI_APPLICATION);
    lstrcpynW(m_nid.szTip, L"便签 — 桌面便签", ARRAYSIZE(m_nid.szTip));
    Shell_NotifyIconW(NIM_ADD, &m_nid);

    // Restore previously-open notes; always show the manager on launch.
    for (auto& note : m_store.Notes()) {
        if (note.open) {
            OpenNoteWindow(note.id, /*focus=*/false);
        }
    }
    ShowManager();
    return true;
}

int App::Run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!DispatchToHosts(msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        PurgeClosedHosts();
    }
    return static_cast<int>(msg.wParam);
}

bool App::DispatchToHosts(MSG& msg) {
    if (m_manager && !m_manager->IsClosed() && m_manager->ProcessMessage(msg)) {
        return true;
    }
    if (m_preview && !m_preview->IsClosed() && m_preview->ProcessMessage(msg)) {
        return true;
    }
    for (auto& kv : m_noteHosts) {
        if (kv.second && !kv.second->IsClosed() && kv.second->ProcessMessage(msg)) {
            return true;
        }
    }
    return false;
}

void App::PurgeClosedHosts() {
    for (auto it = m_noteHosts.begin(); it != m_noteHosts.end();) {
        if (!it->second || it->second->IsClosed()) {
            if (notes::Note* n = m_store.Find(it->first)) {
                n->open = false;
            }
            it = m_noteHosts.erase(it);
        } else {
            ++it;
        }
    }
    if (m_manager && m_manager->IsClosed()) {
        m_manager.reset();
    }
    if (m_preview && m_preview->IsClosed()) {
        m_preview.reset();
    }
}

void App::DrainActions() {
    std::vector<std::string> actions;
    actions.swap(m_pending);
    for (const auto& a : actions) {
        HandleAction(a);
    }
}

void App::HandleAction(const std::string& id) {
    auto starts = [&](const char* p) { return id.rfind(p, 0) == 0; };
    if (id == "new") {
        NewNote();
    } else if (id == "manager") {
        ShowManager();
    } else if (id == "managerClose") {
        CloseManager();
    } else if (id == "search") {
        ApplySearch();
    } else if (starts("close:")) {
        CloseNoteWindow(id.substr(6));
    } else if (starts("del:")) {
        DeleteNote(id.substr(4));
    } else if (starts("pin:")) {
        TogglePin(id.substr(4));
    } else if (starts("open:")) {
        OpenNoteWindow(id.substr(5), /*focus=*/true);
    } else if (starts("hover:")) {
        ShowPreviewFor(id.substr(6));
    } else if (starts("color:")) {
        const std::string rest = id.substr(6);
        const auto pos = rest.find(':');
        if (pos != std::string::npos) {
            SetColor(rest.substr(0, pos), rest.substr(pos + 1));
        }
    }
}

// ---- Notes -----------------------------------------------------------------

void App::NewNote() {
    notes::Note& note = m_store.Add();
    const std::string id = note.id;
    m_store.Save();
    OpenNoteWindow(id, /*focus=*/true);
    RefreshManager();
}

void App::OpenNoteWindow(const std::string& id, bool focus) {
    notes::Note* note = m_store.Find(id);
    if (!note) {
        return;
    }
    auto it = m_noteHosts.find(id);
    if (it != m_noteHosts.end() && it->second && !it->second->IsClosed()) {
        if (focus && it->second->GetHwnd()) {
            SetForegroundWindow(it->second->GetHwnd());
        }
        return;
    }

    const std::string xml = notes::NoteLayoutXml(*note);
    auto host = MakeHost(xml, note->w, note->h, /*hasPos=*/true, note->x, note->y);
    if (!host) {
        return;
    }
    host->SetElementText("title", note->title.c_str());
    host->SetElementText("body", note->body.c_str());
    HWND hwnd = host->GetHwnd();
    if (note->pinned) {
        SetTopmost(hwnd, true);
    }
    if (focus && hwnd) {
        SetForegroundWindow(hwnd);
    }
    note->open = true;
    m_noteHosts[id] = std::move(host);
}

// Rebuild an open note's window from its current model (color/pin baked into the
// generated XML + SVG icons), preserving edited text and window placement.
void App::RecreateNoteWindow(const std::string& id) {
    auto it = m_noteHosts.find(id);
    if (it == m_noteHosts.end() || !it->second || it->second->IsClosed()) {
        return;
    }
    SaveNoteFromWindow(id); // capture latest text + rect into the model first
    notes::Note* note = m_store.Find(id);
    if (!note) {
        return;
    }
    const int x = note->x, y = note->y, w = note->w, h = note->h;
    const bool pinned = note->pinned;
    m_noteHosts.erase(id);
    const std::string xml = notes::NoteLayoutXml(*note);
    auto host = MakeHost(xml, w, h, /*hasPos=*/true, x, y);
    if (host) {
        host->SetElementText("title", note->title.c_str());
        host->SetElementText("body", note->body.c_str());
        if (pinned) {
            SetTopmost(host->GetHwnd(), true);
        }
        m_noteHosts[id] = std::move(host);
    }
}

void App::SaveNoteFromWindow(const std::string& id) {
    auto it = m_noteHosts.find(id);
    if (it == m_noteHosts.end() || !it->second || it->second->IsClosed()) {
        return;
    }
    notes::Note* note = m_store.Find(id);
    if (!note) {
        return;
    }
    note->title = it->second->GetElementText("title");
    note->body = it->second->GetElementText("body");
    if (HWND hwnd = it->second->GetHwnd()) {
        RECT r{};
        if (GetWindowRect(hwnd, &r)) {
            note->x = r.left;
            note->y = r.top;
            note->w = (std::max)(120L, r.right - r.left);
            note->h = (std::max)(120L, r.bottom - r.top);
        }
    }
}

void App::CloseNoteWindow(const std::string& id) {
    SaveNoteFromWindow(id);
    if (notes::Note* note = m_store.Find(id)) {
        note->open = false;
    }
    m_noteHosts.erase(id);
    m_store.Save();
    RefreshManager();
}

void App::DeleteNote(const std::string& id) {
    const int r = MessageBoxW(m_appWnd, L"确定删除这条便签？此操作不可撤销。", L"删除便签",
                              MB_YESNO | MB_ICONWARNING);
    if (r != IDYES) {
        return;
    }
    m_noteHosts.erase(id);
    m_store.Remove(id);
    m_store.Save();
    RefreshManager();
}

void App::TogglePin(const std::string& id) {
    notes::Note* note = m_store.Find(id);
    if (!note) {
        return;
    }
    note->pinned = !note->pinned;
    m_store.Save();
    RecreateNoteWindow(id); // refresh the pin icon + apply/clear topmost
    RefreshManager();
}

void App::SetColor(const std::string& id, const std::string& colorKey) {
    notes::Note* note = m_store.Find(id);
    if (!note) {
        return;
    }
    SaveNoteFromWindow(id); // capture current text + rect before recreating
    note->color = colorKey;
    m_store.Save();
    RecreateNoteWindow(id);
    RefreshManager();
}

// ---- Manager ---------------------------------------------------------------

void App::ShowManager() {
    if (m_manager && !m_manager->IsClosed()) {
        if (m_manager->GetHwnd()) {
            SetForegroundWindow(m_manager->GetHwnd());
        }
        return;
    }
    const RECT wa = WorkArea();
    const int w = 380, h = 480;
    const int x = wa.right - w - 40;
    const int y = wa.top + 60;
    const std::string xml = notes::ManagerLayoutXml(m_store.Notes(), m_filter);
    m_manager = MakeHost(xml, w, h, /*hasPos=*/true, x, y);
    PopulateManagerTitles();
}

void App::RefreshManager() {
    if (!m_manager || m_manager->IsClosed()) {
        return;
    }
    // Same staleness issue as ShowPreviewFor: pull in live edits from any open
    // note windows so the manager's title list matches what's actually typed,
    // not just whatever was last saved to m_store.
    for (auto& kv : m_noteHosts) {
        SaveNoteFromWindow(kv.first);
    }
    int x = 0, y = 0, w = 380, h = 480;
    if (HWND hwnd = m_manager->GetHwnd()) {
        RECT r{};
        if (GetWindowRect(hwnd, &r)) {
            x = r.left;
            y = r.top;
            w = (std::max)(300L, r.right - r.left);
            h = (std::max)(240L, r.bottom - r.top);
        }
    }
    const std::string xml = notes::ManagerLayoutXml(m_store.Notes(), m_filter);
    m_manager = MakeHost(xml, w, h, /*hasPos=*/true, x, y);
    PopulateManagerTitles();
}

void App::CloseManager() {
    m_manager.reset();
}

void App::PopulateManagerTitles() {
    if (!m_manager) {
        return;
    }
    for (const auto& note : m_store.Notes()) {
        const std::wstring title = note.title.empty() ? std::wstring(L"（空白便签）") : note.title;
        const std::string name = "mtitle_" + note.id;
        m_manager->SetElementText(name.c_str(), title.c_str());
    }
    if (!m_filter.empty()) {
        m_manager->SetElementText("search", m_filter.c_str());
    }
}

void App::ApplySearch() {
    if (!m_manager) {
        return;
    }
    m_filter = LowerAscii(m_manager->GetElementText("search"));
    RefreshManager();
}

// ---- Hover preview ---------------------------------------------------------

void App::ShowPreviewFor(const std::string& hoverName) {
    if (hoverName.empty()) {
        HidePreview();
        return;
    }
    const std::string prefix = "hoverpreview_";
    if (hoverName.rfind(prefix, 0) != 0) {
        HidePreview();
        return;
    }
    const std::string id = hoverName.substr(prefix.size());
    // Pull the latest title/body out of the live note window (if it's open and
    // being edited) — m_store otherwise only reflects the last close/save, so
    // hovering a note you're actively typing into would show stale/blank text.
    SaveNoteFromWindow(id);
    notes::Note* note = m_store.Find(id);
    if (!note) {
        HidePreview();
        return;
    }

    POINT cur{};
    GetCursorPos(&cur);
    const int w = 268, h = 190;
    const int x = cur.x + 18;
    const int y = cur.y + 18;

    if (!m_preview) {
        const std::string xml = notes::PreviewLayoutXml();
        m_preview = MakeHost(xml, w, h, /*hasPos=*/true, x, y);
    }
    if (!m_preview) {
        return;
    }
    m_preview->SetElementText("ptitle", note->title.empty() ? L"（空白便签）" : note->title.c_str());
    std::wstring body = note->body;
    if (body.size() > 240) {
        body = body.substr(0, 240) + L"…";
    }
    if (body.empty()) {
        body = L"（没有内容）";
    }
    m_preview->SetElementText("pbody", body.c_str());
    SetWindowPos(m_preview->GetHwnd(), HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    m_previewNoteId = id;
}

void App::HidePreview() {
    if (m_preview && m_preview->GetHwnd()) {
        ShowWindow(m_preview->GetHwnd(), SW_HIDE);
    }
    m_previewNoteId.clear();
}

// ---- Tray / lifecycle ------------------------------------------------------

void App::ShowAllNotes() {
    for (auto& note : m_store.Notes()) {
        OpenNoteWindow(note.id, /*focus=*/false);
    }
}

void App::ShowTrayMenu() {
    POINT pt{};
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_NEW, L"新建便签");
    AppendMenuW(menu, MF_STRING, IDM_MANAGER, L"便签管理器");
    AppendMenuW(menu, MF_STRING, IDM_SHOWALL, L"显示全部便签");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"退出");
    SetForegroundWindow(m_appWnd); // required for the menu to dismiss correctly
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_appWnd, nullptr);
    DestroyMenu(menu);
}

void App::SaveOpenNotePositions() {
    for (auto& kv : m_noteHosts) {
        SaveNoteFromWindow(kv.first);
    }
    m_store.Save();
}

void App::RequestExit() {
    if (m_appWnd) {
        DestroyWindow(m_appWnd);
    }
}

void App::OnAppDestroy() {
    SaveOpenNotePositions();
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
    if (m_iconBig) {
        DestroyIcon(m_iconBig);
        m_iconBig = nullptr;
    }
    if (m_iconSmall) {
        DestroyIcon(m_iconSmall);
        m_iconSmall = nullptr;
    }
    m_noteHosts.clear();
    m_manager.reset();
    m_preview.reset();
}

LRESULT CALLBACK App::AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }
        case WM_APP_ACTION:
            if (app) {
                app->DrainActions();
            }
            return 0;
        case WM_TRAYICON:
            if (app) {
                const UINT ev = LOWORD(lParam);
                if (ev == WM_RBUTTONUP || ev == WM_CONTEXTMENU) {
                    app->ShowTrayMenu();
                } else if (ev == WM_LBUTTONDBLCLK) {
                    app->ShowManager();
                }
            }
            return 0;
        case WM_COMMAND:
            if (app) {
                switch (LOWORD(wParam)) {
                    case IDM_NEW: app->NewNote(); break;
                    case IDM_MANAGER: app->ShowManager(); break;
                    case IDM_SHOWALL: app->ShowAllNotes(); break;
                    case IDM_EXIT: app->RequestExit(); break;
                    default: break;
                }
            }
            return 0;
        case WM_DESTROY:
            if (app) {
                app->OnAppDestroy();
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetCurrentDirectoryW(GetModuleDir().c_str());
    ai_win_ui::InitializeProcessDpiAwareness();

    App app(instance);
    if (!app.Initialize()) {
        MessageBoxW(nullptr,
                    L"便签启动失败。\n\n请确认 resource/ 目录在可执行文件旁边。",
                    L"便签", MB_ICONERROR);
        return 1;
    }
    return app.Run();
}
