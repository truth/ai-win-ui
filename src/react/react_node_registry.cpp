#include "react_node_registry.h"

#include "layout_parser.h"
#include "style_catalog.h"
#include "theme.h"
#include "ui.h"

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace ai_win_ui::react {

namespace {

std::string Utf16ToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }
    std::string result(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

Color ColorFromArgbUint32(uint32_t argb) {
    const float a = static_cast<float>((argb >> 24) & 0xFFu) / 255.0f;
    return ColorFromHex(argb & 0x00FFFFFFu, a);
}

std::string ToLowerAscii(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

Color ResolveColorOp(const MutationOp& op, Theme* theme) {
    if (op.type == MutationOpType::SetPropColor) {
        return ColorFromArgbUint32(op.uintArg);
    }
    if (op.type == MutationOpType::SetPropStr) {
        if (op.strVal.rfind("$color.", 0) == 0 && theme) {
            if (auto resolved = theme->ResolveColor(op.strVal.substr(7))) {
                return *resolved;
            }
        }
        return LayoutParser::ColorFromString(op.strVal);
    }
    return Color{};
}

UIElement* WalkTreeItemByPath(TreeView* tv, const std::vector<int>& path) {
    if (!tv || path.empty()) {
        return nullptr;
    }
    UIElement* cur = nullptr;
    const auto& roots = tv->Children();
    if (path[0] < 0 || static_cast<size_t>(path[0]) >= roots.size()) {
        return nullptr;
    }
    cur = roots[static_cast<size_t>(path[0])].get();
    for (size_t i = 1; i < path.size(); ++i) {
        if (!cur) return nullptr;
        const auto& subs = cur->Children();
        const int idx = path[i];
        if (idx < 0 || static_cast<size_t>(idx) >= subs.size()) {
            return nullptr;
        }
        cur = subs[static_cast<size_t>(idx)].get();
    }
    return cur;
}

} // namespace

ReactNodeRegistry::ReactNodeRegistry(IReactHostSurface& surface, EventSink eventSink)
    : m_surface(surface), m_eventSink(std::move(eventSink)) {}

ReactNodeRegistry::~ReactNodeRegistry() {
    ResetTree();
}

UIElement* ReactNodeRegistry::FindNode(uint32_t nodeId) const {
    auto it = m_nodes.find(nodeId);
    return (it != m_nodes.end()) ? it->second.ptr : nullptr;
}

uint32_t ReactNodeRegistry::FindNodeId(UIElement* element) const {
    if (!element) return 0;
    auto it = m_ptrToId.find(element);
    return (it != m_ptrToId.end()) ? it->second : 0;
}

uint32_t ReactNodeRegistry::CurrentEventSeq(uint32_t nodeId) const {
    auto it = m_nodes.find(nodeId);
    return (it != m_nodes.end()) ? it->second.currentEventSeq : 0;
}

uint32_t ReactNodeRegistry::NextEventSeq(uint32_t nodeId) {
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return 0;
    }
    return ++it->second.currentEventSeq;
}

TreeView* ReactNodeRegistry::FindOwningTreeView(uint32_t nodeId) const {
    uint32_t curId = nodeId;
    while (curId != 0) {
        auto it = m_nodes.find(curId);
        if (it == m_nodes.end()) {
            return nullptr;
        }
        if (it->second.type == NodeTypeId::TreeView) {
            return dynamic_cast<TreeView*>(it->second.ptr);
        }
        curId = it->second.parentId;
    }
    return nullptr;
}

void ReactNodeRegistry::EraseSubtreeEntries(UIElement* el) {
    if (!el) return;
    for (const auto& child : el->Children()) {
        if (child) {
            EraseSubtreeEntries(child.get());
        }
    }
    auto revIt = m_ptrToId.find(el);
    if (revIt != m_ptrToId.end()) {
        m_nodes.erase(revIt->second);
        m_ptrToId.erase(revIt);
    }
}

void ReactNodeRegistry::ResetTree() {
    if (UIElement* root = m_surface.RootElement()) {
        m_surface.NotifySubtreeRemoving(root);
    }
    auto placeholder = std::make_unique<Panel>();
    placeholder->padding = Thickness{0, 0, 0, 0};
    placeholder->background = ColorFromHex(0x0B121A);
    m_surface.SetRootElement(std::move(placeholder));
    m_nodes.clear();
    m_ptrToId.clear();
}

void ReactNodeRegistry::CreateNode(uint32_t nodeId, NodeTypeId typeId) {
    if (nodeId == 0 || m_nodes.find(nodeId) != m_nodes.end()) {
        return;
    }

    std::unique_ptr<UIElement> el;
    switch (typeId) {
        case NodeTypeId::Panel: {
            auto panel = std::make_unique<Panel>();
            // Eliminate legacy Panel{8,8,8,8} padding pitfall in React JSX.
            panel->padding = Thickness{0.0f, 0.0f, 0.0f, 0.0f};
            el = std::move(panel);
            break;
        }
        case NodeTypeId::ScrollViewer:
            el = std::make_unique<ScrollViewer>();
            break;
        case NodeTypeId::Label:
            el = std::make_unique<Label>(L"");
            break;
        case NodeTypeId::Button:
            el = std::make_unique<Button>(L"");
            break;
        case NodeTypeId::TextInput:
            el = std::make_unique<TextInput>(L"");
            break;
        case NodeTypeId::Switch:
            el = std::make_unique<Switch>();
            break;
        case NodeTypeId::Slider:
            el = std::make_unique<Slider>(L"");
            break;
        case NodeTypeId::Checkbox:
            el = std::make_unique<Checkbox>(L"");
            break;
        case NodeTypeId::ProgressBar:
            el = std::make_unique<ProgressBar>(L"");
            break;
        case NodeTypeId::Badge:
            el = std::make_unique<Badge>();
            break;
        case NodeTypeId::Avatar:
            el = std::make_unique<Avatar>();
            break;
        case NodeTypeId::Divider:
            el = std::make_unique<Divider>();
            break;
        case NodeTypeId::Icon:
            el = std::make_unique<VectorIcon>("circle");
            break;
        case NodeTypeId::Breadcrumb:
            el = std::make_unique<Breadcrumb>();
            break;
        case NodeTypeId::BreadcrumbItem:
            el = std::make_unique<BreadcrumbItem>();
            break;
        case NodeTypeId::Pagination:
            el = std::make_unique<Pagination>();
            break;
        case NodeTypeId::TreeView:
            el = std::make_unique<TreeView>();
            break;
        case NodeTypeId::TreeItem:
            el = std::make_unique<TreeItem>();
            break;
        default:
            return;
    }

    UIElement* rawPtr = el.get();
    rawPtr->SetContext(&m_surface.Context());

    NodeEntry entry;
    entry.ptr = rawPtr;
    entry.ownedUnattached = std::move(el);
    entry.type = typeId;
    m_nodes.emplace(nodeId, std::move(entry));
    m_ptrToId.emplace(rawPtr, nodeId);
}

void ReactNodeRegistry::AppendChild(uint32_t parentId, uint32_t childId, std::unordered_set<TreeView*>& dirtyTrees) {
    auto pit = m_nodes.find(parentId);
    auto cit = m_nodes.find(childId);
    if (pit == m_nodes.end() || cit == m_nodes.end() || !pit->second.ptr || !cit->second.ptr) {
        return;
    }
    std::unique_ptr<UIElement> childOwned;
    if (cit->second.ownedUnattached) {
        childOwned = std::move(cit->second.ownedUnattached);
    } else if (cit->second.parentId != 0) {
        auto oldParent = m_nodes.find(cit->second.parentId);
        if (oldParent != m_nodes.end() && oldParent->second.ptr) {
            m_surface.NotifySubtreeRemoving(cit->second.ptr);
            childOwned = oldParent->second.ptr->RemoveChild(cit->second.ptr);
        }
    }
    if (!childOwned) {
        return;
    }
    cit->second.parentId = parentId;
    pit->second.ptr->AddChild(std::move(childOwned));

    if (TreeView* tv = FindOwningTreeView(parentId)) {
        dirtyTrees.insert(tv);
    }
}

void ReactNodeRegistry::InsertBefore(uint32_t parentId, uint32_t childId, uint32_t beforeId, std::unordered_set<TreeView*>& dirtyTrees) {
    auto pit = m_nodes.find(parentId);
    auto cit = m_nodes.find(childId);
    if (pit == m_nodes.end() || cit == m_nodes.end() || !pit->second.ptr || !cit->second.ptr) {
        return;
    }
    UIElement* beforePtr = FindNode(beforeId);

    std::unique_ptr<UIElement> childOwned;
    if (cit->second.ownedUnattached) {
        childOwned = std::move(cit->second.ownedUnattached);
    } else if (cit->second.parentId != 0) {
        auto oldParent = m_nodes.find(cit->second.parentId);
        if (oldParent != m_nodes.end() && oldParent->second.ptr) {
            if (cit->second.parentId != parentId) {
                m_surface.NotifySubtreeRemoving(cit->second.ptr);
            }
            childOwned = oldParent->second.ptr->RemoveChild(cit->second.ptr);
        }
    }
    if (!childOwned) {
        return;
    }
    cit->second.parentId = parentId;
    pit->second.ptr->InsertChildBefore(std::move(childOwned), beforePtr);

    if (TreeView* tv = FindOwningTreeView(parentId)) {
        dirtyTrees.insert(tv);
    }
}

void ReactNodeRegistry::RemoveChild(uint32_t parentId, uint32_t childId, std::unordered_set<TreeView*>& dirtyTrees) {
    auto pit = m_nodes.find(parentId);
    auto cit = m_nodes.find(childId);
    if (pit == m_nodes.end() || cit == m_nodes.end() || !pit->second.ptr || !cit->second.ptr) {
        return;
    }
    m_surface.NotifySubtreeRemoving(cit->second.ptr);
    std::unique_ptr<UIElement> detached = pit->second.ptr->RemoveChild(cit->second.ptr);
    cit->second.parentId = 0;
    if (detached) {
        cit->second.ownedUnattached = std::move(detached);
    }
    if (TreeView* tv = FindOwningTreeView(parentId)) {
        dirtyTrees.insert(tv);
    }
}

void ReactNodeRegistry::DestroyNode(uint32_t nodeId, std::unordered_set<TreeView*>& dirtyTrees) {
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return;
    }
    UIElement* ptr = it->second.ptr;
    const uint32_t parentId = it->second.parentId;
    TreeView* owningTv = FindOwningTreeView(parentId);
    if (owningTv && it->second.type == NodeTypeId::TreeItem) {
        dirtyTrees.insert(owningTv);
    }

    if (ptr) {
        m_surface.NotifySubtreeRemoving(ptr);
    }
    std::unique_ptr<UIElement> toDestroy;
    if (it->second.ownedUnattached) {
        toDestroy = std::move(it->second.ownedUnattached);
    } else if (parentId != 0) {
        if (UIElement* parent = FindNode(parentId)) {
            toDestroy = parent->RemoveChild(ptr);
        }
    }
    if (ptr) {
        EraseSubtreeEntries(ptr);
    } else {
        m_nodes.erase(it);
    }
    toDestroy.reset();
}

void ReactNodeRegistry::SetRoot(uint32_t nodeId) {
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return;
    }
    std::unique_ptr<UIElement> rootOwned;
    if (it->second.ownedUnattached) {
        rootOwned = std::move(it->second.ownedUnattached);
    } else {
        return;
    }
    UIElement* newRootPtr = it->second.ptr;
    if (UIElement* prevRoot = m_surface.RootElement(); prevRoot && prevRoot != newRootPtr) {
        m_surface.NotifySubtreeRemoving(prevRoot);
        EraseSubtreeEntries(prevRoot);
    }
    it = m_nodes.find(nodeId);
    if (it != m_nodes.end()) {
        it->second.isRoot = true;
        it->second.parentId = 0;
    }
    m_surface.SetRootElement(std::move(rootOwned));
}

void ReactNodeRegistry::ApplyEventMask(uint32_t nodeId, uint32_t mask) {
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end() || !it->second.ptr) {
        return;
    }
    NodeEntry& entry = it->second;
    entry.eventMask = mask;
    UIElement* el = entry.ptr;

    switch (entry.type) {
        case NodeTypeId::Button: {
            if (auto* btn = dynamic_cast<Button*>(el)) {
                if (mask & EventMask::Click) {
                    btn->SetOnClick([this, nodeId]() {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::Click;
                        ev.eventSeq = NextEventSeq(nodeId);
                        if (m_eventSink) m_eventSink(std::move(ev));
                    });
                } else {
                    btn->SetOnClick(nullptr);
                }
            }
            break;
        }
        case NodeTypeId::TextInput: {
            if (auto* ti = dynamic_cast<TextInput*>(el)) {
                if (mask & EventMask::TextChanged) {
                    ti->onTextChanged = [this, nodeId](const std::wstring& txt) {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::TextChanged;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.strVal = Utf16ToUtf8(txt);
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    ti->onTextChanged = nullptr;
                }
                if (mask & EventMask::Submit) {
                    ti->onSubmit = [this, nodeId](const std::wstring& txt) {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::Submit;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.strVal = Utf16ToUtf8(txt);
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    ti->onSubmit = nullptr;
                }
            }
            break;
        }
        case NodeTypeId::Switch: {
            if (auto* sw = dynamic_cast<Switch*>(el)) {
                if (mask & EventMask::Change) {
                    sw->onChange = [this, nodeId, sw]() {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::Change;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.boolVal = sw->checked;
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    sw->onChange = nullptr;
                }
            }
            break;
        }
        case NodeTypeId::Checkbox: {
            if (auto* cb = dynamic_cast<Checkbox*>(el)) {
                if (mask & EventMask::Change) {
                    cb->onChange = [this, nodeId](bool checked) {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::Change;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.boolVal = checked;
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    cb->onChange = nullptr;
                }
            }
            break;
        }
        case NodeTypeId::Slider: {
            if (auto* sl = dynamic_cast<Slider*>(el)) {
                if (mask & EventMask::Change) {
                    sl->onChange = [this, nodeId](float val) {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::Change;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.numVal = static_cast<double>(val);
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    sl->onChange = nullptr;
                }
            }
            break;
        }
        case NodeTypeId::BreadcrumbItem: {
            if (auto* bi = dynamic_cast<BreadcrumbItem*>(el)) {
                if (mask & EventMask::Click) {
                    bi->onClick = [this, nodeId]() {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::Click;
                        ev.eventSeq = NextEventSeq(nodeId);
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    bi->onClick = nullptr;
                }
            }
            break;
        }
        case NodeTypeId::Pagination: {
            if (auto* pg = dynamic_cast<Pagination*>(el)) {
                if (mask & EventMask::PageChange) {
                    pg->onPageChange = [this, nodeId](int page, int pageSize) {
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::PageChange;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.intVal = page;
                        ev.intVal2 = pageSize;
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    pg->onPageChange = nullptr;
                }
            }
            break;
        }
        case NodeTypeId::TreeView: {
            if (auto* tv = dynamic_cast<TreeView*>(el)) {
                if (mask & EventMask::TreeSelect) {
                    tv->onSelect = [this, nodeId, tv](const std::vector<int>& path, const std::wstring& label) {
                        UIElement* itemEl = WalkTreeItemByPath(tv, path);
                        const uint32_t itemNodeId = FindNodeId(itemEl);
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::TreeSelect;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.intVal = static_cast<int32_t>(itemNodeId);
                        ev.strVal = Utf16ToUtf8(label);
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    tv->onSelect = nullptr;
                }
                if (mask & EventMask::TreeCheck) {
                    tv->onCheck = [this, nodeId, tv](const std::vector<int>& path, bool checked) {
                        UIElement* itemEl = WalkTreeItemByPath(tv, path);
                        const uint32_t itemNodeId = FindNodeId(itemEl);
                        UiEvent ev;
                        ev.nodeId = nodeId;
                        ev.type = EventTypeId::TreeCheck;
                        ev.eventSeq = NextEventSeq(nodeId);
                        ev.intVal = static_cast<int32_t>(itemNodeId);
                        ev.boolVal = checked;
                        if (m_eventSink) m_eventSink(std::move(ev));
                    };
                } else {
                    tv->onCheck = nullptr;
                }
            }
            break;
        }
        default:
            break;
    }
}

void ReactNodeRegistry::ApplyProp(
    const MutationOp& op,
    bool& outNeedsLayout,
    bool& outNeedsPaint,
    std::unordered_set<TreeView*>& dirtyTrees) {
    auto it = m_nodes.find(op.nodeId);
    if (it == m_nodes.end() || !it->second.ptr) {
        return;
    }
    NodeEntry& entry = it->second;
    UIElement* el = entry.ptr;
    Theme* theme = m_surface.GetTheme();
    StyleCatalog* catalog = m_surface.GetStyleCatalog();
    const PropId prop = static_cast<PropId>(op.propId);

    // Fabric-style stale event echo suppression for controlled interactive inputs.
    auto isStaleEcho = [&]() -> bool {
        return op.ackSeq != 0 && op.ackSeq < entry.currentEventSeq;
    };

    auto touchLayout = [&]() { outNeedsLayout = true; };
    auto touchPaint = [&]() {
        outNeedsPaint = true;
        el->InvalidateSelf(6.0f);
    };

    // Common UIElement properties
    switch (prop) {
        case PropId::Name:
            el->SetName(op.strVal);
            return;
        case PropId::Width:
            el->SetFixedWidth(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::Height:
            el->SetFixedHeight(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::MinWidth:
            el->SetMinWidth(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::MaxWidth:
            el->SetMaxWidth(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::MinHeight:
            el->SetMinHeight(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::MaxHeight:
            el->SetMaxHeight(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::FlexGrow:
            el->SetFlexGrow(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::FlexShrink:
            el->SetFlexShrink(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::FlexBasis:
            el->SetFlexBasis(static_cast<float>(op.numVal));
            touchLayout();
            return;
        case PropId::AlignSelf: {
            const std::string v = ToLowerAscii(op.strVal);
            if (v == "stretch") el->SetAlignSelf(UIElement::SelfAlign::Stretch);
            else if (v == "start" || v == "flex-start") el->SetAlignSelf(UIElement::SelfAlign::Start);
            else if (v == "center") el->SetAlignSelf(UIElement::SelfAlign::Center);
            else if (v == "end" || v == "flex-end") el->SetAlignSelf(UIElement::SelfAlign::End);
            else el->SetAlignSelf(UIElement::SelfAlign::Auto);
            touchLayout();
            return;
        }
        case PropId::Margin:
            el->SetMargin(Thickness{op.tL, op.tT, op.tR, op.tB});
            touchLayout();
            return;
        case PropId::Border:
            el->SetBorder(Thickness{op.tL, op.tT, op.tR, op.tB});
            touchLayout();
            return;
        case PropId::Disabled:
            el->SetDisabled(op.boolVal);
            touchPaint();
            return;
        case PropId::Tooltip:
            el->SetTooltip(LayoutParser::Utf8ToUtf16(op.strVal));
            return;
        case PropId::StyleRef: {
            if (catalog) {
                std::string key = StyleCatalog::ParseStyleToken(op.strVal);
                if (key.empty()) key = op.strVal;
                if (auto resolved = catalog->Resolve(key)) {
                    el->SetStyleFromCatalog(key, *resolved);
                    touchPaint();
                }
            }
            return;
        }
        case PropId::HitTestRole: {
            const std::string v = ToLowerAscii(op.strVal);
            if (v == "caption") el->SetHitTestRole(UIElement::HitTestRole::Caption);
            else if (v == "client") el->SetHitTestRole(UIElement::HitTestRole::Client);
            else el->SetHitTestRole(UIElement::HitTestRole::Default);
            return;
        }
        default:
            break;
    }

    // Type-specific properties
    switch (entry.type) {
        case NodeTypeId::Panel: {
            auto* p = static_cast<Panel*>(el);
            switch (prop) {
                case PropId::Direction: {
                    const std::string v = ToLowerAscii(op.strVal);
                    p->direction = (v == "row") ? Panel::Direction::Row : Panel::Direction::Column;
                    touchLayout();
                    break;
                }
                case PropId::AlignItems: {
                    const std::string v = ToLowerAscii(op.strVal);
                    if (v == "start" || v == "flex-start") p->alignItems = Panel::AlignItems::Start;
                    else if (v == "center") p->alignItems = Panel::AlignItems::Center;
                    else if (v == "end" || v == "flex-end") p->alignItems = Panel::AlignItems::End;
                    else p->alignItems = Panel::AlignItems::Stretch;
                    touchLayout();
                    break;
                }
                case PropId::JustifyContent: {
                    const std::string v = ToLowerAscii(op.strVal);
                    if (v == "center") p->justifyContent = Panel::JustifyContent::Center;
                    else if (v == "end" || v == "flex-end") p->justifyContent = Panel::JustifyContent::End;
                    else if (v == "spacebetween" || v == "space-between") p->justifyContent = Panel::JustifyContent::SpaceBetween;
                    else p->justifyContent = Panel::JustifyContent::Start;
                    touchLayout();
                    break;
                }
                case PropId::Wrap: {
                    const std::string v = ToLowerAscii(op.strVal);
                    p->wrap = (v == "wrap" || op.boolVal) ? Panel::Wrap::Wrap : Panel::Wrap::NoWrap;
                    touchLayout();
                    break;
                }
                case PropId::Padding:
                    p->padding = Thickness{op.tL, op.tT, op.tR, op.tB};
                    touchLayout();
                    break;
                case PropId::Spacing:
                    p->spacing = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::CornerRadius:
                    p->cornerRadius = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::Background:
                    p->background = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    p->borderColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::ShadowBlur:
                    p->shadow.enabled = op.numVal > 0.0;
                    p->shadow.blur = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::ShadowOffsetX:
                    p->shadow.enabled = true;
                    p->shadow.offsetX = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::ShadowOffsetY:
                    p->shadow.enabled = true;
                    p->shadow.offsetY = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::ShadowColor:
                    p->shadow.enabled = true;
                    p->shadow.color = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::GradientStart:
                    p->gradient.enabled = true;
                    p->gradient.start = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::GradientEnd:
                    p->gradient.enabled = true;
                    p->gradient.end = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::GradientAngle:
                    p->gradient.enabled = true;
                    p->gradient.angleDegrees = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::ScrollViewer: {
            auto* sv = static_cast<ScrollViewer*>(el);
            switch (prop) {
                case PropId::ScrollX:
                    sv->scrollX = op.boolVal;
                    touchLayout();
                    break;
                case PropId::ScrollY:
                    sv->scrollY = op.boolVal;
                    touchLayout();
                    break;
                case PropId::ShowScrollbars:
                    sv->showScrollbars = op.boolVal;
                    touchPaint();
                    break;
                case PropId::BarThickness:
                    sv->barThickness = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::Background:
                    sv->background = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::TrackColor:
                    sv->trackColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::ThumbColor:
                    sv->thumbColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Label: {
            auto* lbl = static_cast<Label*>(el);
            switch (prop) {
                case PropId::Text:
                    lbl->SetText(LayoutParser::Utf8ToUtf16(op.strVal));
                    if (!lbl->HasFixedWidth() || !lbl->HasFixedHeight()) {
                        touchLayout();
                    } else {
                        touchPaint();
                    }
                    break;
                case PropId::FontSize:
                    lbl->m_fontSize = static_cast<float>(op.numVal);
                    if (!lbl->HasFixedWidth() || !lbl->HasFixedHeight()) touchLayout();
                    else touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    lbl->m_color = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::FontWeight:
                    lbl->m_fontWeight = static_cast<uint16_t>(std::clamp(op.numVal, 100.0, 900.0));
                    lbl->m_bold = (lbl->m_fontWeight >= 600);
                    touchPaint();
                    break;
                case PropId::Bold:
                    lbl->m_bold = op.boolVal;
                    touchPaint();
                    break;
                case PropId::Italic:
                    lbl->m_italic = op.boolVal;
                    touchPaint();
                    break;
                case PropId::Ellipsis:
                    lbl->SetEllipsis(op.boolVal);
                    touchPaint();
                    break;
                case PropId::HorizontalAlign: {
                    const std::string v = ToLowerAscii(op.strVal);
                    if (v == "center") lbl->m_horizontalAlign = TextHorizontalAlign::Center;
                    else if (v == "end" || v == "right") lbl->m_horizontalAlign = TextHorizontalAlign::End;
                    else lbl->m_horizontalAlign = TextHorizontalAlign::Start;
                    touchPaint();
                    break;
                }
                case PropId::VerticalAlign: {
                    const std::string v = ToLowerAscii(op.strVal);
                    if (v == "center") lbl->m_verticalAlign = TextVerticalAlign::Center;
                    else if (v == "end" || v == "bottom") lbl->m_verticalAlign = TextVerticalAlign::End;
                    else lbl->m_verticalAlign = TextVerticalAlign::Start;
                    touchPaint();
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Button: {
            auto* btn = static_cast<Button*>(el);
            switch (prop) {
                case PropId::Text:
                    btn->SetText(LayoutParser::Utf8ToUtf16(op.strVal));
                    if (!btn->HasFixedWidth()) touchLayout();
                    else touchPaint();
                    break;
                case PropId::FontSize:
                    btn->fontSize = static_cast<float>(op.numVal);
                    btn->LockColors();
                    if (!btn->HasFixedWidth() || !btn->HasFixedHeight()) touchLayout();
                    else touchPaint();
                    break;
                case PropId::CornerRadius:
                    btn->cornerRadius = static_cast<float>(op.numVal);
                    btn->LockColors();
                    touchPaint();
                    break;
                case PropId::Background:
                    btn->background = ResolveColorOp(op, theme);
                    btn->LockColors();
                    touchPaint();
                    break;
                case PropId::Foreground:
                case PropId::Color:
                    btn->foreground = ResolveColorOp(op, theme);
                    btn->LockColors();
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    if (!btn->MutableStyle().base.decoration.has_value()) {
                        btn->MutableStyle().base.decoration = BoxDecoration{};
                    }
                    btn->MutableStyle().base.decoration->border.color = ResolveColorOp(op, theme);
                    btn->LockColors();
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::TextInput: {
            auto* ti = static_cast<TextInput*>(el);
            switch (prop) {
                case PropId::Text: {
                    if (isStaleEcho()) {
                        return;
                    }
                    const std::wstring nextText = LayoutParser::Utf8ToUtf16(op.strVal);
                    if (nextText == ti->Text()) {
                        return;
                    }
                    ti->SetTextPreserveCaret(nextText);
                    if (!ti->HasFixedWidth() && ti->FlexGrow() <= 0.0f) touchLayout();
                    else touchPaint();
                    break;
                }
                case PropId::Placeholder:
                    ti->SetPlaceholder(LayoutParser::Utf8ToUtf16(op.strVal));
                    touchPaint();
                    break;
                case PropId::PlaceholderColor:
                    ti->SetPlaceholderColor(ResolveColorOp(op, theme));
                    ti->LockColors();
                    touchPaint();
                    break;
                case PropId::FontSize:
                    ti->SetFontSize(static_cast<float>(op.numVal));
                    ti->LockColors();
                    touchPaint();
                    break;
                case PropId::CornerRadius:
                    ti->SetCornerRadius(static_cast<float>(op.numVal));
                    ti->LockColors();
                    touchPaint();
                    break;
                case PropId::Background:
                    ti->SetBackgroundColor(ResolveColorOp(op, theme));
                    ti->LockColors();
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    ti->SetBorderColor(ResolveColorOp(op, theme));
                    ti->LockColors();
                    touchPaint();
                    break;
                case PropId::FocusBorderColor:
                    ti->SetFocusBorderColor(ResolveColorOp(op, theme));
                    ti->LockColors();
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    ti->SetTextColor(ResolveColorOp(op, theme));
                    ti->LockColors();
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Switch: {
            auto* sw = static_cast<Switch*>(el);
            switch (prop) {
                case PropId::Checked:
                    if (isStaleEcho()) return;
                    sw->checked = op.boolVal;
                    touchPaint();
                    break;
                case PropId::LabelText:
                case PropId::Text:
                    sw->label = LayoutParser::Utf8ToUtf16(op.strVal);
                    if (!sw->HasFixedWidth()) touchLayout();
                    else touchPaint();
                    break;
                case PropId::OnText:
                    sw->onText = LayoutParser::Utf8ToUtf16(op.strVal);
                    touchPaint();
                    break;
                case PropId::OffText:
                    sw->offText = LayoutParser::Utf8ToUtf16(op.strVal);
                    touchPaint();
                    break;
                case PropId::ActiveColor:
                    sw->activeColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::InactiveColor:
                    sw->inactiveColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    sw->labelColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::FontSize:
                    sw->fontSize = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Slider: {
            auto* sl = static_cast<Slider*>(el);
            switch (prop) {
                case PropId::Value:
                    if (isStaleEcho()) return;
                    sl->SetValue(static_cast<float>(op.numVal));
                    touchPaint();
                    break;
                case PropId::Min:
                    sl->SetRange(static_cast<float>(op.numVal), sl->Max());
                    touchPaint();
                    break;
                case PropId::Max:
                    sl->SetRange(sl->Min(), static_cast<float>(op.numVal));
                    touchPaint();
                    break;
                case PropId::Step:
                    sl->SetStep(static_cast<float>(op.numVal));
                    break;
                case PropId::Text:
                case PropId::LabelText:
                    sl->SetText(LayoutParser::Utf8ToUtf16(op.strVal));
                    touchPaint();
                    break;
                case PropId::Background:
                    sl->background = ResolveColorOp(op, theme);
                    sl->LockColors();
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    sl->borderColor = ResolveColorOp(op, theme);
                    sl->LockColors();
                    touchPaint();
                    break;
                case PropId::HighlightColor:
                case PropId::ActiveColor:
                case PropId::FillColor:
                    sl->SetHighlightColor(ResolveColorOp(op, theme));
                    sl->LockColors();
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    sl->textColor = ResolveColorOp(op, theme);
                    sl->LockColors();
                    touchPaint();
                    break;
                case PropId::CornerRadius:
                    sl->cornerRadius = static_cast<float>(op.numVal);
                    sl->LockColors();
                    touchPaint();
                    break;
                case PropId::FontSize:
                    sl->fontSize = static_cast<float>(op.numVal);
                    sl->LockColors();
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Checkbox: {
            auto* cb = static_cast<Checkbox*>(el);
            switch (prop) {
                case PropId::Checked:
                    if (isStaleEcho()) return;
                    cb->SetChecked(op.boolVal);
                    touchPaint();
                    break;
                case PropId::Text:
                    cb->SetText(LayoutParser::Utf8ToUtf16(op.strVal));
                    if (!cb->HasFixedWidth()) touchLayout();
                    else touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    cb->SetTextColor(ResolveColorOp(op, theme));
                    cb->LockColors();
                    touchPaint();
                    break;
                case PropId::FontSize:
                    cb->SetFontSize(static_cast<float>(op.numVal));
                    cb->LockColors();
                    if (!cb->HasFixedWidth()) touchLayout();
                    else touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::ProgressBar: {
            auto* pb = static_cast<ProgressBar*>(el);
            switch (prop) {
                case PropId::Value:
                    pb->SetValue(static_cast<float>(op.numVal));
                    touchPaint();
                    break;
                case PropId::Min:
                    pb->SetRange(static_cast<float>(op.numVal), pb->Max());
                    touchPaint();
                    break;
                case PropId::Max:
                    pb->SetRange(pb->Min(), static_cast<float>(op.numVal));
                    touchPaint();
                    break;
                case PropId::Text:
                case PropId::LabelText:
                    pb->SetLabel(LayoutParser::Utf8ToUtf16(op.strVal));
                    touchPaint();
                    break;
                case PropId::ShowValueText:
                    pb->SetShowValueText(op.boolVal);
                    touchPaint();
                    break;
                case PropId::BarHeight:
                    pb->barHeight = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::Background:
                    pb->background = ResolveColorOp(op, theme);
                    pb->LockColors();
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    pb->borderColor = ResolveColorOp(op, theme);
                    pb->LockColors();
                    touchPaint();
                    break;
                case PropId::TrackColor:
                    pb->SetTrackColor(ResolveColorOp(op, theme));
                    pb->LockColors();
                    touchPaint();
                    break;
                case PropId::FillColor:
                case PropId::ActiveColor:
                    pb->SetFillColor(ResolveColorOp(op, theme));
                    pb->LockColors();
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    pb->textColor = ResolveColorOp(op, theme);
                    pb->LockColors();
                    touchPaint();
                    break;
                case PropId::CornerRadius:
                    pb->cornerRadius = static_cast<float>(op.numVal);
                    pb->LockColors();
                    touchPaint();
                    break;
                case PropId::FontSize:
                    pb->fontSize = static_cast<float>(op.numVal);
                    pb->LockColors();
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Badge: {
            auto* bg = static_cast<Badge*>(el);
            switch (prop) {
                case PropId::Text:
                    bg->text = LayoutParser::Utf8ToUtf16(op.strVal);
                    if (!bg->HasFixedWidth()) touchLayout();
                    else touchPaint();
                    break;
                case PropId::Variant:
                    bg->variant = op.strVal;
                    touchPaint();
                    break;
                case PropId::Dot:
                    bg->dot = op.boolVal;
                    touchLayout();
                    break;
                case PropId::Icon:
                    bg->icon = op.strVal;
                    touchLayout();
                    break;
                case PropId::FontSize:
                    bg->fontSize = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::CornerRadius:
                    bg->cornerRadius = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::Background:
                    bg->customBackground = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    bg->customColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    bg->customBorderColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Avatar: {
            auto* av = static_cast<Avatar*>(el);
            switch (prop) {
                case PropId::Initials:
                case PropId::Text:
                    av->initials = LayoutParser::Utf8ToUtf16(op.strVal);
                    touchPaint();
                    break;
                case PropId::Icon:
                    av->icon = op.strVal;
                    touchPaint();
                    break;
                case PropId::Size:
                    av->size = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::Circle:
                    av->circle = op.boolVal;
                    touchPaint();
                    break;
                case PropId::CornerRadius:
                    av->cornerRadius = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::Background:
                    av->background = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    av->color = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    av->borderColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::ThicknessVal:
                case PropId::StrokeWidth:
                    av->borderWidth = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::Status:
                    av->status = op.strVal;
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Divider: {
            auto* dv = static_cast<Divider*>(el);
            switch (prop) {
                case PropId::Vertical:
                    dv->vertical = op.boolVal;
                    touchLayout();
                    break;
                case PropId::Dashed:
                    dv->dashed = op.boolVal;
                    touchPaint();
                    break;
                case PropId::ThicknessVal:
                    dv->thickness = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::Color:
                case PropId::BorderColor:
                    dv->color = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::Text:
                    dv->text = LayoutParser::Utf8ToUtf16(op.strVal);
                    touchLayout();
                    break;
                case PropId::Foreground:
                    dv->textColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::FontSize:
                    dv->fontSize = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Icon: {
            auto* ic = static_cast<VectorIcon*>(el);
            switch (prop) {
                case PropId::Icon:
                case PropId::Name:
                    ic->name = op.strVal;
                    touchPaint();
                    break;
                case PropId::Size:
                    ic->size = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    ic->color = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::Fill:
                case PropId::Background:
                    ic->fill = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::StrokeWidth:
                    ic->strokeWidth = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Breadcrumb: {
            auto* bc = static_cast<Breadcrumb*>(el);
            switch (prop) {
                case PropId::Separator:
                    bc->separator = LayoutParser::Utf8ToUtf16(op.strVal);
                    touchLayout();
                    break;
                case PropId::Spacing:
                    bc->spacing = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::FontSize:
                    bc->fontSize = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::SeparatorColor:
                case PropId::Color:
                    bc->separatorColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::BreadcrumbItem: {
            auto* bi = static_cast<BreadcrumbItem*>(el);
            switch (prop) {
                case PropId::Text:
                    bi->text = LayoutParser::Utf8ToUtf16(op.strVal);
                    touchLayout();
                    break;
                case PropId::Icon:
                    bi->icon = op.strVal;
                    touchLayout();
                    break;
                case PropId::Href:
                    bi->href = op.strVal;
                    break;
                case PropId::Active:
                    bi->active = op.boolVal;
                    touchPaint();
                    break;
                case PropId::FontSize:
                    bi->fontSize = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    bi->color = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::ActiveColor:
                    bi->activeColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::HoverColor:
                    bi->hoverColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::Pagination: {
            auto* pg = static_cast<Pagination*>(el);
            switch (prop) {
                case PropId::Current:
                    if (isStaleEcho()) return;
                    pg->current = std::clamp(static_cast<int>(op.numVal), 1, pg->GetTotalPages());
                    touchLayout();
                    break;
                case PropId::Total:
                    pg->total = std::max(0, static_cast<int>(op.numVal));
                    pg->current = std::clamp(pg->current, 1, pg->GetTotalPages());
                    touchLayout();
                    break;
                case PropId::PageSize:
                    pg->pageSize = std::max(1, static_cast<int>(op.numVal));
                    pg->current = std::clamp(pg->current, 1, pg->GetTotalPages());
                    touchLayout();
                    break;
                case PropId::ShowTotal:
                    pg->showTotal = op.boolVal;
                    touchLayout();
                    break;
                case PropId::ItemSize:
                    pg->itemSize = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::Spacing:
                    pg->spacing = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::FontSize:
                    pg->fontSize = static_cast<float>(op.numVal);
                    touchLayout();
                    break;
                case PropId::CornerRadius:
                    pg->cornerRadius = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::ActiveColor:
                    pg->activeColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::ActiveTextColor:
                    pg->activeTextColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::HoverColor:
                    pg->hoverColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    pg->textColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::DisabledColor:
                    pg->disabledColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    pg->borderColor = ResolveColorOp(op, theme);
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::TreeView: {
            auto* tv = static_cast<TreeView*>(el);
            switch (prop) {
                case PropId::ShowLines:
                    tv->showLines = op.boolVal;
                    touchPaint();
                    break;
                case PropId::ShowCheckBox:
                    tv->showCheckBox = op.boolVal;
                    touchPaint();
                    break;
                case PropId::ItemHeight:
                    tv->itemHeight = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::IndentWidth:
                    tv->indentWidth = static_cast<float>(op.numVal);
                    touchPaint();
                    break;
                case PropId::FontSize:
                    tv->fontSize = static_cast<float>(op.numVal);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::CornerRadius:
                    tv->cornerRadius = static_cast<float>(op.numVal);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::Background:
                    tv->background = ResolveColorOp(op, theme);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::BorderColor:
                    tv->borderColor = ResolveColorOp(op, theme);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::SelectedBackground:
                    tv->selectedBackground = ResolveColorOp(op, theme);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::HoverBackground:
                    tv->hoverBackground = ResolveColorOp(op, theme);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::Color:
                case PropId::Foreground:
                    tv->textColor = ResolveColorOp(op, theme);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::LineColor:
                    tv->lineColor = ResolveColorOp(op, theme);
                    tv->LockColors();
                    touchPaint();
                    break;
                case PropId::GlyphColor:
                    tv->glyphColor = ResolveColorOp(op, theme);
                    tv->LockColors();
                    touchPaint();
                    break;
                default:
                    break;
            }
            break;
        }
        case NodeTypeId::TreeItem: {
            auto* ti = static_cast<TreeItem*>(el);
            bool itemChanged = false;
            switch (prop) {
                case PropId::Header:
                case PropId::Text:
                    ti->header = LayoutParser::Utf8ToUtf16(op.strVal);
                    itemChanged = true;
                    break;
                case PropId::Icon:
                    ti->icon = op.strVal;
                    itemChanged = true;
                    break;
                case PropId::Expanded:
                    ti->expanded = op.boolVal;
                    itemChanged = true;
                    break;
                case PropId::Checked:
                    ti->isChecked = op.boolVal;
                    itemChanged = true;
                    break;
                case PropId::Selected:
                    ti->selected = op.boolVal;
                    itemChanged = true;
                    break;
                default:
                    break;
            }
            if (itemChanged) {
                if (TreeView* tv = FindOwningTreeView(op.nodeId)) {
                    dirtyTrees.insert(tv);
                    touchPaint();
                }
            }
            break;
        }
        default:
            break;
    }
}

void ReactNodeRegistry::ApplyBatch(MutationBatch&& batch) {
    if (batch.ops.empty()) {
        return;
    }

    m_surface.BeginBatchUpdate();

    bool structuralOrLayoutChanged = batch.needsLayout;
    bool didSetRoot = false;
    std::unordered_set<TreeView*> dirtyTrees;

    for (const auto& op : batch.ops) {
        switch (op.type) {
            case MutationOpType::CreateNode:
                CreateNode(op.nodeId, static_cast<NodeTypeId>(op.nodeType));
                break;
            case MutationOpType::SetPropBool:
            case MutationOpType::SetPropNum:
            case MutationOpType::SetPropColor:
            case MutationOpType::SetPropThickness:
            case MutationOpType::SetPropStr: {
                bool propNeedsLayout = false;
                bool propNeedsPaint = false;
                ApplyProp(op, propNeedsLayout, propNeedsPaint, dirtyTrees);
                if (propNeedsLayout) {
                    structuralOrLayoutChanged = true;
                }
                (void)propNeedsPaint;
                break;
            }
            case MutationOpType::SetEventMask:
                ApplyEventMask(op.nodeId, op.uintArg);
                break;
            case MutationOpType::AppendChild:
                AppendChild(op.parentId, op.nodeId, dirtyTrees);
                structuralOrLayoutChanged = true;
                break;
            case MutationOpType::InsertBefore:
                InsertBefore(op.parentId, op.nodeId, op.refId, dirtyTrees);
                structuralOrLayoutChanged = true;
                break;
            case MutationOpType::RemoveChild:
                RemoveChild(op.parentId, op.nodeId, dirtyTrees);
                structuralOrLayoutChanged = true;
                break;
            case MutationOpType::DestroyNode:
                DestroyNode(op.nodeId, dirtyTrees);
                structuralOrLayoutChanged = true;
                break;
            case MutationOpType::SetRoot:
                SetRoot(op.nodeId);
                structuralOrLayoutChanged = true;
                didSetRoot = true;
                break;
            case MutationOpType::SetWindowTitle:
                m_surface.SetWindowTitle(LayoutParser::Utf8ToUtf16(op.strVal));
                break;
            case MutationOpType::OpenDemo:
                m_surface.RequestOpenDemo(
                    LayoutParser::Utf8ToUtf16(op.strVal),
                    LayoutParser::Utf8ToUtf16(op.strVal2),
                    LayoutParser::Utf8ToUtf16(op.strVal3));
                break;
            case MutationOpType::ResetTree:
                dirtyTrees.clear();
                ResetTree();
                structuralOrLayoutChanged = true;
                break;
        }
    }

    for (TreeView* tv : dirtyTrees) {
        if (tv) {
            tv->RebuildNodesFromChildren();
            tv->InvalidateSelf(6.0f);
        }
    }

    m_surface.EndBatchUpdate(structuralOrLayoutChanged);

    if (didSetRoot && !m_firstRootCommitted) {
        m_firstRootCommitted = true;
        m_surface.NotifyFirstReactCommitReady();
    }
}

} // namespace ai_win_ui::react
