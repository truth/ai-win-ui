// Generates layout XML strings (UTF-8) for note windows, the manager, and the
// hover preview. User text is never embedded here — it is injected after the
// window is built via Host::SetElementText, so no XML escaping is required.
#pragma once

#include "note_store.h"

#include <string>
#include <vector>

namespace notes {

// A floating sticky-note window (chrome="layered"), tinted with the note palette.
// Editable fields are named "title" and "body"; the pin button is "pinbtn".
std::string NoteLayoutXml(const Note& note);

// The manager window listing all notes. Each row panel is named
// "hoverpreview_<id>" (drives hover preview) and its title label "mtitle_<id>".
std::string ManagerLayoutXml(const std::vector<Note>& notes, const std::wstring& filter);

// The hover preview popup skeleton. Labels "ptitle" / body "pbody" are filled at runtime.
std::string PreviewLayoutXml();

} // namespace notes
