#include "layout_gen.h"

namespace notes {

namespace {

// Case-insensitive substring test for filtering (both already lowered by caller
// for ASCII; CJK compares directly).
bool Contains(const std::wstring& hay, const std::wstring& needle) {
    if (needle.empty()) {
        return true;
    }
    return hay.find(needle) != std::wstring::npos;
}

} // namespace

std::string NoteLayoutXml(const Note& note) {
    const Palette& p = PaletteFor(note.color);
    const std::string& id = note.id;
    const char* pinIcon = note.pinned ? "icons/pin-on.svg" : "icons/pin-off.svg";
    const char* pinFg = note.pinned ? p.accent : p.text;

    std::string x;
    x.reserve(2048);
    x += "<Panel chrome=\"layered\" direction=\"column\" background=\"transparent\" padding=\"14,14,14,14\">";
    x += "<Panel name=\"card\" direction=\"column\" background=\""; x += p.card;
    x += "\" cornerRadius=\"16\" padding=\"0,0,0,0\" flexGrow=\"1\">";

    // Title bar (draggable). Left side is just a small color dot (drag handle,
    // no label — the note is narrow and 4 icon buttons already need ~130px).
    x += "<Panel role=\"titleBar\" hitTest=\"caption\" direction=\"row\" height=\"38\" background=\""; x += p.bar;
    x += "\" padding=\"10,0,4,0\" alignItems=\"center\" justifyContent=\"spaceBetween\">";
    x += "<Panel direction=\"row\" padding=\"0,0,0,0\" alignItems=\"center\" hitTest=\"caption\" flexGrow=\"1\" flexShrink=\"1\" flexBasis=\"0\">";
    x += "<Panel width=\"10\" height=\"10\" cornerRadius=\"5\" background=\""; x += p.accent; x += "\"/>";
    x += "</Panel>";
    // Fixed width required: an auto-width container Panel next to a flexGrow
    // sibling in a Row would otherwise report "fill available" as its measured
    // width, stealing the space meant for the flexGrow drag-handle area (see
    // main.cpp header-crowding fix notes). 4 buttons (26*3+30) + 3*2 spacing = 114.
    x += "<Panel direction=\"row\" padding=\"0,0,0,0\" alignItems=\"center\" spacing=\"2\" hitTest=\"client\" width=\"114\">";
    x += "<Button name=\"pinbtn\" text=\"\" icon=\""; x += pinIcon; x += "\" width=\"26\" height=\"26\" background=\"transparent\" foreground=\""; x += pinFg; x += "\" cornerRadius=\"6\" lockColors=\"true\" hitTest=\"client\" onClick=\"pin:"; x += id; x += "\"/>";
    x += "<Button text=\"\" icon=\"icons/plus.svg\" width=\"26\" height=\"26\" background=\"transparent\" foreground=\""; x += p.text; x += "\" cornerRadius=\"6\" lockColors=\"true\" hitTest=\"client\" onClick=\"new\"/>";
    x += "<Button text=\"\" icon=\"icons/trash.svg\" width=\"26\" height=\"26\" background=\"transparent\" foreground=\""; x += p.text; x += "\" cornerRadius=\"6\" lockColors=\"true\" hitTest=\"client\" onClick=\"del:"; x += id; x += "\"/>";
    x += "<Button variant=\"caption-close\" width=\"30\" height=\"26\" hitTest=\"client\" onClick=\"close:"; x += id; x += "\"/>";
    x += "</Panel>";
    x += "</Panel>"; // title bar

    // Body.
    x += "<Panel direction=\"column\" flexGrow=\"1\" padding=\"12,12,12,10\" spacing=\"8\" background=\""; x += p.card; x += "\">";
    x += "<TextInput name=\"title\" fontSize=\"15\" background=\""; x += p.input; x += "\" textColor=\""; x += p.text; x += "\" cornerRadius=\"8\" height=\"34\"/>";
    x += "<RichTextBox name=\"body\" fontSize=\"13\" background=\""; x += p.input; x += "\" textColor=\""; x += p.text; x += "\" borderColor=\""; x += p.bar; x += "\" cornerRadius=\"8\" flexGrow=\"1\"/>";

    // Color swatch strip (selectable background — 背景颜色可选择).
    x += "<Panel direction=\"row\" padding=\"0,0,0,0\" spacing=\"8\" alignItems=\"center\" height=\"22\">";
    for (const auto& sw : AllPalettes()) {
        x += "<Button text=\"\" width=\"18\" height=\"18\" cornerRadius=\"9\" background=\""; x += sw.card;
        x += "\" foreground=\""; x += sw.card; x += "\" lockColors=\"true\" onClick=\"color:"; x += id; x += ":"; x += sw.key; x += "\"/>";
    }
    x += "</Panel>";

    x += "</Panel>"; // body
    x += "</Panel>"; // card
    x += "</Panel>"; // layered root
    return x;
}

std::string ManagerLayoutXml(const std::vector<Note>& notes, const std::wstring& filter) {
    std::string x;
    x.reserve(4096);
    x += "<Panel chrome=\"layered\" direction=\"column\" background=\"transparent\" padding=\"16,16,16,16\">";
    x += "<Panel direction=\"column\" background=\"#0E1622\" cornerRadius=\"16\" padding=\"0,0,0,0\" flexGrow=\"1\">";

    // Title bar.
    x += "<Panel role=\"titleBar\" hitTest=\"caption\" direction=\"row\" height=\"46\" background=\"#121C2C\" padding=\"16,0,6,0\" alignItems=\"center\" justifyContent=\"spaceBetween\">";
    x += "<Panel direction=\"row\" padding=\"0,0,0,0\" alignItems=\"center\" hitTest=\"caption\" flexGrow=\"1\" flexShrink=\"1\" flexBasis=\"0\">";
    x += "<SvgIcon source=\"icons/note.svg\" width=\"18\" height=\"18\" tint=\"#FBE693\" margin=\"0,0,10,0\"/>";
    x += "<Label text=\"便签管理器\" fontSize=\"14\" color=\"#F2F6FB\"/>";
    x += "</Panel>";
    // Fixed width required (see NoteLayoutXml comment): 84 (new-note button) + 40
    // (close) + 6 spacing = 130.
    x += "<Panel direction=\"row\" padding=\"0,0,0,0\" alignItems=\"center\" spacing=\"6\" hitTest=\"client\" width=\"130\">";
    x += "<Button text=\"＋ 新建\" width=\"84\" fontSize=\"12\" height=\"30\" background=\"#1F6FEB\" foreground=\"#FFFFFF\" cornerRadius=\"8\" lockColors=\"true\" hitTest=\"client\" onClick=\"new\"/>";
    x += "<Button variant=\"caption-close\" width=\"40\" height=\"46\" hitTest=\"client\" onClick=\"managerClose\"/>";
    x += "</Panel>";
    x += "</Panel>"; // title bar

    // Content.
    x += "<Panel direction=\"column\" flexGrow=\"1\" padding=\"14,14,14,14\" spacing=\"10\" background=\"#0A121C\">";

    // Search row.
    x += "<Panel direction=\"row\" spacing=\"8\" alignItems=\"center\" height=\"34\">";
    x += "<TextInput name=\"search\" fontSize=\"13\" background=\"#16202E\" textColor=\"#EAF3FF\" cornerRadius=\"8\" height=\"34\" flexGrow=\"1\"/>";
    x += "<Button text=\"搜索\" fontSize=\"12\" height=\"30\" background=\"#22303F\" foreground=\"#EAF3FF\" cornerRadius=\"8\" lockColors=\"true\" onClick=\"search\"/>";
    x += "</Panel>";

    // Count how many rows match so we can show an empty state.
    int shown = 0;
    for (const auto& note : notes) {
        if (Contains(note.title, filter) || Contains(note.body, filter)) {
            ++shown;
        }
    }

    if (shown == 0) {
        x += "<Panel direction=\"column\" flexGrow=\"1\" alignItems=\"center\" justifyContent=\"center\">";
        x += "<Label text=\"还没有便签，点击「＋ 新建」创建一个\" fontSize=\"13\" color=\"#5C7085\"/>";
        x += "</Panel>";
    } else {
        x += "<ScrollViewer flexGrow=\"1\">";
        x += "<Panel direction=\"column\" spacing=\"8\">";
        for (const auto& note : notes) {
            if (!(Contains(note.title, filter) || Contains(note.body, filter))) {
                continue;
            }
            const Palette& p = PaletteFor(note.color);
            const std::string& id = note.id;
            x += "<Panel name=\"hoverpreview_"; x += id; x += "\" direction=\"row\" height=\"48\" background=\""; x += p.card;
            x += "\" cornerRadius=\"10\" padding=\"12,0,8,0\" alignItems=\"center\" spacing=\"10\">";
            x += "<Panel width=\"12\" height=\"12\" cornerRadius=\"6\" background=\""; x += p.accent; x += "\"/>";
            if (note.pinned) {
                x += "<Label text=\"★\" fontSize=\"13\" color=\""; x += p.accent; x += "\"/>";
            }
            x += "<Label name=\"mtitle_"; x += id; x += "\" text=\"便签\" fontSize=\"13\" color=\""; x += p.text; x += "\" flexGrow=\"1\"/>";
            x += "<Button text=\"\" icon=\"icons/open.svg\" width=\"32\" height=\"32\" background=\"transparent\" foreground=\""; x += p.text; x += "\" cornerRadius=\"6\" lockColors=\"true\" onClick=\"open:"; x += id; x += "\"/>";
            x += "<Button text=\"\" icon=\"icons/trash.svg\" width=\"32\" height=\"32\" background=\"transparent\" foreground=\""; x += p.text; x += "\" cornerRadius=\"6\" lockColors=\"true\" onClick=\"del:"; x += id; x += "\"/>";
            x += "</Panel>";
        }
        x += "</Panel>";
        x += "</ScrollViewer>";
    }

    x += "</Panel>"; // content
    x += "</Panel>"; // card
    x += "</Panel>"; // layered root
    return x;
}

std::string PreviewLayoutXml() {
    std::string x;
    x += "<Panel chrome=\"layered\" direction=\"column\" background=\"transparent\" padding=\"10,10,10,10\">";
    x += "<Panel direction=\"column\" background=\"#0E1622\" cornerRadius=\"12\" padding=\"14,14,14,14\" spacing=\"8\" flexGrow=\"1\">";
    x += "<Label name=\"ptitle\" text=\"\" fontSize=\"14\" color=\"#F5F8FC\"/>";
    x += "<RichTextBox name=\"pbody\" fontSize=\"12\" background=\"#0E1622\" textColor=\"#9AADC0\" borderColor=\"#0E1622\" cornerRadius=\"6\" flexGrow=\"1\"/>";
    x += "</Panel>";
    x += "</Panel>";
    return x;
}

} // namespace notes
