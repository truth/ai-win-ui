#include "text_measurer.h"
#include <list>
#include <string>
#include <unordered_map>

std::unique_ptr<ITextMeasurer> CreateDirectWriteTextMeasurer();
std::unique_ptr<ITextMeasurer> CreateSkiaTextMeasurer();

namespace {

struct CacheKey {
    std::wstring text;
    float fontSize;
    float maxWidth;
    TextWrapMode wrapMode;

    bool operator==(const CacheKey& other) const {
        return fontSize == other.fontSize &&
               maxWidth == other.maxWidth &&
               wrapMode == other.wrapMode &&
               text == other.text;
    }
};

struct CacheKeyHash {
    size_t operator()(const CacheKey& k) const {
        size_t h1 = std::hash<std::wstring>{}(k.text);
        size_t h2 = std::hash<float>{}(k.fontSize);
        size_t h3 = std::hash<float>{}(k.maxWidth);
        size_t h4 = static_cast<size_t>(k.wrapMode);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

class CachingTextMeasurer final : public ITextMeasurer {
public:
    CachingTextMeasurer(std::unique_ptr<ITextMeasurer> inner, size_t capacity)
        : m_inner(std::move(inner)), m_capacity(capacity) {}

    Size MeasureText(const wchar_t* text,
                     uint32_t len,
                     float fontSize,
                     float maxWidth,
                     TextWrapMode wrapMode) override {
        if (!text || len == 0 || fontSize <= 0.0f) {
            return Size{};
        }

        CacheKey key{std::wstring(text, len), fontSize, maxWidth, wrapMode};
        auto it = m_cacheMap.find(key);
        if (it != m_cacheMap.end()) {
            m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
            return it->second->value;
        }

        if (!m_inner) {
            return Size{};
        }

        Size measured = m_inner->MeasureText(text, len, fontSize, maxWidth, wrapMode);
        if (m_capacity > 0) {
            if (m_lruList.size() >= m_capacity) {
                auto last = m_lruList.end();
                --last;
                m_cacheMap.erase(last->key);
                m_lruList.pop_back();
            }

            m_lruList.push_front(CacheEntry{key, measured});
            m_cacheMap[key] = m_lruList.begin();
        }

        return measured;
    }

private:
    struct CacheEntry {
        CacheKey key;
        Size value;
    };

    std::unique_ptr<ITextMeasurer> m_inner;
    size_t m_capacity;
    std::list<CacheEntry> m_lruList;
    std::unordered_map<CacheKey, std::list<CacheEntry>::iterator, CacheKeyHash> m_cacheMap;
};

} // namespace

std::unique_ptr<ITextMeasurer> CreateCachedTextMeasurer(std::unique_ptr<ITextMeasurer> inner, size_t maxCapacity) {
    if (!inner) return nullptr;
    return std::make_unique<CachingTextMeasurer>(std::move(inner), maxCapacity);
}

std::unique_ptr<ITextMeasurer> CreateTextMeasurer(RendererBackend backend) {
    std::unique_ptr<ITextMeasurer> raw;
    if (backend == RendererBackend::Skia) {
        raw = CreateSkiaTextMeasurer();
    }
    if (!raw) {
        raw = CreateDirectWriteTextMeasurer();
    }
    return CreateCachedTextMeasurer(std::move(raw));
}

