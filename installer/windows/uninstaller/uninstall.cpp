#include "uninstall.h"

#include <sstream>
#include <VersionHelpers.h>

#include "../utils/applicationinfo.h"
#include "../utils/authhelper.h"
#include "../utils/paths_to_folders.h"
#include "../utils/process1.h"
#include "../utils/registry.h"


using namespace std;

wstring Uninstaller::UninstExeFile;
bool Uninstaller::isSilent_ = false;

std::list<wstring> Uninstaller::DirsNotRemoved;

RemoveDirectory1 Uninstaller::remove_directory;
Redirection Uninstaller::redirection;

Path Uninstaller::path;
Services Uninstaller::services;

Uninstaller::Uninstaller()
{
}

Uninstaller::~Uninstaller()
{
}

void Uninstaller::setUninstExeFile(const wstring& exe_file, bool bFirstPhase)
{
    UninstExeFile = exe_file;
}

void Uninstaller::setSilent(bool isSilent)
{
    isSilent_ = isSilent;
}

bool Uninstaller::ProcessMsgs()
{
    tagMSG Msg;

    bool Result = false;

    while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (Msg.message == WM_QUIT)
        {
            Result = true;
            break;
        }

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Result;
}

void Uninstaller::ProcessDirsNotRemoved()
{

}

bool Uninstaller::DeleteDirProc(const bool DisableFsRedir, const wstring DirName)
{
    return remove_directory.DeleteDir(DisableFsRedir, DirName, &DirsNotRemoved/*PDeleteDirData(Param)^.DirsNotRemoved*/, nullptr);
}

bool Uninstaller::DeleteFileProc(const bool DisableFsRedir, const wstring FileName)
{
    bool Result;

    Log::instance().out(L"Deleting file: " + FileName);
    Result = redirection.DeleteFileRedir(DisableFsRedir, FileName);
    if (Result == false)
    {
        //char buffer[100];
        //sprintf (buffer, "Failed to delete the file; it may be in use %lu",GetLastError());

        //Log::instance().trace(string(buffer)+"\n");
    }
    return Result;
}

void Uninstaller::DelayDeleteFile(const bool DisableFsRedir, const wstring Filename,
                                  const unsigned int MaxTries, const unsigned int FirstRetryDelayMS,
                                  const unsigned int SubsequentRetryDelayMS)
{
    // Attempts to delete Filename up to MaxTries times, retrying if the file is
    // in use. It sleeps FirstRetryDelayMS msec after the first try, and
    // SubsequentRetryDelayMS msec after subsequent tries.
    for (unsigned int I = 0; I < MaxTries; I++)
    {
        if (I == 1)
        {
            Sleep(FirstRetryDelayMS);
        }
        else
        {
            if (I > 1)
            {
                Sleep(SubsequentRetryDelayMS);
            }

            DWORD error;
            error = GetLastError();
            if (redirection.DeleteFileRedir(DisableFsRedir, Filename) ||
                (error == ERROR_FILE_NOT_FOUND) ||
                (error == ERROR_PATH_NOT_FOUND))
            {
                break;
            }
        }
    }
}

void Uninstaller::DeleteUninstallDataFiles()
{
    Log::instance().out(L"Deleting data file %s", UninstExeFile.c_str());
    DelayDeleteFile(false, UninstExeFile, 13, 50, 250);
}

bool Uninstaller::PerformUninstall(const P_DeleteUninstallDataFilesProc DeleteUninstallDataFilesProc, const wstring& path_for_installation)
{
    Log::instance().out(L"Starting the uninstallation process.");

    ::CoInitialize(nullptr);

    wstring str = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + ApplicationInfo::instance().getId() + L"_is1";
    LSTATUS ret = Registry::RegDeleteKeyIncludingSubkeys1(rvDefault, HKEY_LOCAL_MACHINE, str.c_str());

    if (ret != ERROR_SUCCESS) {
        Log::instance().out(L"Uninstaller::PerformUninstall - failed to delete registry entry %s (%lu)", str.c_str(), ret);
    }

    remove_directory.DelTree(false, path_for_installation, true, true, true, false, nullptr, nullptr, nullptr);

    Log::instance().out(L"Deleting shortcuts.");

    PathsToFolders paths_to_folders;
    wstring common_desktop = paths_to_folders.GetShellFolder(true, sfDesktop, false);

    wstring group = paths_to_folders.GetShellFolder(true, sfPrograms, false);

    redirection.DeleteFileRedir(false, common_desktop + L"\\" + ApplicationInfo::instance().getName() + L".lnk");
    remove_directory.DelTree(false, group + L"\\" + ApplicationInfo::instance().getName(), true, true, true, false, nullptr, nullptr, nullptr);

    if (DeleteUninstallDataFilesProc != nullptr)
    {
        DeleteUninstallDataFilesProc();
        // Now that uninstall data is deleted, try removing the directories it
        // was in that couldn't be deleted before.
        ProcessDirsNotRemoved();
    }

    ::CoUninitialize();

    Log::instance().out(L"Uninstallation process succeeded.");

    return true;
}

void Uninstaller::RunSecondPhase()
{
    Log::instance().out(L"Original Uninstall EXE: " + UninstExeFile);

    if (!InitializeUninstall())
    {
        Log::instance().out(L"return after InitializeUninstall()");
        PostQuitMessage(0);
        return;
    }

    unsigned long iResultCode;

    if (services.serviceExists(L"WindscribeService"))
    {
        services.simpleStopService(L"WindscribeService", true, true);
        services.simpleDeleteService(L"WindscribeService");
    }

    wstring path_for_installation = path.PathExtractDir(UninstExeFile);

    Log::instance().out(L"Running uninstall second phase with path: %s", path_for_installation.c_str());

    AuthHelper::removeRegEntriesForAuthHelper(path_for_installation);

    if (!isSilent_)
    {
        Log::instance().out(L"turn off firewall: %s", path_for_installation.c_str());
        Process::InstExec(path_for_installation + L"\\WindscribeService.exe", L"-firewall_off", path_for_installation, ewWaitUntilTerminated, SW_HIDE, iResultCode);
    }

    Log::instance().out(L"kill openvpn process");
    Process::InstExec(L"taskkill", L"/f /im windscribeopenvpn.exe", L"", ewWaitUntilTerminated, SW_HIDE, iResultCode);

    Log::instance().out(L"uninstall tap adapter");
    Process::InstExec(path_for_installation + L"\\tap\\tapinstall.exe", L"remove tapwindscribe0901", path_for_installation + L"\\tap", ewWaitUntilTerminated, SW_HIDE, iResultCode);

    Log::instance().out(L"uninstall wintun adapter");
    Process::InstExec(path_for_installation + L"\\wintun\\tapinstall.exe", L"remove windtun420", path_for_installation + L"\\wintun", ewWaitUntilTerminated, SW_HIDE, iResultCode);

    Log::instance().out(L"uninstall split tunnel driver");
    UninstallSplitTunnelDriver(path_for_installation);

    Log::instance().out(L"perform uninstall");
    PerformUninstall(&DeleteUninstallDataFiles, path_for_installation);

    // open uninstall screen in browser
    wstring userId;
    if (Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2\\", L"userId", userId) == false) userId = L"";
    if (userId != L"")
    {
        wstring urlStr = L"https://windscribe.com/uninstall/desktop?user=" + userId;
        ShellExecute(nullptr, L"open", urlStr.c_str(), L"", L"", SW_SHOW);
    }

    // remove any existing MAC spoofs from registry
    std::wstring subkeyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    std::vector<std::wstring> networkInterfaceNames = Registry::regGetSubkeyChildNames(HKEY_LOCAL_MACHINE, subkeyPath.c_str());

    for (std::wstring networkInterfaceName : networkInterfaceNames)
    {
        std::wstring interfacePath = subkeyPath + networkInterfaceName;

        if (Registry::regHasSubkeyProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed"))
        {
            Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"NetworkAddress");
            Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed");
        }
    }

    Log::instance().out(L"remove_directory.RemoveDir %s", path_for_installation.c_str());
    remove_directory.RemoveDir(path_for_installation.c_str());

    Log::instance().out(L"remove_directory.RemoveDir finished");

    if (!isSilent_)
    {
        MessageBox(nullptr,
            reinterpret_cast<LPCWSTR>(L"Windscribe was successfully removed from your computer."),
            reinterpret_cast<LPCWSTR>(L"Windscribe Uninstall"),
            MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
    }
    PostQuitMessage(0);
}

bool Uninstaller::InitializeUninstall()
{
    if (isSilent_) {
        return true;
    }

    HWND hwnd = ApplicationInfo::getAppMainWindowHandle();

    while (hwnd)
    {
        if (MessageBox(nullptr,
            reinterpret_cast<LPCWSTR>(L"Close Windscribe to continue. Please note, your connection will not be protected while the application is off."),
            reinterpret_cast<LPCWSTR>(L"Windscribe Uninstall"),
            MB_ICONINFORMATION | MB_RETRYCANCEL | MB_SETFOREGROUND) == IDCANCEL)
        {
            return false;
        }

        hwnd = ApplicationInfo::getAppMainWindowHandle();
    }

    return true;
}

// Attempt to unpin a shortcut. Returns True if the shortcut was successfully
// removed from the list of pinned items and/or the taskbar, or if the shortcut
// was not pinned at all. http://msdn.microsoft.com/en-us/library/bb774817.aspx
bool Uninstaller::UnpinShellLink(const wstring Filename)
{
    bool Result = false;
    IShellItem* ShellItem = nullptr;
    IStartMenuPinnedList* StartMenuPinnedList = nullptr;

    wstring Filename1 = path.PathExpand(Filename);

    if (SUCCEEDED(::SHCreateItemFromParsingName(Filename1.c_str(), nullptr, IID_IShellItem, reinterpret_cast<void**>(&ShellItem))) &&
        SUCCEEDED(::CoCreateInstance(CLSID_StartMenuPin, nullptr, CLSCTX_INPROC_SERVER, IID_IStartMenuPinnedList, reinterpret_cast<void**>(&StartMenuPinnedList))))
    {
        Result = (StartMenuPinnedList->RemoveFromList(ShellItem) == S_OK);
    }
    else {
        Result = true;
    }

    if (StartMenuPinnedList != nullptr) {
        StartMenuPinnedList->Release();
    }

    if (ShellItem != nullptr) {
        ShellItem->Release();
    }

    return Result;
}

void Uninstaller::UninstallSplitTunnelDriver(const std::wstring& installationPath)
{
    wostringstream commandLine;
    commandLine << L"setupapi,InstallHinfSection DefaultUninstall 132 " << Path::AddBackslash(installationPath) << L"splittunnel\\windscribesplittunnel.inf";

    auto result = Process::InstExec(L"rundll32", commandLine.str(), 60 * 1000, SW_HIDE);

    if (!result.has_value()) {
        Log::instance().out("WARNING: The split tunnel driver uninstall failed to launch.");
    }
    else if (result.value() == WAIT_TIMEOUT) {
        Log::instance().out("WARNING: The split tunnel driver uninstall stage timed out.");
        return;
    }
    else if (result.value() != NO_ERROR) {
        Log::instance().out("WARNING: The split tunnel driver uninstall returned a failure code (%lu).", result.value());
    }
}
