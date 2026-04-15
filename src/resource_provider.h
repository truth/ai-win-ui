#pragma once

#include <string>
#include <vector>

class IResourceProvider {
public:
    virtual ~IResourceProvider() = default;
    virtual bool Exists(const std::string& path) = 0;
    virtual std::vector<uint8_t> LoadBytes(const std::string& path) = 0;
    virtual std::string LoadText(const std::string& path) = 0;
};

class DirectoryResourceProvider : public IResourceProvider {
public:
    explicit DirectoryResourceProvider(std::wstring rootDir = L"resource");

    bool Exists(const std::string& path) override;
    std::vector<uint8_t> LoadBytes(const std::string& path) override;
    std::string LoadText(const std::string& path) override;

private:
    std::wstring GetFullPath(const std::string& path) const;

    std::wstring m_rootDir;
};
