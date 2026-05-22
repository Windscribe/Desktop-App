#pragma once

#include <functional>
#include <string>

namespace wsl {

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
    bool extract(const std::wstring &resourceName, const std::wstring &archiveName,
                 const std::wstring &extractFolder, const std::wstring &targetFolder);

    // Extract archiveName from the embedded resourceName to the extractFolder, then
    // extract filename from the archive to the targetFolder.
    bool extract(const std::wstring &resourceName, const std::wstring &archiveName,
                 const std::wstring &extractFolder, const std::wstring &filename,
                 const std::wstring &targetFolder);

private:
    std::function<void(const std::wstring&)> logFunction_ = nullptr;

private:
    void executeCmd(const std::wstring &cmd, const std::wstring &workingDir);
    void extractArchiveFromResource(const std::wstring &resourceName, const std::wstring &archiveName,
                                    const std::wstring &extractFolder);
    void extractFile(const std::wstring &archiveName, const std::wstring &extractFolder,
                     const std::wstring &filename, const std::wstring &targetFolder);
    void extractFiles(const std::wstring &archiveName, const std::wstring &extractFolder, const std::wstring &targetFolder);
    void log(const std::wstring& msg);

    std::wstring extractor(const std::wstring &folder) const;
};

}
