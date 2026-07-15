#include "style_catalog.h"
#include "layout_parser.h"
#include "resource_provider.h"

#include <windows.h>

namespace {

std::string DirnameOf(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return {};
    }
    return path.substr(0, pos + 1);
}

std::string JoinPath(const std::string& dir, const std::string& rel) {
    if (rel.empty()) {
        return dir;
    }
    if (rel.find(':') != std::string::npos || (!rel.empty() && (rel[0] == '/' || rel[0] == '\\'))) {
        return rel;
    }
    if (dir.empty()) {
        return rel;
    }
    return dir + rel;
}

} // namespace

// Make ParseComponentStyle linkable - it's in anonymous namespace of layout_parser.
// Re-implement minimal export: layout_parser will expose a free function.
// For now we duplicate the call via LayoutParser public API - add below.

std::string StyleCatalog::ParseStyleToken(const std::string& token) {
    const std::string prefix = "$style.";
    if (token.size() > prefix.size() && token.compare(0, prefix.size(), prefix) == 0) {
        return token.substr(prefix.size());
    }
    return {};
}

void StyleCatalog::Clear() {
    m_styles.clear();
}

bool StyleCatalog::Has(const std::string& name) const {
    return m_styles.find(name) != m_styles.end();
}

std::optional<ComponentStyle> StyleCatalog::Resolve(const std::string& name) const {
    auto it = m_styles.find(name);
    if (it == m_styles.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool StyleCatalog::LoadFromResource(IResourceProvider& provider, const std::string& path) {
    std::unordered_set<std::string> stack;
    if (!provider.Exists(path)) {
        OutputDebugStringA(("[StyleCatalog] missing: " + path + "\n").c_str());
        return false;
    }
    const std::string text = provider.LoadText(path);
    return MergeJsonText(provider, text, path, stack);
}

bool StyleCatalog::MergeFile(IResourceProvider& provider, const std::string& path) {
    std::unordered_set<std::string> stack;
    if (!provider.Exists(path)) {
        return false;
    }
    return MergeJsonText(provider, provider.LoadText(path), path, stack);
}

bool StyleCatalog::MergeJsonText(IResourceProvider& provider,
                                 const std::string& jsonText,
                                 const std::string& sourcePath,
                                 std::unordered_set<std::string>& loadingStack) {
    if (loadingStack.count(sourcePath)) {
        OutputDebugStringA(("[StyleCatalog] circular import: " + sourcePath + "\n").c_str());
        return false;
    }
    loadingStack.insert(sourcePath);

    JsonValue root = LayoutParser::ParseJson(jsonText);
    if (!root.IsObject()) {
        loadingStack.erase(sourcePath);
        return false;
    }

    const std::string baseDir = DirnameOf(sourcePath);

    // import: ["buttons.json"] or "shared.json"
    if (root["import"].IsArray()) {
        for (const auto& item : root["import"].arrayValue) {
            if (!item.IsString() || item.stringValue.empty()) {
                continue;
            }
            const std::string importPath = JoinPath(baseDir, item.stringValue);
            if (provider.Exists(importPath)) {
                MergeJsonText(provider, provider.LoadText(importPath), importPath, loadingStack);
            } else if (provider.Exists(item.stringValue)) {
                MergeJsonText(provider, provider.LoadText(item.stringValue), item.stringValue, loadingStack);
            } else {
                OutputDebugStringA(("[StyleCatalog] import not found: " + item.stringValue + "\n").c_str());
            }
        }
    } else if (root["import"].IsString()) {
        const std::string importPath = JoinPath(baseDir, root["import"].stringValue);
        if (provider.Exists(importPath)) {
            MergeJsonText(provider, provider.LoadText(importPath), importPath, loadingStack);
        }
    }

    // styles: { "name": { ComponentStyle object } }
    // Two passes so "extend": "$style.other" works regardless of key order
    // (imports are already merged before this file's styles).
    const StyleCatalog* previousCatalog = LayoutParser::SetActiveStyleCatalog(this);
    const JsonValue& styles = root["styles"].IsObject() ? root["styles"] : root;
    if (styles.IsObject()) {
        auto parseAll = [&]() {
            for (const auto& [name, value] : styles.objectValue) {
                if (name == "import" || name == "styles") {
                    continue;
                }
                if (!value.IsObject()) {
                    continue;
                }
                m_styles[name] = LayoutParser::ParseComponentStylePublic(value);
            }
        };
        parseAll();
        parseAll();
    }
    LayoutParser::SetActiveStyleCatalog(previousCatalog);

    loadingStack.erase(sourcePath);
    return true;
}
