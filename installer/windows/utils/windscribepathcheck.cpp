#include "windscribepathcheck.h"

#include <shlwapi.h>

#include "path.h"

namespace PathCheck
{

bool isNeedAppendSubdirectory(const std::wstring &installPath, const std::wstring &prevInstallPath)
{
    // this is current app dir, then nothing to append
    if (!installPath.empty() && !prevInstallPath.empty() && Path::equivalent(installPath, prevInstallPath)) {
        return false;
    }

    // is directory exists?
    bool isDirExists = false;
    DWORD ftyp = GetFileAttributes(installPath.c_str());
    if (ftyp != INVALID_FILE_ATTRIBUTES) {
        if (ftyp & FILE_ATTRIBUTE_DIRECTORY) {
            isDirExists = true;
        }
    }

    // check if directory empty
    bool isDirEmpty = false;
    if (isDirExists) {
        isDirEmpty = PathIsDirectoryEmpty(installPath.c_str());
    }

    //  if the selected directory has files in it, need to append app name to it
    return (isDirExists && !isDirEmpty);
}

}