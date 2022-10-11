#ifndef UNINSTALLER_H
#define UNINSTALLER_H

#include <list>
#include <string>

#include "../utils/path.h"
#include "../utils/redirection.h"
#include "../utils/remove_directory.h"
#include "../utils/services.h"


class Uninstaller
{
public:
    Uninstaller();
    ~Uninstaller();

    static void setSilent(bool isSilent);
    static void setUninstExeFile(const std::wstring& exe_file, bool bFirstPhase);

    static void RunSecondPhase();

private:
    static Services services;
    static RemoveDirectory1 remove_directory;
    static Path path;
    static Redirection redirection;
    static std::wstring UninstExeFile;
    static bool isSilent_;
    static std::list<std::wstring> DirsNotRemoved;

private:
    static void ProcessDirsNotRemoved();

    static bool DeleteDirProc(const bool DisableFsRedir, const std::wstring DirName);
    static bool DeleteFileProc(const bool DisableFsRedir, const std::wstring FileName);

    static void DelayDeleteFile(const bool DisableFsRedir, const std::wstring Filename,
                                const unsigned int MaxTries, const unsigned int FirstRetryDelayMS,
                                const unsigned int SubsequentRetryDelayMS);

    static void DeleteUninstallDataFiles();

    typedef void (*P_DeleteUninstallDataFilesProc)();
    static bool PerformUninstall(const P_DeleteUninstallDataFilesProc DeleteUninstallDataFilesProc, const std::wstring& path_for_installation);

    static bool InitializeUninstall();

    static bool UnpinShellLink(const std::wstring Filename);

    static bool ProcessMsgs();

    static void UninstallSplitTunnelDriver(const std::wstring& installationPath);
};

#endif // UNINSTALLER_H
