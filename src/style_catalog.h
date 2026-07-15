#pragma once

#include "style.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class IResourceProvider;

// Named ComponentStyle registry loaded from resource/styles/*.json.
// Supports:
//   { "import": ["buttons.json"], "styles": { "card": { "base": ... } } }
// Layout reference: "style": "$style.card" or style="$style.card"
class StyleCatalog {
public:
    // Load a root catalog (relative to resource provider). Processes import[] first.
    bool LoadFromResource(IResourceProvider& provider, const std::string& path);

    // Merge one styles file (relative path). Returns false on missing/invalid file.
    bool MergeFile(IResourceProvider& provider, const std::string& path);

    void Clear();
    bool Empty() const { return m_styles.empty(); }

    // name without "$style." prefix.
    std::optional<ComponentStyle> Resolve(const std::string& name) const;
    bool Has(const std::string& name) const;

    // Parse "$style.name" -> name, else empty.
    static std::string ParseStyleToken(const std::string& token);

private:
    bool MergeJsonText(IResourceProvider& provider,
                       const std::string& jsonText,
                       const std::string& sourcePath,
                       std::unordered_set<std::string>& loadingStack);

    std::unordered_map<std::string, ComponentStyle> m_styles;
};
