#include "react_js_runtime.h"
#include "frame_profiler.h"

extern "C" {
#include "quickjs.h"
}

#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>

namespace ai_win_ui::react {

struct ReactJsRuntime::TimerEntry::Impl {
    JSValue fn = JS_UNDEFINED;
};

namespace {

ReactJsRuntime* GetRt(JSContext* ctx) {
    return static_cast<ReactJsRuntime*>(JS_GetContextOpaque(ctx));
}

JSValue JsNativeBeginBatch(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    if (auto* rt = GetRt(ctx)) rt->NativeBeginBatch();
    return JS_UNDEFINED;
}

JSValue JsNativeCreate(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_UNDEFINED;
    uint32_t nodeId = 0;
    uint32_t typeId = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 || JS_ToUint32(ctx, &typeId, argv[1]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::CreateNode;
        op.nodeId = nodeId;
        op.nodeType = static_cast<uint16_t>(typeId);
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetPropNum(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3) return JS_UNDEFINED;
    uint32_t nodeId = 0, propId = 0, ackSeq = 0;
    double numVal = 0.0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 ||
        JS_ToUint32(ctx, &propId, argv[1]) != 0 ||
        JS_ToFloat64(ctx, &numVal, argv[2]) != 0) {
        return JS_UNDEFINED;
    }
    if (argc >= 4) {
        JS_ToUint32(ctx, &ackSeq, argv[3]);
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetPropNum;
        op.nodeId = nodeId;
        op.propId = static_cast<uint16_t>(propId);
        op.numVal = numVal;
        op.ackSeq = ackSeq;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetPropBool(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3) return JS_UNDEFINED;
    uint32_t nodeId = 0, propId = 0, ackSeq = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 || JS_ToUint32(ctx, &propId, argv[1]) != 0) {
        return JS_UNDEFINED;
    }
    const int b = JS_ToBool(ctx, argv[2]);
    if (b < 0) return JS_UNDEFINED;
    if (argc >= 4) {
        JS_ToUint32(ctx, &ackSeq, argv[3]);
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetPropBool;
        op.nodeId = nodeId;
        op.propId = static_cast<uint16_t>(propId);
        op.boolVal = (b != 0);
        op.ackSeq = ackSeq;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetPropColor(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3) return JS_UNDEFINED;
    uint32_t nodeId = 0, propId = 0, argb = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 ||
        JS_ToUint32(ctx, &propId, argv[1]) != 0 ||
        JS_ToUint32(ctx, &argb, argv[2]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetPropColor;
        op.nodeId = nodeId;
        op.propId = static_cast<uint16_t>(propId);
        op.uintArg = argb;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetPropThickness(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 6) return JS_UNDEFINED;
    uint32_t nodeId = 0, propId = 0;
    double l = 0, t = 0, r = 0, b = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 ||
        JS_ToUint32(ctx, &propId, argv[1]) != 0 ||
        JS_ToFloat64(ctx, &l, argv[2]) != 0 ||
        JS_ToFloat64(ctx, &t, argv[3]) != 0 ||
        JS_ToFloat64(ctx, &r, argv[4]) != 0 ||
        JS_ToFloat64(ctx, &b, argv[5]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetPropThickness;
        op.nodeId = nodeId;
        op.propId = static_cast<uint16_t>(propId);
        op.tL = static_cast<float>(l);
        op.tT = static_cast<float>(t);
        op.tR = static_cast<float>(r);
        op.tB = static_cast<float>(b);
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetPropStr(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3) return JS_UNDEFINED;
    uint32_t nodeId = 0, propId = 0, ackSeq = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 || JS_ToUint32(ctx, &propId, argv[1]) != 0) {
        return JS_UNDEFINED;
    }
    size_t len = 0;
    const char* s = JS_ToCStringLen(ctx, &len, argv[2]);
    if (!s) return JS_UNDEFINED;
    std::string val(s, len);
    JS_FreeCString(ctx, s);
    if (argc >= 4) {
        JS_ToUint32(ctx, &ackSeq, argv[3]);
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetPropStr;
        op.nodeId = nodeId;
        op.propId = static_cast<uint16_t>(propId);
        op.strVal = std::move(val);
        op.ackSeq = ackSeq;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetEventMask(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_UNDEFINED;
    uint32_t nodeId = 0, mask = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 || JS_ToUint32(ctx, &mask, argv[1]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetEventMask;
        op.nodeId = nodeId;
        op.uintArg = mask;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeAppend(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_UNDEFINED;
    uint32_t parentId = 0, childId = 0;
    if (JS_ToUint32(ctx, &parentId, argv[0]) != 0 || JS_ToUint32(ctx, &childId, argv[1]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::AppendChild;
        op.parentId = parentId;
        op.nodeId = childId;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeInsertBefore(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3) return JS_UNDEFINED;
    uint32_t parentId = 0, childId = 0, beforeId = 0;
    if (JS_ToUint32(ctx, &parentId, argv[0]) != 0 ||
        JS_ToUint32(ctx, &childId, argv[1]) != 0 ||
        JS_ToUint32(ctx, &beforeId, argv[2]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::InsertBefore;
        op.parentId = parentId;
        op.nodeId = childId;
        op.refId = beforeId;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeRemove(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_UNDEFINED;
    uint32_t parentId = 0, childId = 0;
    if (JS_ToUint32(ctx, &parentId, argv[0]) != 0 || JS_ToUint32(ctx, &childId, argv[1]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::RemoveChild;
        op.parentId = parentId;
        op.nodeId = childId;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeDestroy(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_UNDEFINED;
    uint32_t nodeId = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::DestroyNode;
        op.nodeId = nodeId;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetRoot(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_UNDEFINED;
    uint32_t nodeId = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetRoot;
        op.nodeId = nodeId;
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeEndBatch(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    bool needsLayout = false;
    if (argc >= 1) {
        const int b = JS_ToBool(ctx, argv[0]);
        if (b > 0) needsLayout = true;
    }
    if (auto* rt = GetRt(ctx)) {
        rt->NativeEndBatch(needsLayout);
    }
    return JS_UNDEFINED;
}

JSValue JsNativeSetTitle(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_UNDEFINED;
    size_t len = 0;
    const char* s = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!s) return JS_UNDEFINED;
    std::string title(s, len);
    JS_FreeCString(ctx, s);
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::SetWindowTitle;
        op.strVal = std::move(title);
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeOpenDemo(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_UNDEFINED;
    auto readOptStr = [&](int idx) -> std::string {
        if (idx >= argc || JS_IsUndefined(argv[idx]) || JS_IsNull(argv[idx])) return {};
        size_t l = 0;
        const char* p = JS_ToCStringLen(ctx, &l, argv[idx]);
        if (!p) return {};
        std::string r(p, l);
        JS_FreeCString(ctx, p);
        return r;
    };
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::OpenDemo;
        op.strVal = readOptStr(0);
        op.strVal2 = readOptStr(1);
        op.strVal3 = readOptStr(2);
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeWindowAction(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_UNDEFINED;
    size_t len = 0;
    const char* s = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!s) return JS_UNDEFINED;
    std::string act(s, len);
    JS_FreeCString(ctx, s);
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::WindowAction;
        op.strVal = std::move(act);
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

float ReadJsPropFloat(JSContext* ctx, JSValueConst obj, const char* name, float fallback = 0.0f) {
    JSValue v = JS_GetPropertyStr(ctx, obj, name);
    double out = static_cast<double>(fallback);
    if (!JS_IsUndefined(v) && !JS_IsNull(v)) {
        JS_ToFloat64(ctx, &out, v);
    }
    JS_FreeValue(ctx, v);
    return static_cast<float>(out);
}

bool ReadJsPropBool(JSContext* ctx, JSValueConst obj, const char* name, bool fallback = false) {
    JSValue v = JS_GetPropertyStr(ctx, obj, name);
    bool out = fallback;
    if (!JS_IsUndefined(v) && !JS_IsNull(v)) {
        const int b = JS_ToBool(ctx, v);
        if (b >= 0) out = (b != 0);
    }
    JS_FreeValue(ctx, v);
    return out;
}

std::string ReadJsPropStr(JSContext* ctx, JSValueConst obj, const char* name) {
    JSValue v = JS_GetPropertyStr(ctx, obj, name);
    std::string out;
    if (!JS_IsUndefined(v) && !JS_IsNull(v)) {
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx, &len, v);
        if (s) {
            out.assign(s, len);
            JS_FreeCString(ctx, s);
        }
    }
    JS_FreeValue(ctx, v);
    return out;
}

bool ReadPointFromJs(JSContext* ctx, JSValueConst obj, ChartPointData& out) {
    if (!JS_IsObject(obj)) return false;
    out.price = ReadJsPropFloat(ctx, obj, "price");
    out.leadPrice = ReadJsPropFloat(ctx, obj, "leadPrice");
    out.avgPrice = ReadJsPropFloat(ctx, obj, "avgPrice");
    out.volume = ReadJsPropFloat(ctx, obj, "volume");
    out.indicator1 = ReadJsPropFloat(ctx, obj, "indicator1");
    out.indicator2 = ReadJsPropFloat(ctx, obj, "indicator2");
    out.isUp = ReadJsPropBool(ctx, obj, "isUp", true);
    out.timeStr = ReadJsPropStr(ctx, obj, "timeStr");
    return true;
}

bool ReadCandleFromJs(JSContext* ctx, JSValueConst obj, ChartCandleData& out) {
    if (!JS_IsObject(obj)) return false;
    out.date = ReadJsPropStr(ctx, obj, "date");
    out.open = ReadJsPropFloat(ctx, obj, "open");
    out.high = ReadJsPropFloat(ctx, obj, "high");
    out.low = ReadJsPropFloat(ctx, obj, "low");
    out.close = ReadJsPropFloat(ctx, obj, "close");
    out.volume = ReadJsPropFloat(ctx, obj, "volume");
    out.ma5 = ReadJsPropFloat(ctx, obj, "ma5");
    out.ma10 = ReadJsPropFloat(ctx, obj, "ma10");
    out.ma20 = ReadJsPropFloat(ctx, obj, "ma20");
    return true;
}

JSValue JsNativeChartSetPoints(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_UNDEFINED;
    uint32_t nodeId = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 || !JS_IsObject(argv[1])) {
        return JS_UNDEFINED;
    }
    JSValue lenVal = JS_GetPropertyStr(ctx, argv[1], "length");
    uint32_t len = 0;
    JS_ToUint32(ctx, &len, lenVal);
    JS_FreeValue(ctx, lenVal);
    const uint32_t count = std::min<uint32_t>(len, 100000u);

    MutationOp op;
    op.type = MutationOpType::ChartSetPoints;
    op.nodeId = nodeId;
    op.chartPoints.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, argv[1], i);
        ChartPointData pt;
        if (ReadPointFromJs(ctx, item, pt)) {
            op.chartPoints.push_back(std::move(pt));
        }
        JS_FreeValue(ctx, item);
    }
    if (auto* rt = GetRt(ctx)) {
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeChartAppendPoint(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_UNDEFINED;
    uint32_t nodeId = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 || !JS_IsObject(argv[1])) {
        return JS_UNDEFINED;
    }
    bool replaceLast = false;
    if (argc >= 3) {
        const int b = JS_ToBool(ctx, argv[2]);
        if (b > 0) replaceLast = true;
    }
    ChartPointData pt;
    if (!ReadPointFromJs(ctx, argv[1], pt)) {
        return JS_UNDEFINED;
    }
    if (auto* rt = GetRt(ctx)) {
        MutationOp op;
        op.type = MutationOpType::ChartAppendPoint;
        op.nodeId = nodeId;
        op.boolVal = replaceLast;
        op.chartPoints.push_back(std::move(pt));
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeChartSetCandles(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_UNDEFINED;
    uint32_t nodeId = 0;
    if (JS_ToUint32(ctx, &nodeId, argv[0]) != 0 || !JS_IsObject(argv[1])) {
        return JS_UNDEFINED;
    }
    JSValue lenVal = JS_GetPropertyStr(ctx, argv[1], "length");
    uint32_t len = 0;
    JS_ToUint32(ctx, &len, lenVal);
    JS_FreeValue(ctx, lenVal);
    const uint32_t count = std::min<uint32_t>(len, 100000u);

    MutationOp op;
    op.type = MutationOpType::ChartSetCandles;
    op.nodeId = nodeId;
    op.chartCandles.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, argv[1], i);
        ChartCandleData bar;
        if (ReadCandleFromJs(ctx, item, bar)) {
            op.chartCandles.push_back(std::move(bar));
        }
        JS_FreeValue(ctx, item);
    }
    if (auto* rt = GetRt(ctx)) {
        rt->NativePushOp(std::move(op));
    }
    return JS_UNDEFINED;
}

JSValue JsNativeLog(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    std::ostringstream oss;
    oss << "[ReactJS]";
    for (int i = 0; i < argc; ++i) {
        size_t len = 0;
        const char* s = JS_ToCStringLen(ctx, &len, argv[i]);
        if (s) {
            oss << " " << std::string(s, len);
            JS_FreeCString(ctx, s);
        }
    }
    oss << "\n";
    const std::string line = oss.str();
    OutputDebugStringA(line.c_str());
    return JS_UNDEFINED;
}

JSValue JsSetTimeout(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_NewInt32(ctx, 0);
    }
    double delay = 0.0;
    if (argc >= 2 && !JS_IsUndefined(argv[1])) {
        JS_ToFloat64(ctx, &delay, argv[1]);
    }
    if (auto* rt = GetRt(ctx)) {
        const uint32_t id = rt->AddTimerFromJs(ctx, const_cast<void*>(reinterpret_cast<const void*>(&argv[0])), static_cast<int64_t>(std::max(0.0, delay)), false);
        return JS_NewInt32(ctx, static_cast<int32_t>(id));
    }
    return JS_NewInt32(ctx, 0);
}

JSValue JsSetInterval(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_NewInt32(ctx, 0);
    }
    double delay = 16.0;
    if (argc >= 2 && !JS_IsUndefined(argv[1])) {
        JS_ToFloat64(ctx, &delay, argv[1]);
    }
    if (auto* rt = GetRt(ctx)) {
        const uint32_t id = rt->AddTimerFromJs(ctx, const_cast<void*>(reinterpret_cast<const void*>(&argv[0])), static_cast<int64_t>(std::max(1.0, delay)), true);
        return JS_NewInt32(ctx, static_cast<int32_t>(id));
    }
    return JS_NewInt32(ctx, 0);
}

JSValue JsRequestAnimationFrame(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_NewInt32(ctx, 0);
    }
    if (auto* rt = GetRt(ctx)) {
        const uint32_t id = rt->AddTimerFromJs(ctx, const_cast<void*>(reinterpret_cast<const void*>(&argv[0])), 16, false);
        return JS_NewInt32(ctx, static_cast<int32_t>(id));
    }
    return JS_NewInt32(ctx, 0);
}

JSValue JsClearTimer(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_UNDEFINED;
    uint32_t id = 0;
    if (JS_ToUint32(ctx, &id, argv[0]) == 0) {
        if (auto* rt = GetRt(ctx)) {
            rt->CancelTimerFromJs(id);
        }
    }
    return JS_UNDEFINED;
}

JSValue JsPerformanceNow(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    if (auto* rt = GetRt(ctx)) {
        return JS_NewFloat64(ctx, static_cast<double>(rt->CurrentClockMs()));
    }
    return JS_NewFloat64(ctx, 0.0);
}

JSValue JsQueueMicrotask(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) return JS_UNDEFINED;
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue promiseCtor = JS_GetPropertyStr(ctx, global, "Promise");
    JSValue resolveFn = JS_GetPropertyStr(ctx, promiseCtor, "resolve");
    JSValue resolved = JS_Call(ctx, resolveFn, promiseCtor, 0, nullptr);
    JSValue thenFn = JS_GetPropertyStr(ctx, resolved, "then");
    JSValue argvThen[1] = { argv[0] };
    JSValue res = JS_Call(ctx, thenFn, resolved, 1, argvThen);
    JS_FreeValue(ctx, res);
    JS_FreeValue(ctx, thenFn);
    JS_FreeValue(ctx, resolved);
    JS_FreeValue(ctx, resolveFn);
    JS_FreeValue(ctx, promiseCtor);
    JS_FreeValue(ctx, global);
    return JS_UNDEFINED;
}

} // namespace

ReactJsRuntime::ReactJsRuntime(CommitSink commitSink)
    : m_commitSink(std::move(commitSink)) {}

ReactJsRuntime::~ReactJsRuntime() {
    Stop();
    DestroyQuickJs();
}

void ReactJsRuntime::Start(std::string initialBundleUtf8, std::string sourceName) {
    if (m_started) return;
    m_started = true;
    m_stopRequested = false;
    m_workerThread = std::thread(
        &ReactJsRuntime::WorkerMain,
        this,
        std::move(initialBundleUtf8),
        std::move(sourceName));
}

void ReactJsRuntime::Stop() {
    if (!m_started) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopRequested = true;
    }
    m_cv.notify_all();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_started = false;
}

void ReactJsRuntime::PostUiEvent(UiEvent&& ev) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stopRequested) return;
        m_eventQueue.push_back(std::move(ev));
    }
    m_cv.notify_one();
}

void ReactJsRuntime::RequestReload(std::string newBundleUtf8, std::string sourceName) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stopRequested) return;
        m_hasPendingReload = true;
        m_reloadBundle = std::move(newBundleUtf8);
        m_reloadSourceName = std::move(sourceName);
    }
    m_cv.notify_one();
}

const char* EventTypeToTrigger(EventTypeId type) {
    switch (type) {
        case EventTypeId::Click: return "react_click";
        case EventTypeId::KeyDown: return "react_key";
        case EventTypeId::TextChanged:
        case EventTypeId::Submit:
        case EventTypeId::Change:
        case EventTypeId::PageChange:
        case EventTypeId::TreeSelect:
        case EventTypeId::TreeCheck:
            return "react_input";
        default:
            return "react_event";
    }
}

bool ReactJsRuntime::InitSynchronouslyForTest() {
    return InitQuickJs();
}

bool ReactJsRuntime::EvalSynchronouslyForTest(const std::string& scriptUtf8, const std::string& sourceName) {
    m_activeOriginEventMs = 0.0;
    m_activeTrigger = "react_init";
    m_activeJsStartMs = ai_win_ui::FrameProfiler::NowMs();
    const bool ok = EvalScript(scriptUtf8, sourceName);
    DrainMicrotasks();
    m_activeJsStartMs = 0.0;
    m_activeTrigger = "";
    return ok;
}

bool ReactJsRuntime::DispatchEventSynchronouslyForTest(const UiEvent& ev) {
    m_activeOriginEventMs = ev.timestampMs;
    m_activeTrigger = EventTypeToTrigger(ev.type);
    m_activeJsStartMs = ai_win_ui::FrameProfiler::NowMs();
    DispatchEventToJs(ev);
    DrainMicrotasks();
    m_activeOriginEventMs = 0.0;
    m_activeJsStartMs = 0.0;
    m_activeTrigger = "";
    return true;
}

bool ReactJsRuntime::StepTimersAndJobsSynchronouslyForTest(int64_t advanceMs) {
    m_testClockOffsetMs += std::max<int64_t>(0, advanceMs);
    m_activeOriginEventMs = 0.0;
    m_activeTrigger = "react_timer";
    m_activeJsStartMs = ai_win_ui::FrameProfiler::NowMs();
    FireDueTimers();
    DrainMicrotasks();
    m_activeJsStartMs = 0.0;
    m_activeTrigger = "";
    return true;
}

void ReactJsRuntime::ShutdownSynchronouslyForTest() {
    DestroyQuickJs();
}

void ReactJsRuntime::NativeBeginBatch() {
    m_pendingBatch.ops.clear();
    m_pendingBatch.needsLayout = false;
    m_pendingBatch.originEventMs = 0.0;
    m_pendingBatch.jsExecMs = 0.0;
    m_pendingBatch.commitTrigger = "";
    m_inBatch = true;
}

void ReactJsRuntime::NativePushOp(MutationOp&& op) {
    if (!m_inBatch) {
        MutationBatch single;
        if (op.type == MutationOpType::SetRoot) {
            single.needsLayout = true;
        }
        single.originEventMs = m_activeOriginEventMs;
        single.commitTrigger = m_activeTrigger;
        if (m_activeJsStartMs > 0.0) {
            single.jsExecMs = std::max(0.0, ai_win_ui::FrameProfiler::NowMs() - m_activeJsStartMs);
        }
        m_activeOriginEventMs = 0.0;
        single.ops.push_back(std::move(op));
        if (m_commitSink) m_commitSink(std::move(single));
        return;
    }
    m_pendingBatch.ops.push_back(std::move(op));
}

void ReactJsRuntime::NativeEndBatch(bool needsLayout) {
    if (!m_inBatch) return;
    m_inBatch = false;
    m_pendingBatch.needsLayout = m_pendingBatch.needsLayout || needsLayout;
    m_pendingBatch.originEventMs = m_activeOriginEventMs;
    m_pendingBatch.commitTrigger = m_activeTrigger;
    if (m_activeJsStartMs > 0.0) {
        m_pendingBatch.jsExecMs = std::max(0.0, ai_win_ui::FrameProfiler::NowMs() - m_activeJsStartMs);
    }
    m_activeOriginEventMs = 0.0;
    if (!m_pendingBatch.ops.empty() && m_commitSink) {
        m_commitSink(std::move(m_pendingBatch));
    }
    m_pendingBatch.ops.clear();
    m_pendingBatch.needsLayout = false;
    m_pendingBatch.originEventMs = 0.0;
    m_pendingBatch.jsExecMs = 0.0;
    m_pendingBatch.commitTrigger = "";
}

uint32_t ReactJsRuntime::AddTimerFromJs(JSContext* ctx, void* jsFuncPtr, int64_t delayMs, bool isInterval) {
    const JSValueConst* fnPtr = reinterpret_cast<const JSValueConst*>(jsFuncPtr);
    const uint32_t id = m_nextTimerId++;
    TimerEntry entry;
    entry.id = id;
    entry.dueMs = NowSteadyMs() + delayMs;
    entry.intervalMs = delayMs;
    entry.isInterval = isInterval;
    entry.impl = new TimerEntry::Impl();
    entry.impl->fn = JS_DupValue(ctx, *fnPtr);
    m_timers.push_back(entry);
    return id;
}

void ReactJsRuntime::CancelTimerFromJs(uint32_t timerId) {
    for (auto& t : m_timers) {
        if (t.id == timerId) {
            t.cancelled = true;
            if (t.impl && m_ctx && !JS_IsUndefined(t.impl->fn)) {
                JS_FreeValue(m_ctx, t.impl->fn);
                t.impl->fn = JS_UNDEFINED;
            }
            break;
        }
    }
}

int64_t ReactJsRuntime::NowSteadyMs() const {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count() + m_testClockOffsetMs;
}

bool ReactJsRuntime::InitQuickJs() {
    DestroyQuickJs();
    m_rt = JS_NewRuntime();
    if (!m_rt) return false;
    // 2MB JS recursion limit (matches Windows 8MB thread stack safely)
    JS_SetMaxStackSize(m_rt, 2 * 1024 * 1024);

    m_ctx = JS_NewContext(m_rt);
    if (!m_ctx) {
        JS_FreeRuntime(m_rt);
        m_rt = nullptr;
        return false;
    }
    JS_SetContextOpaque(m_ctx, this);
    RegisterNativeBindings();
    RegisterTimersAndGlobals();
    return true;
}

void ReactJsRuntime::DestroyQuickJs() {
    if (m_ctx) {
        for (auto& t : m_timers) {
            if (t.impl) {
                if (!JS_IsUndefined(t.impl->fn)) {
                    JS_FreeValue(m_ctx, t.impl->fn);
                    t.impl->fn = JS_UNDEFINED;
                }
                delete t.impl;
                t.impl = nullptr;
            }
        }
        m_timers.clear();
        m_inBatch = false;
        m_pendingBatch.ops.clear();
        JS_FreeContext(m_ctx);
        m_ctx = nullptr;
    } else {
        for (auto& t : m_timers) {
            delete t.impl;
            t.impl = nullptr;
        }
        m_timers.clear();
    }
    if (m_rt) {
        JS_FreeRuntime(m_rt);
        m_rt = nullptr;
    }
}

void ReactJsRuntime::RegisterNativeBindings() {
    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue nativeObj = JS_NewObject(m_ctx);

    auto bindFn = [&](const char* name, JSCFunction* fn, int argc) {
        JS_SetPropertyStr(m_ctx, nativeObj, name, JS_NewCFunction(m_ctx, fn, name, argc));
    };

    bindFn("beginBatch", &JsNativeBeginBatch, 0);
    bindFn("create", &JsNativeCreate, 2);
    bindFn("setPropNum", &JsNativeSetPropNum, 4);
    bindFn("setPropBool", &JsNativeSetPropBool, 4);
    bindFn("setPropColor", &JsNativeSetPropColor, 3);
    bindFn("setPropThickness", &JsNativeSetPropThickness, 6);
    bindFn("setPropStr", &JsNativeSetPropStr, 4);
    bindFn("setEventMask", &JsNativeSetEventMask, 2);
    bindFn("append", &JsNativeAppend, 2);
    bindFn("insertBefore", &JsNativeInsertBefore, 3);
    bindFn("remove", &JsNativeRemove, 2);
    bindFn("destroy", &JsNativeDestroy, 1);
    bindFn("setRoot", &JsNativeSetRoot, 1);
    bindFn("endBatch", &JsNativeEndBatch, 1);
    bindFn("setTitle", &JsNativeSetTitle, 1);
    bindFn("openDemo", &JsNativeOpenDemo, 3);
    bindFn("windowAction", &JsNativeWindowAction, 1);
    bindFn("chartSetPoints", &JsNativeChartSetPoints, 2);
    bindFn("chartAppendPoint", &JsNativeChartAppendPoint, 3);
    bindFn("chartSetCandles", &JsNativeChartSetCandles, 2);
    bindFn("log", &JsNativeLog, 2);

    JS_SetPropertyStr(m_ctx, global, "__aiNative", nativeObj);
    JS_FreeValue(m_ctx, global);
}

void ReactJsRuntime::RegisterTimersAndGlobals() {
    JSValue global = JS_GetGlobalObject(m_ctx);

    JS_SetPropertyStr(m_ctx, global, "setTimeout", JS_NewCFunction(m_ctx, &JsSetTimeout, "setTimeout", 2));
    JS_SetPropertyStr(m_ctx, global, "clearTimeout", JS_NewCFunction(m_ctx, &JsClearTimer, "clearTimeout", 1));
    JS_SetPropertyStr(m_ctx, global, "setInterval", JS_NewCFunction(m_ctx, &JsSetInterval, "setInterval", 2));
    JS_SetPropertyStr(m_ctx, global, "clearInterval", JS_NewCFunction(m_ctx, &JsClearTimer, "clearInterval", 1));
    JS_SetPropertyStr(m_ctx, global, "requestAnimationFrame", JS_NewCFunction(m_ctx, &JsRequestAnimationFrame, "requestAnimationFrame", 1));
    JS_SetPropertyStr(m_ctx, global, "cancelAnimationFrame", JS_NewCFunction(m_ctx, &JsClearTimer, "cancelAnimationFrame", 1));
    JS_SetPropertyStr(m_ctx, global, "queueMicrotask", JS_NewCFunction(m_ctx, &JsQueueMicrotask, "queueMicrotask", 1));

    JSValue consoleObj = JS_NewObject(m_ctx);
    JS_SetPropertyStr(m_ctx, consoleObj, "log", JS_NewCFunction(m_ctx, &JsNativeLog, "log", 1));
    JS_SetPropertyStr(m_ctx, consoleObj, "warn", JS_NewCFunction(m_ctx, &JsNativeLog, "warn", 1));
    JS_SetPropertyStr(m_ctx, consoleObj, "error", JS_NewCFunction(m_ctx, &JsNativeLog, "error", 1));
    JS_SetPropertyStr(m_ctx, consoleObj, "info", JS_NewCFunction(m_ctx, &JsNativeLog, "info", 1));
    JS_SetPropertyStr(m_ctx, global, "console", consoleObj);

    JSValue perfObj = JS_NewObject(m_ctx);
    JS_SetPropertyStr(m_ctx, perfObj, "now", JS_NewCFunction(m_ctx, &JsPerformanceNow, "now", 0));
    JS_SetPropertyStr(m_ctx, global, "performance", perfObj);

    JS_FreeValue(m_ctx, global);
}

void ReactJsRuntime::LogError(JSContext* ctx, const char* prefix) {
    JSValue exc = JS_GetException(ctx);
    const char* msg = JS_ToCString(ctx, exc);
    std::string full = std::string("[ReactJS ERROR] ") + prefix + ": " + (msg ? msg : "(unknown)");
    if (msg) JS_FreeCString(ctx, msg);

    JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
    if (!JS_IsUndefined(stack)) {
        const char* stackStr = JS_ToCString(ctx, stack);
        if (stackStr) {
            full += "\n";
            full += stackStr;
            JS_FreeCString(ctx, stackStr);
        }
    }
    JS_FreeValue(ctx, stack);
    JS_FreeValue(ctx, exc);
    full += "\n";
    OutputDebugStringA(full.c_str());
    std::fprintf(stderr, "%s", full.c_str());
}

bool ReactJsRuntime::EvalScript(const std::string& code, const std::string& filename) {
    if (!m_ctx) return false;
    JSValue res = JS_Eval(m_ctx, code.c_str(), code.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(res)) {
        LogError(m_ctx, filename.c_str());
        JS_FreeValue(m_ctx, res);
        return false;
    }
    JS_FreeValue(m_ctx, res);
    return true;
}

void ReactJsRuntime::DrainMicrotasks() {
    if (!m_rt) return;
    JSContext* jobCtx = nullptr;
    int guard = 0;
    while (JS_IsJobPending(m_rt) && guard++ < 4096) {
        const int err = JS_ExecutePendingJob(m_rt, &jobCtx);
        if (err < 0) {
            if (jobCtx) {
                LogError(jobCtx, "Promise microtask");
            }
            break;
        }
    }
}

void ReactJsRuntime::DispatchEventToJs(const UiEvent& ev) {
    if (!m_ctx) return;
    JSValue global = JS_GetGlobalObject(m_ctx);
    JSValue dispatchFn = JS_GetPropertyStr(m_ctx, global, "__dispatchEvent");
    if (JS_IsFunction(m_ctx, dispatchFn)) {
        JSValue payload = JS_NewObject(m_ctx);
        JS_SetPropertyStr(m_ctx, payload, "bool", JS_NewBool(m_ctx, ev.boolVal ? 1 : 0));
        JS_SetPropertyStr(m_ctx, payload, "int", JS_NewInt32(m_ctx, ev.intVal));
        JS_SetPropertyStr(m_ctx, payload, "int2", JS_NewInt32(m_ctx, ev.intVal2));
        JS_SetPropertyStr(m_ctx, payload, "num", JS_NewFloat64(m_ctx, ev.numVal));
        JS_SetPropertyStr(m_ctx, payload, "str", JS_NewStringLen(m_ctx, ev.strVal.data(), ev.strVal.size()));

        JSValue args[4] = {
            JS_NewInt32(m_ctx, static_cast<int32_t>(ev.nodeId)),
            JS_NewInt32(m_ctx, static_cast<int32_t>(ev.type)),
            JS_NewInt32(m_ctx, static_cast<int32_t>(ev.eventSeq)),
            payload
        };
        JSValue ret = JS_Call(m_ctx, dispatchFn, global, 4, args);
        if (JS_IsException(ret)) {
            LogError(m_ctx, "__dispatchEvent");
        }
        JS_FreeValue(m_ctx, ret);
        for (int i = 0; i < 4; ++i) {
            JS_FreeValue(m_ctx, args[i]);
        }
    }
    JS_FreeValue(m_ctx, dispatchFn);
    JS_FreeValue(m_ctx, global);
}

void ReactJsRuntime::FireDueTimers() {
    if (!m_ctx || m_timers.empty()) return;
    const int64_t now = NowSteadyMs();

    // Snapshot ids that are due right now (timers added inside callbacks run next tick).
    std::vector<size_t> dueIndices;
    dueIndices.reserve(m_timers.size());
    for (size_t i = 0; i < m_timers.size(); ++i) {
        if (!m_timers[i].cancelled && m_timers[i].dueMs <= now) {
            dueIndices.push_back(i);
        }
    }

    for (size_t idx : dueIndices) {
        if (idx >= m_timers.size()) continue;
        TimerEntry& t = m_timers[idx];
        if (t.cancelled || !t.impl || JS_IsUndefined(t.impl->fn)) continue;

        JSValue fnDup = JS_DupValue(m_ctx, t.impl->fn);
        if (!t.isInterval) {
            t.cancelled = true;
            JS_FreeValue(m_ctx, t.impl->fn);
            t.impl->fn = JS_UNDEFINED;
        } else {
            t.dueMs = now + std::max<int64_t>(1, t.intervalMs);
        }

        JSValue arg = JS_NewFloat64(m_ctx, static_cast<double>(now));
        JSValue global = JS_GetGlobalObject(m_ctx);
        JSValue ret = JS_Call(m_ctx, fnDup, global, 1, &arg);
        if (JS_IsException(ret)) {
            LogError(m_ctx, "Timer callback");
        }
        JS_FreeValue(m_ctx, ret);
        JS_FreeValue(m_ctx, global);
        JS_FreeValue(m_ctx, arg);
        JS_FreeValue(m_ctx, fnDup);
    }

    // Compact cancelled timers
    for (size_t i = m_timers.size(); i > 0; --i) {
        size_t idx = i - 1;
        if (m_timers[idx].cancelled) {
            if (m_timers[idx].impl) {
                if (!JS_IsUndefined(m_timers[idx].impl->fn)) {
                    JS_FreeValue(m_ctx, m_timers[idx].impl->fn);
                    m_timers[idx].impl->fn = JS_UNDEFINED;
                }
                delete m_timers[idx].impl;
                m_timers[idx].impl = nullptr;
            }
            m_timers.erase(m_timers.begin() + static_cast<ptrdiff_t>(idx));
        }
    }
}

int64_t ReactJsRuntime::NextTimerDelayMs() const {
    if (m_timers.empty()) return 50;
    const int64_t now = NowSteadyMs();
    int64_t minDelay = 50;
    for (const auto& t : m_timers) {
        if (t.cancelled) continue;
        const int64_t d = std::max<int64_t>(0, t.dueMs - now);
        minDelay = std::min(minDelay, d);
    }
    return minDelay;
}

void ReactJsRuntime::WorkerMain(std::string initialBundle, std::string sourceName) {
    if (InitQuickJs()) {
        m_activeOriginEventMs = 0.0;
        m_activeTrigger = "react_init";
        m_activeJsStartMs = ai_win_ui::FrameProfiler::NowMs();
        EvalScript(initialBundle, sourceName);
        DrainMicrotasks();
        m_activeJsStartMs = 0.0;
        m_activeTrigger = "";
    }

    while (true) {
        std::vector<UiEvent> pendingEvents;
        bool doReload = false;
        std::string reloadCode;
        std::string reloadName;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_stopRequested) {
                break;
            }
            if (m_eventQueue.empty() && !m_hasPendingReload) {
                const int64_t waitMs = NextTimerDelayMs();
                m_cv.wait_for(lock, std::chrono::milliseconds(waitMs), [&]() {
                    return m_stopRequested || !m_eventQueue.empty() || m_hasPendingReload;
                });
            }
            if (m_stopRequested) {
                break;
            }
            if (!m_eventQueue.empty()) {
                pendingEvents.swap(m_eventQueue);
            }
            if (m_hasPendingReload) {
                doReload = true;
                m_hasPendingReload = false;
                reloadCode = std::move(m_reloadBundle);
                reloadName = std::move(m_reloadSourceName);
            }
        }

        if (doReload) {
            MutationBatch resetBatch;
            MutationOp op;
            op.type = MutationOpType::ResetTree;
            resetBatch.needsLayout = true;
            resetBatch.ops.push_back(std::move(op));
            if (m_commitSink) {
                m_commitSink(std::move(resetBatch));
            }
            if (InitQuickJs()) {
                m_activeOriginEventMs = 0.0;
                m_activeTrigger = "react_init";
                m_activeJsStartMs = ai_win_ui::FrameProfiler::NowMs();
                EvalScript(reloadCode, reloadName);
                DrainMicrotasks();
                m_activeJsStartMs = 0.0;
                m_activeTrigger = "";
            }
            continue;
        }

        for (const auto& ev : pendingEvents) {
            m_activeOriginEventMs = ev.timestampMs;
            m_activeTrigger = EventTypeToTrigger(ev.type);
            m_activeJsStartMs = ai_win_ui::FrameProfiler::NowMs();
            DispatchEventToJs(ev);
            DrainMicrotasks();
            m_activeOriginEventMs = 0.0;
            m_activeJsStartMs = 0.0;
            m_activeTrigger = "";
        }

        m_activeOriginEventMs = 0.0;
        m_activeTrigger = "react_timer";
        m_activeJsStartMs = ai_win_ui::FrameProfiler::NowMs();
        FireDueTimers();
        DrainMicrotasks();
        m_activeJsStartMs = 0.0;
        m_activeTrigger = "";
    }

    DestroyQuickJs();
}

} // namespace ai_win_ui::react

