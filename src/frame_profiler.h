#pragma once

// FrameProfiler: lightweight per-frame timing for the UiHost paint pipeline.
//
// Phases measured per presented frame:
//   resize  - renderer surface (re)allocation since the previous frame
//   layout  - accumulated UpdateLayout() time since the previous frame
//   render  - UI tree Render + RenderOverlay (CPU raster for Skia)
//   present - EndFrame (BitBlt / UpdateLayeredWindow / D2D EndDraw)
//   paint   - whole WM_PAINT handler (clear + render + present)
//   latency - first input/resize message since the previous frame -> present done
//
// Always collected (QPC cost is negligible). Displayed via the F12 overlay and
// optionally written per frame as CSV via AI_WIN_UI_FRAME_STATS=<path>.

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace ai_win_ui {

struct FrameSample {
    double resizeMs = 0.0;
    double layoutMs = 0.0;
    int layoutCount = 0;
    double renderMs = 0.0;
    double presentMs = 0.0;
    double paintMs = 0.0;
    double latencyMs = -1.0;  // < 0 when the frame was not triggered by input
    double intervalMs = 0.0;
    const char* trigger = "";
};

struct FramePhaseStats {
    double avg = 0.0;
    double p95 = 0.0;
    double max = 0.0;
    int count = 0;
};

class FrameProfiler {
public:
    static constexpr size_t kHistory = 120;

    static double NowMs() {
        static const double freq = [] {
            LARGE_INTEGER f{};
            QueryPerformanceFrequency(&f);
            return static_cast<double>(f.QuadPart);
        }();
        LARGE_INTEGER c{};
        QueryPerformanceCounter(&c);
        return static_cast<double>(c.QuadPart) * 1000.0 / freq;
    }

    void OpenCsv(const std::string& path) {
        if (path.empty()) {
            return;
        }
        m_csv.open(path, std::ios::binary | std::ios::trunc);
        if (m_csv) {
            m_csv << "frame,t_ms,trigger,resize_ms,layout_ms,layout_count,render_ms,present_ms,paint_ms,latency_ms,interval_ms\n";
        }
    }

    // Record the first triggering event since the last presented frame.
    void MarkInput(const char* trigger) {
        if (m_pendingInputMs < 0.0) {
            m_pendingInputMs = NowMs();
            m_pendingTrigger = trigger;
        }
    }

    void AddResize(double ms) { m_pendingResizeMs += ms; }

    void AddLayout(double ms) {
        m_pendingLayoutMs += ms;
        ++m_pendingLayoutCount;
    }

    void BeginPaint() { m_paintBegin = NowMs(); }
    void MarkRenderBegin() { m_renderBegin = NowMs(); }
    void MarkRenderEnd() { m_renderEnd = NowMs(); }

    void EndPaint() {
        const double end = NowMs();
        FrameSample s;
        s.resizeMs = m_pendingResizeMs;
        s.layoutMs = m_pendingLayoutMs;
        s.layoutCount = m_pendingLayoutCount;
        s.renderMs = std::max(0.0, m_renderEnd - m_renderBegin);
        s.presentMs = std::max(0.0, end - m_renderEnd);
        s.paintMs = std::max(0.0, end - m_paintBegin);
        s.latencyMs = m_pendingInputMs >= 0.0 ? end - m_pendingInputMs : -1.0;
        s.intervalMs = m_lastPresentMs > 0.0 ? end - m_lastPresentMs : 0.0;
        s.trigger = m_pendingInputMs >= 0.0 ? m_pendingTrigger : "invalidate";
        m_lastPresentMs = end;

        m_history[m_head] = s;
        m_head = (m_head + 1) % kHistory;
        m_size = std::min(m_size + 1, kHistory);
        ++m_frameIndex;

        if (m_csv) {
            if (m_startMs <= 0.0) {
                m_startMs = end;
            }
            char line[320];
            std::snprintf(line, sizeof(line),
                "%llu,%.3f,%s,%.3f,%.3f,%d,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                static_cast<unsigned long long>(m_frameIndex), end - m_startMs, s.trigger,
                s.resizeMs, s.layoutMs, s.layoutCount, s.renderMs, s.presentMs, s.paintMs,
                s.latencyMs, s.intervalMs);
            m_csv << line;
            m_csv.flush();
        }

        m_pendingResizeMs = 0.0;
        m_pendingLayoutMs = 0.0;
        m_pendingLayoutCount = 0;
        m_pendingInputMs = -1.0;
        m_pendingTrigger = "";
    }

    size_t Size() const { return m_size; }
    unsigned long long FrameCount() const { return m_frameIndex; }

    const FrameSample& Last() const {
        return m_history[(m_head + kHistory - 1) % kHistory];
    }

    // Field selector -> stats over the history ring (ignores negative values).
    template <typename Fn>
    FramePhaseStats Stats(Fn field) const {
        std::vector<double> values;
        values.reserve(m_size);
        for (size_t i = 0; i < m_size; ++i) {
            const double v = field(m_history[i]);
            if (v >= 0.0) {
                values.push_back(v);
            }
        }
        FramePhaseStats out;
        if (values.empty()) {
            return out;
        }
        std::sort(values.begin(), values.end());
        double sum = 0.0;
        for (double v : values) {
            sum += v;
        }
        out.count = static_cast<int>(values.size());
        out.avg = sum / values.size();
        out.max = values.back();
        out.p95 = values[std::min(values.size() - 1, static_cast<size_t>(values.size() * 0.95))];
        return out;
    }

private:
    std::array<FrameSample, kHistory> m_history{};
    size_t m_head = 0;
    size_t m_size = 0;
    unsigned long long m_frameIndex = 0;

    double m_pendingResizeMs = 0.0;
    double m_pendingLayoutMs = 0.0;
    int m_pendingLayoutCount = 0;
    double m_pendingInputMs = -1.0;
    const char* m_pendingTrigger = "";

    double m_paintBegin = 0.0;
    double m_renderBegin = 0.0;
    double m_renderEnd = 0.0;
    double m_lastPresentMs = 0.0;
    double m_startMs = 0.0;

    std::ofstream m_csv;
};

}  // namespace ai_win_ui

