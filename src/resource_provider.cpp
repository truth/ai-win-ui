#include "resource_provider.h"

#include <fstream>
#include <sstream>
#include <windows.h>

DirectoryResourceProvider::DirectoryResourceProvider(std::wstring rootDir)
    : m_rootDir(std::move(rootDir)) {
    if (!m_rootDir.empty() && (m_rootDir.back() == L'/' || m_rootDir.back() == L'\\')) {
        m_rootDir.pop_back();
    }
}

std::wstring DirectoryResourceProvider::GetFullPath(const std::string& path) const {
    std::wstring fullPath = m_rootDir;
    fullPath.append(L"\\");

    std::wstring pathW;
    const int size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (size > 0) {
        pathW.resize(size - 1);
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, pathW.data(), size);
    }

    for (auto& ch : pathW) {
        if (ch == L'/') {
            ch = L'\\';
        }
    }

    fullPath.append(pathW);
    return fullPath;
}

bool DirectoryResourceProvider::Exists(const std::string& path) {
    const std::wstring fullPath = GetFullPath(path);
    DWORD attrs = GetFileAttributesW(fullPath.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::vector<uint8_t> DirectoryResourceProvider::LoadBytes(const std::string& path) {
    const std::wstring fullPath = GetFullPath(path);
    std::ifstream stream(fullPath, std::ios::binary);
    if (!stream) {
        return {};
    }

    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());
    return data;
}

std::string DirectoryResourceProvider::LoadText(const std::string& path) {
    const std::wstring fullPath = GetFullPath(path);
    std::ifstream stream(fullPath, std::ios::binary);
    if (!stream) {
        return {};
    }

    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

FallbackResourceProvider::FallbackResourceProvider(std::unique_ptr<IResourceProvider> primary,
                                                   std::unique_ptr<IResourceProvider> secondary)
    : m_primary(std::move(primary)),
      m_secondary(std::move(secondary)) {
}

bool FallbackResourceProvider::Exists(const std::string& path) {
    if (m_primary && m_primary->Exists(path)) {
        return true;
    }
    return m_secondary ? m_secondary->Exists(path) : false;
}

std::vector<uint8_t> FallbackResourceProvider::LoadBytes(const std::string& path) {
    if (m_primary && m_primary->Exists(path)) {
        auto bytes = m_primary->LoadBytes(path);
        if (!bytes.empty()) {
            return bytes;
        }
    }
    if (m_secondary) {
        return m_secondary->LoadBytes(path);
    }
    return {};
}

std::string FallbackResourceProvider::LoadText(const std::string& path) {
    if (m_primary && m_primary->Exists(path)) {
        auto text = m_primary->LoadText(path);
        if (!text.empty()) {
            return text;
        }
    }
    if (m_secondary) {
        return m_secondary->LoadText(path);
    }
    return {};
}
