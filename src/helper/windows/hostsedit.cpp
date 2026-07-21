#include "ws_branding.h"
#include <shlwapi.h>

#include <stdio.h>

#include <cwctype>

#include <spdlog/spdlog.h>

#include "hostsedit.h"
#include "utils.h"
#include "utils/wsscopeguard.h"

HostsEdit::HostsEdit() : addedByMarker_(L"added by " WS_PRODUCT_NAME_W L", do not modify.")
{
    systemDir_ = Utils::getSystemDir();
}

HostsEdit::~HostsEdit()
{
}

bool HostsEdit::addHosts(const std::wstring &ip, const std::wstring &hostname)
{
    FILE *file = nullptr;
    const errno_t err = _wfopen_s(&file, getHostsPath().c_str(), L"a+");
    if (err != 0 || !file) {
        spdlog::error("HostsEdit::addHosts could not open hosts file for appending (errno {})", err);
        return false;
    }

    std::vector<std::wstring> strs;
    wchar_t buf[10000];
    while (fgetws(buf, 10000, file) != nullptr) {
        strs.push_back(buf);
    }

    std::wstring entry = ip + L" " + hostname;
    if (!stringInVector(strs, entry)) {
        // Add a separating newline if the file isn't empty and doesn't already end with one.
        // fgetws preserves the trailing newline, so the last line we read tells us this; an
        // empty file (strs empty) needs no leading newline.
        if (!strs.empty() && !strs.back().empty() && strs.back().back() != L'\n') {
            fseek(file, 0, SEEK_END);
            fputws(L"\n", file);
        }

        fputws(entry.c_str(), file);
        fputws(L"   #", file);
        fputws(addedByMarker_.c_str(), file);
    }
    fclose(file);
    return true;
}

bool HostsEdit::makeHostsFileWritable() const
{
    const auto hostsFile = getHostsPath();
    const auto attributes = ::GetFileAttributes(hostsFile.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        spdlog::error("GetFileAttributes of hosts file failed ({})", ::GetLastError());
        return false;
    }

    if (::SetFileAttributes(hostsFile.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY) == FALSE) {
        spdlog::error("SetFileAttributes of hosts file failed ({})", ::GetLastError());
        return false;
    }

    return true;
}

bool HostsEdit::removeHosts()
{
    const auto tempHostsPath = getTempHostsPath();
    const auto hostsPath = getHostsPath();

    // Ensure the files are closed before we attempt the move.
    {
        FILE *fileTmp = nullptr;
        FILE *file = nullptr;
        auto closeFiles = wsl::wsScopeGuard([&] {
            if (file) fclose(file);
            if (fileTmp) fclose(fileTmp);
        });

        errno_t err = _wfopen_s(&fileTmp, tempHostsPath.c_str(), L"w");
        if (err != 0 || !fileTmp) {
            spdlog::error("HostsEdit::removeHosts could not open temp hosts file for writing (errno {})", err);
            return false;
        }

        err = _wfopen_s(&file, hostsPath.c_str(), L"r");
        if (err != 0 || !file) {
            spdlog::error("HostsEdit::removeHosts could not open hosts file for reading (errno {})", err);
            return false;
        }

        std::vector<std::wstring> strs;
        wchar_t buf[10000];
        while (fgetws(buf, 10000, file) != nullptr) {
            if (wcsstr(buf, addedByMarker_.c_str()) == NULL) {
                strs.push_back(buf);
            }
        }

        for (std::vector<std::wstring>::iterator it = strs.begin(); it != strs.end(); ++it) {
            std::wstring str = *it;
            // remove newline for last string
            if (it == (strs.end() - 1)) {
                if (!str.empty() && str.back() == L'\n') {
                    str.pop_back();
                }
            }
            fputws(str.c_str(), fileTmp);
        }
    }

    if (MoveFileEx(tempHostsPath.c_str(), hostsPath.c_str(), MOVEFILE_REPLACE_EXISTING) == 0) {
        spdlog::error("HostsEdit::removeHosts could not move temp hosts file over hosts file ({})", ::GetLastError());
        return false;
    }

    return true;
}

std::wstring HostsEdit::getHostsPath() const
{
    wchar_t szPath[MAX_PATH];
    PathCombine(szPath, systemDir_.c_str(), L"drivers\\etc\\hosts");
    return szPath;
}

std::wstring HostsEdit::getTempHostsPath() const
{
    wchar_t szPath[MAX_PATH];
    PathCombine(szPath, systemDir_.c_str(), L"drivers\\etc\\hosts.tmp");
    return szPath;
}

bool HostsEdit::stringInVector(const std::vector<std::wstring> &vec, const std::wstring &str)
{
    if (str.empty()) {
        return false;
    }

    for (const auto &line : vec) {
        // Ensure we match the string exactly (e.g. is not a substring).
        for (size_t pos = line.find(str); pos != std::wstring::npos; pos = line.find(str, pos + 1)) {
            const bool boundaryBefore = (pos == 0) || iswspace(line[pos - 1]);
            const size_t after = pos + str.size();
            const bool boundaryAfter = (after == line.size()) || iswspace(line[after]) || line[after] == L'#';
            if (boundaryBefore && boundaryAfter) {
                return true;
            }
        }
    }
    return false;
}
