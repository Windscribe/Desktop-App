#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace wsl {

// Outcome of an archive operation: the 7z exit code and its error output (stderr),
// so callers can log/report the actual failure instead of a generic error.
struct ArchiveResult
{
    enum class Status {
        Success,
        LaunchFailed,   // 7zr.exe is missing or could not be started (AV/Smart App Control?)
        ExtractFailed,  // extraction failed; see exitCode/errorText
    };

    Status status = Status::Success;
    uint32_t exitCode = 0;   // 7z process exit code, when it ran and failed
    std::wstring errorText;  // stderr of 7z, or a system error description

    bool success() const { return status == Status::Success; }
    explicit operator bool() const { return success(); }
};

class Archive
{
public:
    explicit Archive();
    void setLogFunction(const std::function<void(const std::wstring&)>& func);

    // Extract the embedded 7zr.exe decompressor to extractFolder.  Must be called once
    // before extract() if the folder doesn't already contain a trusted 7zr.exe.
    bool extractDecompressor(const std::wstring &extractFolder);

    // Extract archiveName from the embedded resourceName to the extractFolder, then
    // extract all files from the archive to the targetFolder.  The caller must have
    // placed a trusted 7zr.exe in extractFolder beforehand (see extractDecompressor).
    ArchiveResult extract(const std::wstring &resourceName, const std::wstring &archiveName,
                          const std::wstring &extractFolder, const std::wstring &targetFolder);

    // Extract archiveName from the embedded resourceName to the extractFolder, then
    // extract filename from the archive to the targetFolder.
    ArchiveResult extract(const std::wstring &resourceName, const std::wstring &archiveName,
                          const std::wstring &extractFolder, const std::wstring &filename,
                          const std::wstring &targetFolder);

private:
    std::function<void(const std::wstring&)> logFunction_ = nullptr;

private:
    ArchiveResult executeCmd(const std::wstring &cmd, const std::wstring &workingDir);
    void extractArchiveFromResource(const std::wstring &resourceName, const std::wstring &archiveName,
                                    const std::wstring &extractFolder);
    ArchiveResult extractFile(const std::wstring &archiveName, const std::wstring &extractFolder,
                              const std::wstring &filename, const std::wstring &targetFolder);
    ArchiveResult extractFiles(const std::wstring &archiveName, const std::wstring &extractFolder, const std::wstring &targetFolder);
    void log(const std::wstring& msg);

    std::wstring extractor(const std::wstring &folder) const;
};

}
