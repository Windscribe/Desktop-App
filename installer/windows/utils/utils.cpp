#include "utils.h"

#include <shlwapi.h>
#include <shlobj_core.h>

#include "applicationinfo.h"
#include "logger.h"


static std::wstring getProgramFilesFolder(bool get64bit)
{
    int csidl = (get64bit ? CSIDL_PROGRAM_FILES : CSIDL_PROGRAM_FILESX86);

    TCHAR programFilesPath[MAX_PATH];
    bool result = ::SHGetSpecialFolderPath(0, programFilesPath, csidl, FALSE);
    if (!result) {
        Log::WSDebugMessage(L"SHGetSpecialFolderPath(%d) failed", csidl);
        return std::wstring();
    }

    return std::wstring(programFilesPath);
}

namespace Utils
{
    std::wstring defaultInstallPath()
    {
        std::wstring defaultInstallPath = getProgramFilesFolder(true) + L"\\" + ApplicationInfo::instance().getName();
        return defaultInstallPath;
    }

	// Check if 'path' is a sub-folder of the "Program Files (x86)" folder.
	bool in32BitProgramFilesFolder(const std::wstring path)
	{
        std::wstring folder = getProgramFilesFolder(false);
		if (folder.empty()) {
            return false;
		}

        return ::PathIsPrefix(folder.c_str(), path.c_str());
	}
}
