#include <Windows.h>

#include <string>

#include "../utils/path.h"

namespace RemoveDir
{

using namespace std;

static bool isDirectoryAndNotReparsePoint(const std::wstring &Name)
{
    //{ Returns True if the specified directory exists and is NOT a reparse point. }
    DWORD Attr = GetFileAttributes(Name.c_str());
    return (Attr != 0xFFFFFFFF) && ((Attr & FILE_ATTRIBUTE_DIRECTORY) != 0) && ((Attr & FILE_ATTRIBUTE_REPARSE_POINT) == 0);
}

bool DelTree(const wstring Path, const bool IsDir, const bool DeleteFiles, const bool DeleteSubdirsAlso, const bool breakOnError)
    // Deletes the specified directory including all files and subdirectories in
    // it (including those with hidden, system, and read-only attributes). Returns
    // true if it was able to successfully remove everything. If breakOnError is
    // set to true it will stop and return false the first time a delete failed or
    // DeleteDirProc/DeleteFileProc returned false.
{
    bool Result = true;
    if ((DeleteFiles) && (!IsDir || isDirectoryAndNotReparsePoint(Path))) {
        wstring BasePath, FindSpec;
        if (IsDir) {
            BasePath = Path;
            FindSpec = Path::append(BasePath, L"*");
        }
        else {
            BasePath = Path::extractDir(Path);
            FindSpec = Path;
        }

        WIN32_FIND_DATA FindData;
        HANDLE H = ::FindFirstFile(FindSpec.c_str(), &FindData);
        if (H != INVALID_HANDLE_VALUE) {
            while (true) {
                wstring fileName = FindData.cFileName;
                if ((fileName != L".") && (fileName != L"..")) {
                    wstring filePath = Path::append(BasePath, fileName);
                    if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0) {
                        // Strip the read-only attribute if this is a file, or if it's a directory and we're deleting subdirectories also.
                        if (DeleteSubdirsAlso || (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                            ::SetFileAttributes(filePath.c_str(), FindData.dwFileAttributes & static_cast<DWORD>(~FILE_ATTRIBUTE_READONLY));
                        }
                    }

                    if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                        if (!::DeleteFile(filePath.c_str())) {
                            Result = false;
                        }
                    }
                    else {
                        if (DeleteSubdirsAlso) {
                            if (!DelTree(filePath, true, true, true, breakOnError)) {
                                Result = false;
                            }
                        }
                    }
                }
                if ((breakOnError && !Result) || !::FindNextFile(H, &FindData)) break;
            }

            ::FindClose(H);
        }
    }

    if ((!breakOnError || Result) && IsDir) {
        Result = ::RemoveDirectory(Path.c_str());
    }

    return Result;
}

}