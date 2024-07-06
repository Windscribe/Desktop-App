#include "path.h"

#include <shlwapi.h>
#include <filesystem>

#include "utils.h"

using namespace std;

namespace Path
{

wstring addSeparator(const wstring &fileName)
{
    if (fileName.empty() || fileName.ends_with(L'\\') || fileName.ends_with(L'/')) {
        return fileName;
    }

    return fileName + L'\\';
}

wstring removeSeparator(const wstring &fileName)
{
    if (fileName.empty() || !(fileName.ends_with(L'\\') || fileName.ends_with(L'/'))) {
        return fileName;
    }

    return fileName.substr(0, fileName.size() - 1);
}

wstring extractName(const wstring &fileName)
{
    filesystem::path path(fileName);
    return path.filename();
}

wstring extractDir(const wstring &fileName)
{
    filesystem::path path(fileName);
    return path.parent_path();
}

bool isRoot(const std::wstring& fileName)
{
    return ::PathIsRoot(fileName.c_str());
}

bool isOnSystemDrive(const std::wstring& fileName)
{
    if (!fileName.empty()) {
        wstring systemDir = Utils::getSystemDir();
        if (!systemDir.empty()) {
            return ::PathIsSameRootW(systemDir.c_str(), fileName.c_str());;
        }
    }

    return false;
}

bool equivalent(const std::wstring& fileName1, const std::wstring& fileName2)
{
    // Cannot use std::filesystem::equivalent(), as it expects both paths to reference
    // a valid file system entity.  Also cannot use filesystem::path::compare as it
    // does a case-sensitive comparison.

    filesystem::path fsPath1(fileName1);
    filesystem::path fsPath2(fileName2);

    // make_preferred to normalize the path separator.
    wstring path1 = removeSeparator(fsPath1.make_preferred());
    wstring path2 = removeSeparator(fsPath2.make_preferred());

    return (::StrCmpIW(path1.c_str(), path2.c_str()) == 0);
}

wstring append(const std::wstring &dir, const std::wstring &suffix)
{
    std::filesystem::path parent(dir);
    parent.append(suffix);
    return parent.make_preferred();
}

bool isSystemProtected(const wstring &dir)
{
    // We're not doing any actual 'system protected' checks here.  The user should be installing
    // to 'Program Files'.  Any other system-protected location is an anti-pattern.

    // make_preferred to normalize the path separator.
    filesystem::path fsDir(dir);
    const auto targetDir = removeSeparator(fsDir.make_preferred());
    const auto programFiles = Utils::programFilesFolder();
    return ::PathIsPrefix(programFiles.c_str(), dir.c_str());
}

}
