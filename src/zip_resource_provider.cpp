#include "zip_resource_provider.h"

#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#define MINIZ_HEADER_FILE_ONLY
#include <stdint.h>

// Minimal miniz subset for ZIP reading.
// Source: public-domain miniz library subset.

#ifndef MINIZ_HEADER_INCLUDED
#define MINIZ_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int mz_uint;
typedef unsigned long long mz_uint64;

typedef struct mz_zip_archive_tag mz_zip_archive;

typedef struct mz_zip_archive_file_stat_tag {
    mz_uint64 m_file_index;
    mz_uint64 m_uncomp_size;
    mz_uint64 m_comp_size;
    mz_uint m_method;
    mz_uint m_bit_flag;
    mz_uint m_file_name_size;
    mz_uint m_extra_field_size;
    mz_uint m_file_comment_size;
    mz_uint m_adler32;
    mz_uint m_crc32;
    mz_uint64 m_local_header_ofs;
    char m_filename[260];
} mz_zip_archive_file_stat;

struct mz_zip_archive_tag {
    void* m_pState;
};

mz_uint64 mz_zip_reader_init_file(mz_zip_archive* pArchive, const char* pFilename, mz_uint flags);
void mz_zip_reader_end(mz_zip_archive* pArchive);
int mz_zip_reader_get_num_files(mz_zip_archive* pArchive);
int mz_zip_reader_file_stat(mz_zip_archive* pArchive, int file_index, mz_zip_archive_file_stat* pStat);
int mz_zip_reader_locate_file(mz_zip_archive* pArchive, const char* pName, const char* pComment, int flags);
void* mz_zip_reader_extract_file_to_heap(mz_zip_archive* pArchive, int file_index, size_t* pSize, int flags);

#ifdef __cplusplus
}
#endif
#endif // MINIZ_HEADER_INCLUDED

// Implementation section.

#include <fstream>
#include <ostream>
#include <sstream>

// Forward declarations for internal ZIP parsing.

namespace {

struct ZipFileEntry {
    std::string filename;
    uint32_t crc32;
    uint32_t comp_size;
    uint32_t uncomp_size;
    uint16_t method;
    uint32_t local_header_offset;
};

struct ZipState {
    std::vector<uint8_t> data;
    std::vector<ZipFileEntry> entries;
};

static bool ReadAllBytes(const std::wstring& path, std::vector<uint8_t>& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    if (size <= 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(out.data()), size);
    return file.good();
}

static uint32_t ReadLE32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}

static uint16_t ReadLE16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static bool ParseCentralDirectory(ZipState& state) {
    const size_t size = state.data.size();
    if (size < 22) {
        return false;
    }

    const uint32_t kEndOfCentralDir = 0x06054b50;
    size_t pos = size - 22;
    while (pos != SIZE_MAX) {
        if (ReadLE32(&state.data[pos]) == kEndOfCentralDir) {
            break;
        }
        if (pos == 0) {
            break;
        }
        pos--;
    }
    if (pos == SIZE_MAX || pos + 22 > size) {
        return false;
    }

    const uint8_t* eocd = &state.data[pos];
    uint16_t total_entries = ReadLE16(eocd + 10);
    uint16_t cd_size = ReadLE16(eocd + 12);
    uint32_t cd_offset = ReadLE32(eocd + 16);
    if (cd_offset + cd_size > size) {
        return false;
    }

    size_t entryPos = cd_offset;
    const uint32_t kCentralDirSig = 0x02014b50;
    for (uint16_t i = 0; i < total_entries; ++i) {
        if (entryPos + 46 > size) {
            return false;
        }
        const uint8_t* header = &state.data[entryPos];
        if (ReadLE32(header) != kCentralDirSig) {
            return false;
        }
        uint16_t fileNameLen = ReadLE16(header + 28);
        uint16_t extraLen = ReadLE16(header + 30);
        uint16_t commentLen = ReadLE16(header + 32);
        uint16_t method = ReadLE16(header + 10);
        uint32_t crc32 = ReadLE32(header + 16);
        uint32_t comp_size = ReadLE32(header + 20);
        uint32_t uncomp_size = ReadLE32(header + 24);
        uint32_t local_header_offset = ReadLE32(header + 42);
        if (entryPos + 46 + fileNameLen + extraLen + commentLen > size) {
            return false;
        }
        std::string filename(reinterpret_cast<const char*>(header + 46), fileNameLen);
        ZipFileEntry entry;
        entry.filename = std::move(filename);
        entry.crc32 = crc32;
        entry.comp_size = comp_size;
        entry.uncomp_size = uncomp_size;
        entry.method = method;
        entry.local_header_offset = local_header_offset;
        state.entries.push_back(std::move(entry));
        entryPos += 46 + fileNameLen + extraLen + commentLen;
    }
    return true;
}

static void* mz_malloc(size_t size) {
    return malloc(size);
}

static void mz_free(void* p) {
    free(p);
}

static bool InflateData(const uint8_t* src, size_t src_len, void* dst, size_t dst_len) {
    // Use Windows built-in zlib via uncompress? We do not have zlib, so fallback is unsupported.
    return false;
}

static void* ExtractFileToHeap(ZipState* state, int fileIndex, size_t* pSize) {
    if (!state || fileIndex < 0 || static_cast<size_t>(fileIndex) >= state->entries.size()) {
        return nullptr;
    }
    const ZipFileEntry& entry = state->entries[fileIndex];
    if (entry.local_header_offset + 30 > state->data.size()) {
        return nullptr;
    }
    const uint8_t* header = state->data.data() + entry.local_header_offset;
    if (ReadLE32(header) != 0x04034b50) {
        return nullptr;
    }
    uint16_t fileNameLen = ReadLE16(header + 26);
    uint16_t extraLen = ReadLE16(header + 28);
    size_t dataOffset = entry.local_header_offset + 30 + fileNameLen + extraLen;
    if (dataOffset + entry.comp_size > state->data.size()) {
        return nullptr;
    }
    if (entry.method == 0) {
        void* out = mz_malloc(entry.comp_size);
        if (!out) return nullptr;
        memcpy(out, state->data.data() + dataOffset, entry.comp_size);
        if (pSize) *pSize = entry.comp_size;
        return out;
    }

    // Current implementation only supports ZIP stored entries (method 0).
    // Compressed entries (e.g. deflate) are not yet supported.
    return nullptr;
}

} // namespace

extern "C" {

mz_uint64 mz_zip_reader_init_file(mz_zip_archive* pArchive, const char* pFilename, mz_uint flags) {
    if (!pArchive || !pFilename) {
        return 0;
    }
    std::wstring filePath;
    const int size = MultiByteToWideChar(CP_UTF8, 0, pFilename, -1, nullptr, 0);
    if (size > 0) {
        filePath.resize(size - 1);
        MultiByteToWideChar(CP_UTF8, 0, pFilename, -1, filePath.data(), size);
    }
    ZipState* state = new ZipState();
    if (!ReadAllBytes(filePath, state->data)) {
        delete state;
        return 0;
    }
    if (!ParseCentralDirectory(*state)) {
        delete state;
        return 0;
    }
    pArchive->m_pState = state;
    return state->entries.size();
}

void mz_zip_reader_end(mz_zip_archive* pArchive) {
    if (!pArchive || !pArchive->m_pState) {
        return;
    }
    delete static_cast<ZipState*>(pArchive->m_pState);
    pArchive->m_pState = nullptr;
}

int mz_zip_reader_get_num_files(mz_zip_archive* pArchive) {
    if (!pArchive || !pArchive->m_pState) {
        return 0;
    }
    ZipState* state = static_cast<ZipState*>(pArchive->m_pState);
    return static_cast<int>(state->entries.size());
}

int mz_zip_reader_file_stat(mz_zip_archive* pArchive, int file_index, mz_zip_archive_file_stat* pStat) {
    if (!pArchive || !pArchive->m_pState || !pStat) {
        return 0;
    }
    ZipState* state = static_cast<ZipState*>(pArchive->m_pState);
    if (file_index < 0 || static_cast<size_t>(file_index) >= state->entries.size()) {
        return 0;
    }
    const ZipFileEntry& entry = state->entries[file_index];
    pStat->m_file_index = file_index;
    pStat->m_uncomp_size = entry.uncomp_size;
    pStat->m_comp_size = entry.comp_size;
    pStat->m_method = entry.method;
    pStat->m_crc32 = entry.crc32;
    pStat->m_local_header_ofs = entry.local_header_offset;
    strcpy_s(pStat->m_filename, sizeof(pStat->m_filename), entry.filename.c_str());
    return 1;
}

int mz_zip_reader_locate_file(mz_zip_archive* pArchive, const char* pName, const char* pComment, int flags) {
    if (!pArchive || !pArchive->m_pState || !pName) {
        return -1;
    }
    ZipState* state = static_cast<ZipState*>(pArchive->m_pState);
    std::string needle(pName);
    if ((flags & 1) == 0) {
        std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);
    }
    for (size_t i = 0; i < state->entries.size(); ++i) {
        std::string name = state->entries[i].filename;
        if ((flags & 1) == 0) {
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        }
        if (name == needle) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void* mz_zip_reader_extract_file_to_heap(mz_zip_archive* pArchive, int file_index, size_t* pSize, int flags) {
    if (!pArchive || !pArchive->m_pState) {
        return nullptr;
    }
    return ExtractFileToHeap(static_cast<ZipState*>(pArchive->m_pState), file_index, pSize);
}

} // extern "C"

std::string ZipResourceProvider::Utf16ToUtf8(const std::wstring& value) const {
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::string ZipResourceProvider::NormalizePath(const std::string& path) const {
    std::wstring widePath;
    const int size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (size > 0) {
        widePath.resize(size - 1);
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, widePath.data(), size);
    }
    std::replace(widePath.begin(), widePath.end(), L'\\', L'/');
    return Utf16ToUtf8(widePath);
}

ZipResourceProvider::ZipResourceProvider(std::wstring zipFilePath)
    : m_archive(nullptr), m_isValid(false) {
    std::wstring normalized = std::move(zipFilePath);
    if (!normalized.empty() && (normalized.back() == L'/' || normalized.back() == L'\\')) {
        normalized.pop_back();
    }

    std::string utf8Path = Utf16ToUtf8(normalized);
    mz_zip_archive* archive = new mz_zip_archive();
    memset(archive, 0, sizeof(*archive));
    const size_t fileCount = mz_zip_reader_init_file(archive, utf8Path.c_str(), 0);
    if (fileCount > 0) {
        m_archive = archive;
        m_isValid = true;
    } else {
        mz_zip_reader_end(archive);
        delete archive;
    }
}

ZipResourceProvider::~ZipResourceProvider() {
    if (m_archive) {
        mz_zip_archive* archive = static_cast<mz_zip_archive*>(m_archive);
        mz_zip_reader_end(archive);
        delete archive;
    }
}

bool ZipResourceProvider::IsValid() const {
    return m_isValid;
}

bool ZipResourceProvider::Exists(const std::string& path) {
    if (!m_isValid || !m_archive) {
        return false;
    }
    mz_zip_archive* archive = static_cast<mz_zip_archive*>(m_archive);
    std::string pathUtf8 = NormalizePath(path);
    int index = mz_zip_reader_locate_file(archive, pathUtf8.c_str(), nullptr, 0);
    return index >= 0;
}

std::vector<uint8_t> ZipResourceProvider::LoadBytes(const std::string& path) {
    if (!m_isValid || !m_archive) {
        return {};
    }
    mz_zip_archive* archive = static_cast<mz_zip_archive*>(m_archive);
    std::string pathUtf8 = NormalizePath(path);
    int index = mz_zip_reader_locate_file(archive, pathUtf8.c_str(), nullptr, 0);
    if (index < 0) {
        return {};
    }
    size_t size = 0;
    void* data = mz_zip_reader_extract_file_to_heap(archive, index, &size, 0);
    if (!data) {
        return {};
    }
    std::vector<uint8_t> result(static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
    mz_free(data);
    return result;
}

std::string ZipResourceProvider::LoadText(const std::string& path) {
    std::vector<uint8_t> bytes = LoadBytes(path);
    if (bytes.empty()) {
        return {};
    }
    return std::string(bytes.begin(), bytes.end());
}
