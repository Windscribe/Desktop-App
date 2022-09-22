#include "utils.h"

#include <shlwapi.h>
#include <shlobj_core.h>

#include "logger.h"


namespace Utils
{
    void deleteFile(const std::wstring fileName)
    {
        DWORD dwAttrib = ::GetFileAttributes(fileName.c_str());
        if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
            // If the path points to a symbolic link, the Win32 DeleteFile API will delete the symbolic link, not the target.
            ::DeleteFile(fileName.c_str());
        }
    }

	// Check if 'path' is a sub-folder of the "Program Files (x86)" folder.
	bool in32BitProgramFilesFolder(const std::wstring path)
	{
		TCHAR programFilesPath[MAX_PATH];
		bool result = ::SHGetSpecialFolderPath(0, programFilesPath, CSIDL_PROGRAM_FILESX86, FALSE);
		if (result) {
			result = ::PathIsPrefix(programFilesPath, path.c_str());
		}

		return result;
	}
}