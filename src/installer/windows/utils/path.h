#pragma once

#include <string>

namespace Path
{
    std::wstring addSeparator(const std::wstring &fileName);
    std::wstring append(const std::wstring &dir, const std::wstring &suffix);
    std::wstring extractDir(const std::wstring &fileName);
    std::wstring extractName(const std::wstring &fileName);
    std::wstring removeSeparator(const std::wstring &fileName);

    bool equivalent(const std::wstring& fileName1, const std::wstring& fileName2);
    bool isOnSystemDrive(const std::wstring& fileName);
    bool isRoot(const std::wstring& fileName);
    bool isSystemProtected(const std::wstring &dir);
}
