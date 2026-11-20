#pragma once

#include "react_bridge_iface.h"
#include "react_protocol.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

class TreeView;
class UIElement;

namespace ai_win_ui::react {

// Owned strictly by the Win32 UI thread. Maps JS-allocated monotonic nodeIds to
// live C++ UIElement instances and applies MutationBatches inside UiHost batches.
class ReactNodeRegistry {
public:
    using EventSink = std::function<void(UiEvent&&)>;

    ReactNodeRegistry(IReactHostSurface& surface, EventSink eventSink);
    ~ReactNodeRegistry();

    void ApplyBatch(MutationBatch&& batch);
    void ResetTree();

    size_t NodeCount() const { return m_nodes.size(); }
    UIElement* FindNode(uint32_t nodeId) const;
    uint32_t FindNodeId(UIElement* element) const;
    uint32_t CurrentEventSeq(uint32_t nodeId) const;

private:
    struct NodeEntry {
        UIElement* ptr = nullptr;
        std::unique_ptr<UIElement> ownedUnattached;
        NodeTypeId type = NodeTypeId::Panel;
        uint32_t parentId = 0;
        uint32_t currentEventSeq = 0;
        uint32_t eventMask = 0;
        bool isRoot = false;
    };

    void CreateNode(uint32_t nodeId, NodeTypeId typeId);
    void AppendChild(uint32_t parentId, uint32_t childId, std::unordered_set<TreeView*>& dirtyTrees);
    void InsertBefore(uint32_t parentId, uint32_t childId, uint32_t beforeId, std::unordered_set<TreeView*>& dirtyTrees);
    void RemoveChild(uint32_t parentId, uint32_t childId, std::unordered_set<TreeView*>& dirtyTrees);
    void DestroyNode(uint32_t nodeId, std::unordered_set<TreeView*>& dirtyTrees);
    void SetRoot(uint32_t nodeId);
    void ApplyProp(
        const MutationOp& op,
        bool& outNeedsLayout,
        bool& outNeedsPaint,
        std::unordered_set<TreeView*>& dirtyTrees);
    void ApplyEventMask(uint32_t nodeId, uint32_t mask);

    uint32_t NextEventSeq(uint32_t nodeId);
    TreeView* FindOwningTreeView(uint32_t nodeId) const;
    void EraseSubtreeEntries(UIElement* el);

    IReactHostSurface& m_surface;
    EventSink m_eventSink;
    std::unordered_map<uint32_t, NodeEntry> m_nodes;
    std::unordered_map<UIElement*, uint32_t> m_ptrToId;
    bool m_firstRootCommitted = false;
};

} // namespace ai_win_ui::react

