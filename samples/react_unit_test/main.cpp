#include "react_bridge_iface.h"
#include "react_js_runtime.h"
#include "react_node_registry.h"
#include "react_protocol.h"
#include "layout_engine.h"
#include "ui.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

class FakeSurface final : public ai_win_ui::IReactHostSurface {
public:
    FakeSurface() {
        m_context.dpi = 96;
        m_context.dpiScale = 1.0f;
        m_context.viewportWidth = 1000.0f;
        m_context.viewportHeight = 700.0f;
        m_layoutEngine = CreateDefaultLayoutEngine();
        m_context.layoutEngine = m_layoutEngine.get();
    }

    HWND GetHwnd() const override { return nullptr; }
    UIContext& Context() override { return m_context; }
    Theme* GetTheme() const override { return nullptr; }
    StyleCatalog* GetStyleCatalog() const override { return nullptr; }
    IResourceProvider* GetResourceProvider() const override { return nullptr; }

    void SetRootElement(std::unique_ptr<UIElement> root) override {
        m_root = std::move(root);
        if (m_root) {
            m_root->SetContext(&m_context);
        }
    }
    UIElement* RootElement() const override { return m_root.get(); }

    void NotifySubtreeRemoving(UIElement* subtree) override {
        ++notifyRemovingCount;
        lastRemovedSubtree = subtree;
    }

    void BeginBatchUpdate() override { ++batchDepth; }
    void EndBatchUpdate(bool requestLayout) override {
        if (batchDepth > 0) --batchDepth;
        if (requestLayout) {
            ++layoutCount;
            if (m_root) {
                m_root->Measure(1000.0f, 700.0f);
                m_root->Arrange(Rect::Make(0.0f, 0.0f, 1000.0f, 700.0f));
            }
        } else {
            ++paintOnlyCount;
        }
    }

    void SetWindowTitle(const std::wstring& title) override { lastTitle = title; }
    void RequestOpenDemo(const std::wstring&, const std::wstring&, const std::wstring&) override {}
    void NotifyFirstReactCommitReady() override { ++firstCommitReadyCount; }

    std::unique_ptr<UIElement> m_root;
    std::unique_ptr<ILayoutEngine> m_layoutEngine;
    UIContext m_context{};
    int batchDepth = 0;
    int layoutCount = 0;
    int paintOnlyCount = 0;
    int notifyRemovingCount = 0;
    int firstCommitReadyCount = 0;
    UIElement* lastRemovedSubtree = nullptr;
    std::wstring lastTitle;
};

std::string LoadShowcaseBundle() {
    const std::filesystem::path candidates[] = {
        std::filesystem::path("resource/react/react_demo.bundle.js"),
        std::filesystem::path("../resource/react/react_demo.bundle.js"),
        std::filesystem::path("../../resource/react/react_demo.bundle.js"),
    };
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p)) {
            std::ifstream ifs(p, std::ios::binary);
            std::ostringstream oss;
            oss << ifs.rdbuf();
            return oss.str();
        }
    }
    return {};
}

bool TestEventSeqSuppressionAndRegistry() {
    using namespace ai_win_ui::react;
    auto check = [](bool cond, const char* msg) -> bool {
        if (!cond) std::printf("[FAIL] %s\n", msg);
        return cond;
    };
    FakeSurface surface;
    std::vector<UiEvent> capturedEvents;
    ReactNodeRegistry reg(surface, [&](UiEvent&& ev) { capturedEvents.push_back(std::move(ev)); });

    // Build root Panel (id=1), TextInput (id=2), fixed-size Label (id=3), TreeView (id=4) + TreeItem (id=5)
    {
        MutationBatch b;
        b.needsLayout = false;
        b.ops.push_back({MutationOpType::CreateNode, 1, 0, 0, 0, 0, static_cast<uint16_t>(NodeTypeId::Panel)});
        b.ops.push_back({MutationOpType::CreateNode, 2, 0, 0, 0, 0, static_cast<uint16_t>(NodeTypeId::TextInput)});
        MutationOp setTiText;
        setTiText.type = MutationOpType::SetPropStr;
        setTiText.nodeId = 2;
        setTiText.propId = static_cast<uint16_t>(PropId::Text);
        setTiText.strVal = "init";
        b.ops.push_back(std::move(setTiText));

        MutationOp setEv;
        setEv.type = MutationOpType::SetEventMask;
        setEv.nodeId = 2;
        setEv.uintArg = EventMask::TextChanged;
        b.ops.push_back(setEv);

        b.ops.push_back({MutationOpType::CreateNode, 3, 0, 0, 0, 0, static_cast<uint16_t>(NodeTypeId::Label)});
        MutationOp wOp{MutationOpType::SetPropNum, 3, 0, 0, 0, 0, 0, static_cast<uint16_t>(PropId::Width)};
        wOp.numVal = 120.0;
        b.ops.push_back(wOp);
        MutationOp hOp{MutationOpType::SetPropNum, 3, 0, 0, 0, 0, 0, static_cast<uint16_t>(PropId::Height)};
        hOp.numVal = 24.0;
        b.ops.push_back(hOp);

        b.ops.push_back({MutationOpType::CreateNode, 4, 0, 0, 0, 0, static_cast<uint16_t>(NodeTypeId::TreeView)});
        b.ops.push_back({MutationOpType::CreateNode, 5, 0, 0, 0, 0, static_cast<uint16_t>(NodeTypeId::TreeItem)});
        MutationOp hdrOp;
        hdrOp.type = MutationOpType::SetPropStr;
        hdrOp.nodeId = 5;
        hdrOp.propId = static_cast<uint16_t>(PropId::Header);
        hdrOp.strVal = "NodeA";
        b.ops.push_back(std::move(hdrOp));

        b.ops.push_back({MutationOpType::AppendChild, 5, 4});
        b.ops.push_back({MutationOpType::AppendChild, 2, 1});
        b.ops.push_back({MutationOpType::AppendChild, 3, 1});
        b.ops.push_back({MutationOpType::AppendChild, 4, 1});
        b.ops.push_back({MutationOpType::SetRoot, 1});
        reg.ApplyBatch(std::move(b));
    }

    auto* rootPanel = dynamic_cast<Panel*>(surface.RootElement());
    if (!check(rootPanel != nullptr, "rootPanel != nullptr")) return false;
    // Pitfall check: React Panel defaults padding to 0
    if (!check(rootPanel->padding.left == 0.0f && rootPanel->padding.top == 0.0f, "panel padding == 0")) return false;
    if (!check(surface.firstCommitReadyCount == 1, "firstCommitReadyCount == 1")) return false;

    auto* ti = dynamic_cast<TextInput*>(reg.FindNode(2));
    if (!check(ti != nullptr && ti->Text() == L"init", "ti->Text() == init")) return false;

    // Simulate 3 fast optimistic keystrokes on UI thread
    ti->OnChar(L'X');
    ti->OnChar(L'Y');
    ti->OnChar(L'Z');
    if (!check(capturedEvents.size() == 3, "capturedEvents.size() == 3")) return false;
    if (!check(reg.CurrentEventSeq(2) == 3, "CurrentEventSeq(2) == 3")) return false;

    // Visual-only update on fixed-size Label must NOT trigger Yoga layout
    const int layoutBefore = surface.layoutCount;
    const int paintBefore = surface.paintOnlyCount;
    {
        MutationBatch visBatch;
        visBatch.needsLayout = false;
        MutationOp t;
        t.type = MutationOpType::SetPropStr;
        t.nodeId = 3;
        t.propId = static_cast<uint16_t>(PropId::Text);
        t.strVal = "Ticks: 99";
        visBatch.ops.push_back(std::move(t));
        reg.ApplyBatch(std::move(visBatch));
    }
    if (!check(surface.layoutCount == layoutBefore, "layoutCount unchanged on fixed label")) return false;
    if (!check(surface.paintOnlyCount == paintBefore + 1, "paintOnlyCount incremented")) return false;

    // Stale JS echo (ackSeq = 1 < currentEventSeq = 3) MUST be discarded!
    {
        MutationBatch stale;
        MutationOp op;
        op.type = MutationOpType::SetPropStr;
        op.nodeId = 2;
        op.propId = static_cast<uint16_t>(PropId::Text);
        op.strVal = "initX";
        op.ackSeq = 1;
        stale.ops.push_back(std::move(op));
        reg.ApplyBatch(std::move(stale));
    }
    if (!check(ti->Text() == L"initXYZ", "stale echo rejected")) return false;

    // Valid JS programmatic reset (ackSeq = 3 >= currentEventSeq = 3) MUST apply!
    {
        MutationBatch valid;
        MutationOp op;
        op.type = MutationOpType::SetPropStr;
        op.nodeId = 2;
        op.propId = static_cast<uint16_t>(PropId::Text);
        op.strVal = "";
        op.ackSeq = 3;
        valid.ops.push_back(std::move(op));
        reg.ApplyBatch(std::move(valid));
    }
    if (!check(ti->Text().empty(), "valid reset applied")) return false;

    // Verify InsertBefore reorders existing native siblings in-place
    {
        MutationBatch addSibs;
        addSibs.ops.push_back({MutationOpType::CreateNode, 6, 0, 0, 0, 0, static_cast<uint16_t>(NodeTypeId::Label)});
        addSibs.ops.push_back({MutationOpType::CreateNode, 7, 0, 0, 0, 0, static_cast<uint16_t>(NodeTypeId::Label)});
        addSibs.ops.push_back({MutationOpType::AppendChild, 6, 1});
        addSibs.ops.push_back({MutationOpType::AppendChild, 7, 1});
        reg.ApplyBatch(std::move(addSibs));

        UIElement* n6 = reg.FindNode(6);
        UIElement* n7 = reg.FindNode(7);
        const auto& kidsBefore = rootPanel->Children();
        if (!check(kidsBefore.size() >= 2 &&
                   kidsBefore[kidsBefore.size() - 2].get() == n6 &&
                   kidsBefore[kidsBefore.size() - 1].get() == n7,
                   "append order [n6, n7]")) {
            return false;
        }

        MutationBatch reorder;
        reorder.ops.push_back({MutationOpType::InsertBefore, 7, 1, 6});
        reg.ApplyBatch(std::move(reorder));

        const auto& kidsAfter = rootPanel->Children();
        if (!check(kidsAfter.size() >= 2 &&
                   kidsAfter[kidsAfter.size() - 2].get() == n7 &&
                   kidsAfter[kidsAfter.size() - 1].get() == n6,
                   "InsertBefore reordered [n7, n6]")) {
            return false;
        }
    }

    // Destroy subtree notifies surface before erasing node
    const int remBefore = surface.notifyRemovingCount;
    {
        MutationBatch del;
        del.ops.push_back({MutationOpType::DestroyNode, 5});
        reg.ApplyBatch(std::move(del));
    }
    if (!check(surface.notifyRemovingCount > remBefore, "notifyRemovingCount incremented")) return false;
    if (!check(reg.FindNode(5) == nullptr, "FindNode(5) == nullptr")) return false;

    std::printf("[PASS] TestEventSeqSuppressionAndRegistry\n");
    return true;
}

bool TestShowcaseBundleExecutionAndDiff() {
    using namespace ai_win_ui::react;
    auto check = [](bool cond, const char* msg) -> bool {
        if (!cond) std::printf("[FAIL] %s\n", msg);
        return cond;
    };
    const std::string bundle = LoadShowcaseBundle();
    if (bundle.empty()) {
        std::printf("[FAIL] Missing resource/react/react_demo.bundle.js\n");
        return false;
    }

    FakeSurface surface;
    std::vector<UiEvent> emitted;
    ReactNodeRegistry reg(surface, [&](UiEvent&& ev) { emitted.push_back(std::move(ev)); });
    ReactJsRuntime js([&](MutationBatch&& batch) {
        reg.ApplyBatch(std::move(batch));
    });

    if (!check(js.InitSynchronouslyForTest(), "InitSynchronouslyForTest")) return false;
    if (!check(js.EvalSynchronouslyForTest(bundle, "react_demo.bundle.js"), "EvalSynchronouslyForTest")) return false;

    if (!check(surface.RootElement() != nullptr, "surface.RootElement() != nullptr")) return false;
    if (reg.NodeCount() < 20) {
        std::printf("[FAIL] Expected >=20 mounted native nodes, got %zu\n", reg.NodeCount());
        return false;
    }
    if (!check(surface.layoutCount >= 1, "surface.layoutCount >= 1")) return false;
    if (surface.lastTitle.find(L"React JSX Showcase") == std::wstring::npos) {
        std::printf("[FAIL] Expected window title updated via useEffect\n");
        return false;
    }

    // Exercise TextInput event round-trip
    UIElement* tiEl = surface.RootElement()->FindElementByName("optTextInput");
    auto* ti = dynamic_cast<TextInput*>(tiEl);
    if (!ti) {
        std::printf("[FAIL] Could not find named TextInput optTextInput\n");
        return false;
    }
    const size_t lenBefore = ti->Text().size();
    ti->OnChar(L'!');
    if (!check(!emitted.empty(), "TextChanged event emitted")) return false;
    if (!check(ti->Text().size() == lenBefore + 1, "Optimistic input applied")) return false;

    const int layoutsBeforeEvent = surface.layoutCount;
    const int paintsBeforeEvent = surface.paintOnlyCount;
    js.DispatchEventSynchronouslyForTest(emitted.back());
    // Dispatching TextChanged updates fixed-width/height Chars label (visual-only!)
    if (!check(surface.paintOnlyCount > paintsBeforeEvent && surface.layoutCount == layoutsBeforeEvent,
               "Visual-only commit skipped Yoga layout")) {
        std::printf("  paintOnly %d->%d, layout %d->%d\n",
            paintsBeforeEvent, surface.paintOnlyCount, layoutsBeforeEvent, surface.layoutCount);
        return false;
    }

    // Exercise keyed list reconciliation via Only Open filter + Reverse + Add Item clicks
    Checkbox* onlyOpenCb = nullptr;
    Button* reverseBtn = nullptr;
    Button* addBtn = nullptr;
    Panel* taskListPanel = nullptr;
    {
        auto findKeyedCard = [&](auto& self, UIElement* node) -> void {
            if (taskListPanel || !node) return;
            const auto& ch = node->Children();
            if (ch.size() >= 2) {
                auto* hdrRow = dynamic_cast<Panel*>(ch[0].get());
                auto* listPanel = dynamic_cast<Panel*>(ch[1].get());
                if (hdrRow && listPanel && hdrRow->Children().size() >= 2) {
                    auto* actionsPanel = dynamic_cast<Panel*>(hdrRow->Children()[1].get());
                    if (actionsPanel && actionsPanel->Children().size() == 3 &&
                        dynamic_cast<Checkbox*>(actionsPanel->Children()[0].get()) &&
                        dynamic_cast<Button*>(actionsPanel->Children()[1].get()) &&
                        dynamic_cast<Button*>(actionsPanel->Children()[2].get())) {
                        onlyOpenCb = static_cast<Checkbox*>(actionsPanel->Children()[0].get());
                        reverseBtn = static_cast<Button*>(actionsPanel->Children()[1].get());
                        addBtn = static_cast<Button*>(actionsPanel->Children()[2].get());
                        taskListPanel = listPanel;
                        return;
                    }
                }
            }
            for (const auto& c : ch) {
                self(self, c.get());
            }
        };
        findKeyedCard(findKeyedCard, surface.RootElement());
    }
    if (!check(onlyOpenCb && reverseBtn && addBtn && taskListPanel, "Located keyed task card controls")) {
        return false;
    }
    if (!check(taskListPanel->Children().size() == 4, "Initial pageTasks size == 4")) {
        return false;
    }

    auto clickCenter = [&](UIElement* el) {
        const Rect b = el->Bounds();
        const float cx = (b.left + b.right) * 0.5f;
        const float cy = (b.top + b.bottom) * 0.5f;
        el->OnMouseDown(cx, cy);
        el->OnMouseUp(cx, cy);
    };

    // Toggle "Only Open" -> filters to open tasks [5, 7]
    const size_t evBeforeFilter = emitted.size();
    clickCenter(onlyOpenCb);
    if (!check(emitted.size() == evBeforeFilter + 1, "Only Open toggle emitted")) {
        return false;
    }
    js.DispatchEventSynchronouslyForTest(emitted.back());
    if (!check(taskListPanel->Children().size() == 2, "Filtered open pageTasks size == 2")) {
        return false;
    }

    // Reverse -> swaps existing native panels [5, 7] -> [7, 5] via InsertBefore
    UIElement* rowKey5 = taskListPanel->Children()[0].get();
    UIElement* rowKey7 = taskListPanel->Children()[1].get();
    const size_t evBeforeRev = emitted.size();
    clickCenter(reverseBtn);
    if (!check(emitted.size() == evBeforeRev + 1, "Reverse button click emitted")) {
        return false;
    }
    js.DispatchEventSynchronouslyForTest(emitted.back());
    if (!check(taskListPanel->Children().size() == 2, "Reversed pageTasks size == 2") ||
        !check(taskListPanel->Children()[0].get() == rowKey7 &&
               taskListPanel->Children()[1].get() == rowKey5,
               "Keyed reconciler reused & reordered row panels [7, 5]")) {
        return false;
    }

    // Add Item prepends open task key=8 -> [8, 7, 5]; existing row7/row5 shift to [1, 2]
    const size_t nodesBeforeAdd = reg.NodeCount();
    const size_t evBeforeAdd = emitted.size();
    clickCenter(addBtn);
    if (!check(emitted.size() == evBeforeAdd + 1, "Add Item button click emitted")) {
        return false;
    }
    js.DispatchEventSynchronouslyForTest(emitted.back());
    if (!check(taskListPanel->Children().size() == 3, "Post-add pageTasks size == 3") ||
        !check(taskListPanel->Children()[1].get() == rowKey7 &&
               taskListPanel->Children()[2].get() == rowKey5,
               "Add Item preserved keyed rows [7, 5] at indices 1, 2") ||
        !check(reg.NodeCount() > nodesBeforeAdd,
               "New keyed task row mounted in registry")) {
        return false;
    }

    // Exercise QuickJS timers cleanly and teardown with zero GC leaks
    js.StepTimersAndJobsSynchronouslyForTest(20);
    js.ShutdownSynchronouslyForTest();

    std::printf("[PASS] TestShowcaseBundleExecutionAndDiff (nodes=%zu)\n", reg.NodeCount());
    return true;
}

} // namespace

int main() {
    if (!TestEventSeqSuppressionAndRegistry()) {
        std::printf("FAILED: TestEventSeqSuppressionAndRegistry\n");
        return 1;
    }
    if (!TestShowcaseBundleExecutionAndDiff()) {
        std::printf("FAILED: TestShowcaseBundleExecutionAndDiff\n");
        return 1;
    }
    std::printf("ALL REACT UNIT TESTS PASSED\n");
    return 0;
}

