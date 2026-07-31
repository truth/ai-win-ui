// Sticky-notes data model + JSON persistence (%APPDATA%\ai-win-ui-notes\notes.json).
#pragma once

#include <string>
#include <vector>

namespace notes {

// A single sticky note. Text is stored as UTF-16 (Win32 native); persisted as UTF-8.
struct Note {
    std::string id;        // stable unique id, e.g. "n7"
    std::wstring title;    // short heading (single line)
    std::wstring body;     // free text
    std::string color;     // palette key: yellow|amber|pink|green|blue|purple|slate
    int x = 120;
    int y = 120;
    int w = 268;
    int h = 300;
    bool pinned = false;   // always-on-top (钉在桌面)
    bool open = true;      // whether its floating window is shown
    long long order = 0;   // sort order in the manager
};

// A named background palette entry. All values are #RRGGBB literals safe to embed
// directly into generated layout XML.
struct Palette {
    const char* key;      // color key
    const wchar_t* label; // display name (Chinese)
    const char* card;     // note body background
    const char* bar;      // title bar background
    const char* accent;   // dot / focus accent
    const char* text;     // primary text color
    const char* subtext;  // secondary text color
    const char* input;    // editable field background
};

// Built-in palette. Returns the entry for key, or the default (index 0) if unknown.
const Palette& PaletteFor(const std::string& key);
const std::vector<Palette>& AllPalettes();

// UTF conversion helpers (Win32 CP_UTF8).
std::string ToUtf8(const std::wstring& w);
std::wstring ToUtf16(const std::string& s);

class NoteStore {
public:
    NoteStore();

    void Load();          // read notes.json (no-op if absent)
    bool Save() const;    // write notes.json (creates the folder)

    std::vector<Note>& Notes() { return m_notes; }
    const std::vector<Note>& Notes() const { return m_notes; }

    Note* Find(const std::string& id);
    const Note* Find(const std::string& id) const;

    Note& Add();                       // create a new note with a fresh id + cascade position
    bool Remove(const std::string& id);

    const std::wstring& FilePath() const { return m_path; }

private:
    std::wstring m_path;
    std::vector<Note> m_notes;
    int m_nextId = 1;
};

} // namespace notes
