#pragma once

#include <Windows.h>

#include <string>

class Uninstaller
{
public:
    Uninstaller();
    ~Uninstaller();

    static void setSilent(bool isSilent);
    static void setUninstExeFile(const std::wstring& exe_file, bool bFirstPhase);

    static void RunSecondPhase();

    static int messageBox(const UINT resourceID, const UINT type, HWND ownerWindow = nullptr);

private:
    static std::wstring UninstExeFile;
    static bool isSilent_;

private:
    static void DelayDeleteFile(const std::wstring Filename, const unsigned int MaxTries,
                                const unsigned int FirstRetryDelayMS,
                                const unsigned int SubsequentRetryDelayMS);

    static void DeleteUninstallDataFiles();

    typedef void (*P_DeleteUninstallDataFilesProc)();
    static bool PerformUninstall(const P_DeleteUninstallDataFilesProc DeleteUninstallDataFilesProc, const std::wstring& path_for_installation);

    static bool InitializeUninstall();

    static bool ProcessMsgs();

    static void UninstallSplitTunnelDriver(const std::wstring& installationPath);
    static void UninstallOpenVPNDCODriver(const std::wstring& installationPath);

    static void DeleteService();
};
