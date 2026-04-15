#pragma once

#include "resource_provider.h"

#include <string>

class ZipResourceProvider : public IResourceProvider {
public:
    explicit ZipResourceProvider(std::wstring zipFilePath);
    ~ZipResourceProvider();

    bool IsValid() const;
    bool Exists(const std::string& path) override;
    std::vector<uint8_t> LoadBytes(const std::string& path) override;
    std::string LoadText(const std::string& path) override;

private:
    std::string NormalizePath(const std::string& path) const;
    std::string Utf16ToUtf8(const std::wstring& value) const;

    void* m_archive = nullptr;
    bool m_isValid = false;
};
