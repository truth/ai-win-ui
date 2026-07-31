#include "note_store.h"

#include "layout_parser.h" // LayoutParser::ParseJson (public)

#include <Windows.h>

#include <fstream>
#include <sstream>

namespace notes {

// ---- Palette ---------------------------------------------------------------

static const std::vector<Palette> kPalettes = {
    {"yellow", L"柠檬黄", "#FFF4C2", "#FBE693", "#E6A100", "#3A2E00", "#7A6410", "#FFFBE8"},
    {"amber",  L"暖橙",   "#FFE0B8", "#FFC98A", "#E07A1F", "#442400", "#8A5518", "#FFF4E6"},
    {"pink",   L"樱粉",   "#FFD6E4", "#FFB6CE", "#D53E77", "#43101F", "#8A234A", "#FFF0F5"},
    {"green",  L"薄荷",   "#CBF3D6", "#A6E7B8", "#1F9D53", "#0B3319", "#1C6E3B", "#EEFCF1"},
    {"blue",   L"晴空",   "#CBE4FF", "#9FCBFF", "#2472D3", "#082647", "#1B5296", "#EEF6FF"},
    {"purple", L"薰衣草", "#E2D6FF", "#C9B6FF", "#7A45D3", "#2A1147", "#552A96", "#F5F0FF"},
    {"slate",  L"石墨",   "#20303F", "#182634", "#6EA0FF", "#EAF3FF", "#9AB0C6", "#1A2836"},
};

const std::vector<Palette>& AllPalettes() { return kPalettes; }

const Palette& PaletteFor(const std::string& key) {
    for (const auto& p : kPalettes) {
        if (key == p.key) {
            return p;
        }
    }
    return kPalettes.front();
}

// ---- UTF helpers -----------------------------------------------------------

std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) {
        return std::string();
    }
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? n : 0, '\0');
    if (n > 0) {
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), n, nullptr, nullptr);
    }
    return out;
}

std::wstring ToUtf16(const std::string& s) {
    if (s.empty()) {
        return std::wstring();
    }
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(n > 0 ? n : 0, L'\0');
    if (n > 0) {
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), n);
    }
    return out;
}

// ---- JSON writing (minimal, UTF-8) -----------------------------------------

static void AppendJsonString(std::string& out, const std::string& s) {
    out.push_back('"');
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    wsprintfA(buf, "\\u%04x", c & 0xff);
                    out += buf;
                } else {
                    out.push_back(c); // UTF-8 bytes pass through
                }
        }
    }
    out.push_back('"');
}

// ---- NoteStore -------------------------------------------------------------

NoteStore::NoteStore() {
    wchar_t appdata[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
    std::wstring dir = (n > 0 && n < MAX_PATH) ? std::wstring(appdata) : L".";
    dir += L"\\ai-win-ui-notes";
    CreateDirectoryW(dir.c_str(), nullptr);
    m_path = dir + L"\\notes.json";
}

Note* NoteStore::Find(const std::string& id) {
    for (auto& note : m_notes) {
        if (note.id == id) {
            return &note;
        }
    }
    return nullptr;
}

const Note* NoteStore::Find(const std::string& id) const {
    for (const auto& note : m_notes) {
        if (note.id == id) {
            return &note;
        }
    }
    return nullptr;
}

static int NoteIdNumber(const std::string& id) {
    // ids look like "n<number>"; extract the numeric suffix.
    int value = 0;
    for (char c : id) {
        if (c >= '0' && c <= '9') {
            value = value * 10 + (c - '0');
        }
    }
    return value;
}

Note& NoteStore::Add() {
    Note note;
    note.id = "n" + std::to_string(m_nextId++);
    note.color = "yellow";
    note.order = static_cast<long long>(m_notes.size());
    const int step = static_cast<int>(m_notes.size() % 8);
    note.x = 140 + step * 30;
    note.y = 140 + step * 30;
    m_notes.push_back(std::move(note));
    return m_notes.back();
}

bool NoteStore::Remove(const std::string& id) {
    for (auto it = m_notes.begin(); it != m_notes.end(); ++it) {
        if (it->id == id) {
            m_notes.erase(it);
            return true;
        }
    }
    return false;
}

void NoteStore::Load() {
    std::ifstream in(m_path, std::ios::binary);
    if (!in) {
        return;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();
    if (text.empty()) {
        return;
    }

    JsonValue root = LayoutParser::ParseJson(text);
    if (!root.IsObject()) {
        return;
    }

    m_notes.clear();
    const JsonValue& arr = root["notes"];
    if (arr.IsArray()) {
        for (size_t i = 0; i < arr.arrayValue.size(); ++i) {
            const JsonValue& n = arr.arrayValue[i];
            if (!n.IsObject()) {
                continue;
            }
            Note note;
            note.id = n["id"].IsString() ? n["id"].stringValue : ("n" + std::to_string(i + 1));
            note.title = ToUtf16(n["title"].IsString() ? n["title"].stringValue : std::string());
            note.body = ToUtf16(n["body"].IsString() ? n["body"].stringValue : std::string());
            note.color = n["color"].IsString() ? n["color"].stringValue : std::string("yellow");
            if (n["x"].IsNumber()) note.x = (int)n["x"].numberValue;
            if (n["y"].IsNumber()) note.y = (int)n["y"].numberValue;
            if (n["w"].IsNumber()) note.w = (int)n["w"].numberValue;
            if (n["h"].IsNumber()) note.h = (int)n["h"].numberValue;
            if (n["pinned"].IsBool()) note.pinned = n["pinned"].boolValue;
            if (n["open"].IsBool()) note.open = n["open"].boolValue;
            if (n["order"].IsNumber()) note.order = (long long)n["order"].numberValue;
            const int idNum = NoteIdNumber(note.id);
            if (idNum >= m_nextId) {
                m_nextId = idNum + 1;
            }
            m_notes.push_back(std::move(note));
        }
    }
    if (root["nextId"].IsNumber()) {
        m_nextId = (std::max)(m_nextId, (int)root["nextId"].numberValue);
    }
}

bool NoteStore::Save() const {
    std::string out;
    out.reserve(256 + m_notes.size() * 128);
    out += "{\n  \"nextId\": ";
    out += std::to_string(m_nextId);
    out += ",\n  \"notes\": [\n";
    for (size_t i = 0; i < m_notes.size(); ++i) {
        const Note& n = m_notes[i];
        out += "    { \"id\": ";
        AppendJsonString(out, n.id);
        out += ", \"title\": ";
        AppendJsonString(out, ToUtf8(n.title));
        out += ", \"body\": ";
        AppendJsonString(out, ToUtf8(n.body));
        out += ", \"color\": ";
        AppendJsonString(out, n.color);
        out += ", \"x\": " + std::to_string(n.x);
        out += ", \"y\": " + std::to_string(n.y);
        out += ", \"w\": " + std::to_string(n.w);
        out += ", \"h\": " + std::to_string(n.h);
        out += ", \"pinned\": ";
        out += n.pinned ? "true" : "false";
        out += ", \"open\": ";
        out += n.open ? "true" : "false";
        out += ", \"order\": " + std::to_string(n.order);
        out += " }";
        if (i + 1 < m_notes.size()) {
            out += ",";
        }
        out += "\n";
    }
    out += "  ]\n}\n";

    std::ofstream file(m_path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file.write(out.data(), (std::streamsize)out.size());
    return file.good();
}

} // namespace notes
