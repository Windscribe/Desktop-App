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

    // True when the user asked to also remove their settings and data because they will not
    // be reinstalling: via the confirmation dialog's checkbox, or /DELETEALLDATA on the
    // command line.  Never set for a plain /VERYSILENT uninstall, which is how the installer
    // removes the previous version during an upgrade.
    static void setDeleteAllData(bool deleteAllData);

    // Shows the uninstall confirmation dialog, with a checkbox to also delete the user's
    // settings and data.  deleteAllData supplies the checkbox's initial state and receives
    // its final state.  Returns true if the user confirmed the uninstall.
    static bool confirmUninstall(bool &deleteAllData);

    // Returns true if the uninstallation ran to completion and removed the
    // installation cleanly (registry entry and installation folder); false if it
    // was cancelled, aborted early, or left leftovers behind.  Callers use this
    // to decide whether the diagnostic logs in %ProgramData%\Windscribe may be
    // deleted (see WinMain in main.cpp).
    static bool RunSecondPhase();

    static int messageBox(const UINT resourceID, const UINT type, HWND ownerWindow = nullptr);

private:
    static std::wstring UninstExeFile;
    static bool isSilent_;
    static bool deleteAllData_;

private:
    static std::wstring loadString(const UINT resourceID, const wchar_t *fallback);

    static void DelayDeleteFile(const std::wstring Filename, const unsigned int MaxTries,
                                const unsigned int FirstRetryDelayMS,
                                const unsigned int SubsequentRetryDelayMS);

    static void DeleteUninstallDataFiles();

    typedef void (*P_DeleteUninstallDataFilesProc)();
    static bool PerformUninstall(const P_DeleteUninstallDataFilesProc DeleteUninstallDataFilesProc, const std::wstring& path_for_installation);

    static bool InitializeUninstall();

    static bool ProcessMsgs();

    static void UninstallSplitTunnelDriver();
    static void UninstallOpenVPNDCODriver(const std::wstring& installationPath);
    static void UninstallTapWindows6Driver(const std::wstring& installationPath);
    static void UninstallWireGuardService();

    static void UninstallHelper();
};
