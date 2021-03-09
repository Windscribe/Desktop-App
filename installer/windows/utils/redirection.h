#ifndef REDIRECTION_H
#define REDIRECTION_H
#include <Windows.h>
#include <string>
#include "directory.h"

class Redirection
{
private:
    bool FsRedirectionFunctionsAvailable;
    Directory dir;
public:
    struct TPreviousFsRedirectionState{
      bool DidDisable;
      void *OldValue;
    };


    bool DirExistsRedir(const bool DisableFsRedir, const std::wstring Filename);
    DWORD GetFileAttributesRedir(const bool DisableFsRedir, const std::wstring Filename);
    bool DisableFsRedirectionIf(const bool Disable, TPreviousFsRedirectionState PreviousState);
    void RestoreFsRedirection(const TPreviousFsRedirectionState PreviousState);
    HANDLE FindFirstFileRedir(const bool DisableFsRedir, const std::wstring Filename, WIN32_FIND_DATA &FindData);
    bool CreateProcessRedir(                   const wchar_t *lpApplicationName, const wchar_t *lpCommandLine,
                                               const LPSECURITY_ATTRIBUTES lpProcessAttributes,const LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                               const bool bInheritHandles, const DWORD dwCreationFlags,
                                               const void *lpEnvironment, const wchar_t *lpCurrentDirectory,
                                               LPSTARTUPINFO lpStartupInfo,
                                               LPPROCESS_INFORMATION lpProcessInformation);
    bool MoveFileExRedir(const bool DisableFsRedir,
    const std::wstring ExistingFilename, const std::wstring NewFilename, const DWORD Flags);
    bool DeleteFileRedir(const bool DisableFsRedir, const std::wstring Filename);
    bool IsDirectoryAndNotReparsePointRedir(const bool DisableFsRedir, const std::wstring Name);
    bool SetFileAttributesReDir(const bool DisableFsRedir, const std::wstring Filename, const DWORD Attrib);
    bool RemoveDirectoryReDir(const bool DisableFsRedir, const std::wstring Filename);
    bool CreateDirectoryRedir(const bool DisableFsRedir, const std::wstring Filename);
    bool IsDirectoryAndNotReparsePoint(const std::wstring Name);
    bool AreFsRedirectionFunctionsAvailable();
    bool NewFileExistsRedir(const bool DisableFsRedir, const std::wstring Filename);
    bool NewFileExists(const std::wstring Name);

    Redirection();
};

#endif // REDIRECTION_H
