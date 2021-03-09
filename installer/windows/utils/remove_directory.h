#ifndef REMOVEDIRECTORY_H
#define REMOVEDIRECTORY_H

#include "logger.h"
#include "redirection.h"
#include "path.h"
#include "directory.h"
#include "version_os.h"

class RemoveDirectory1
{
 private:
    Redirection redirection;
    Path path;
    Directory directory;

 public:
    typedef bool (*P_DeleteFileProc)(const bool DisableFsRedir, const std::wstring FileName);
    typedef bool (*P_DeleteDirProc)(const bool DisableFsRedir, const std::wstring DirName);
    P_DeleteFileProc pDeleteFileProc;
    P_DeleteFileProc pDeleteDirProc;

    bool DelTree(const bool DisableFsRedir, const std::wstring Path,
    const bool IsDir, const bool DeleteFiles, const bool DeleteSubdirsAlso, const bool breakOnError,
    const P_DeleteDirProc DeleteDirProc, const P_DeleteFileProc DeleteFileProc,
    const void *Param);

    bool RemoveDir(std::wstring dir);

    bool DeleteDir(const bool DisableFsRedir, const std::wstring DirName,
    std::list<std::wstring> *DirsNotRemoved, std::list<std::wstring> *RestartDeleteDirList);
    bool IsDirEmpty(const bool DisableFsRedir, const std::wstring Dir);

    std::list<std::wstring> RestartDeleteDirList[2];
    void RestartDeleteDir(const bool DisableFsRedir, std::wstring Dir);
    bool ListContainsPathOrSubdir(std::list<std::wstring> *List,  const std::wstring Path);
    bool CompareMem(const void *P1, const void *P2);

    RemoveDirectory1();
};

#endif // REMOVEDIRECTORY_H
