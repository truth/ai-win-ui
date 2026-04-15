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
