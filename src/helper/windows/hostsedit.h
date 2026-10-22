#pragma once

#include <windows.h>

#include <string>
#include <string_view>
#include <vector>

class HostsEdit
{
public:
    static HostsEdit &instance()
    {
        static HostsEdit he;
        return he;
    }

    bool addHosts(const std::wstring &ip, const std::wstring &hostname);
    bool removeHosts();

private:
    std::string addedByMarker_;
    std::wstring systemDir_;

    HostsEdit();
    virtual ~HostsEdit();

    bool ensureHostsFileWritable(const std::wstring &path, bool allowMissing, bool &wasReadOnly) const;
    void restoreReadOnly(const std::wstring &path) const;
    HANDLE openFile(const std::wstring &path, DWORD desiredAccess, DWORD creationDisposition) const;
    bool readFileBytes(HANDLE file, std::string &bytes) const;
    bool writeFileBytes(HANDLE file, const void *data, DWORD size) const;
    void logWin32Error(const char *what, const std::wstring &path, DWORD error, bool logLockers = false) const;
    std::wstring getHostsPath() const;
    std::wstring getTempHostsPath() const;
    bool stringInLines(const std::vector<std::string_view> &lines, const std::string &str) const;
};
