#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

struct archive;
struct archive_entry;

namespace wsl {

// Outcome of an archive operation, with the reason for a failure so callers can log/report it
// instead of a generic error.
struct ArchiveResult
{
    bool succeeded = true;
    std::wstring errorText;

    bool success() const { return succeeded; }
    explicit operator bool() const { return success(); }
};

// Derived from PayloadExtractor in the ControlD desktop installer (app-desktop repository, installer/payloadextractor.h).
// This copy adds single-file extraction and returns the failure reason; consolidate the two if either changes.

// Extracts a 7z archive embedded in this executable's resources, in-process.  The archive is decoded
// straight out of the resource in this image's memory; the only files written are the ones it contains.
class PayloadExtractor
{
public:
    void setLogFunction(const std::function<void(const std::wstring&)>& func);

    // Extracts every entry of the archive embedded as resourceName into targetFolder, which is created
    // if it does not exist.  Files written before a failure are left in place.
    ArchiveResult extract(const std::wstring &resourceName, const std::filesystem::path &targetFolder);

    // Extracts only the top-level file named filename from the archive embedded as resourceName.
    ArchiveResult extract(const std::wstring &resourceName, const std::wstring &filename, const std::filesystem::path &targetFolder);

private:
    std::function<void(const std::wstring&)> logFunction_ = nullptr;

    void log(const std::wstring &msg) const;
    ArchiveResult extractResource(const std::wstring &resourceName, const std::wstring *filename,
                                  const std::filesystem::path &targetFolder);
    void extractFromMemory(const void *data, size_t size, const std::wstring *filename, const std::filesystem::path &targetFolder);
    void extractEntry(archive *reader, archive_entry *entry, const std::wstring &name, const std::filesystem::path &targetFolder) const;
    void writeFile(archive *reader, const std::filesystem::path &target, int64_t expectedSize) const;
};

}
