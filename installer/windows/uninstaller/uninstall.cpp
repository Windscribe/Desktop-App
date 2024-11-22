#include "uninstall.h"

#include <filesystem>
#include <shlwapi.h>
#include <Shobjidl.h>
#include <spdlog/spdlog.h>

#include "authhelper.h"
#include "registry.h"
#include "remove_directory.h"
#include "resource.h"
#include "../utils/applicationinfo.h"
#include "../utils/path.h"
#include "../utils/utils.h"

#include "global_consts.h"
#include "servicecontrolmanager.h"


using namespace std;

wstring Uninstaller::UninstExeFile;
bool Uninstaller::isSilent_ = false;

static bool deleteFile(const wstring &fileName)
{
    return ::DeleteFile(fileName.c_str());
}

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

    while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
        if (Msg.message == WM_QUIT) {
            Result = true;
            break;
        }

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Result;
}

void Uninstaller::DelayDeleteFile(const wstring Filename, const unsigned int MaxTries,
                                  const unsigned int FirstRetryDelayMS,
                                  const unsigned int SubsequentRetryDelayMS)
{
    // Attempts to delete Filename up to MaxTries times, retrying if the file is
    // in use. It sleeps FirstRetryDelayMS msec after the first try, and
    // SubsequentRetryDelayMS msec after subsequent tries.
    for (unsigned int I = 0; I < MaxTries; I++) {
        if (I == 1) {
            Sleep(FirstRetryDelayMS);
        }
        else {
            if (I > 1) {
                Sleep(SubsequentRetryDelayMS);
            }

            DWORD error;
            error = GetLastError();
            if (deleteFile(Filename) || (error == ERROR_FILE_NOT_FOUND) || (error == ERROR_PATH_NOT_FOUND)) {
                break;
            }
        }
    }
}

void Uninstaller::DeleteUninstallDataFiles()
{
    spdlog::info(L"Deleting data file {}", UninstExeFile.c_str());
    DelayDeleteFile(UninstExeFile, 13, 50, 250);
}

bool Uninstaller::PerformUninstall(const P_DeleteUninstallDataFilesProc DeleteUninstallDataFilesProc, const wstring& path_for_installation)
{
    spdlog::info(L"Starting the uninstallation process.");

    ::CoInitialize(nullptr);

    const wstring keyName = ApplicationInfo::uninstallerRegistryKey(false);
    LSTATUS ret = Registry::RegDeleteKeyIncludingSubkeys1(HKEY_LOCAL_MACHINE, keyName.c_str());

    if (ret != ERROR_SUCCESS) {
        spdlog::error(L"Uninstaller::PerformUninstall - failed to delete registry entry {} ({})", keyName, ret);
    }

    RemoveDir::DelTree(path_for_installation, true, true, true, false);

    spdlog::info(L"Deleting shortcuts.");

    wstring common_desktop = Utils::desktopFolder();
    wstring group = Utils::startMenuProgramsFolder();

    deleteFile(common_desktop + L"\\" + ApplicationInfo::name() + L".lnk");
    RemoveDir::DelTree(group + L"\\" + ApplicationInfo::name(), true, true, true, false);

    if (DeleteUninstallDataFilesProc != nullptr) {
        DeleteUninstallDataFilesProc();
    }

    ::CoUninitialize();

    spdlog::info(L"Uninstallation process succeeded.");

    return true;
}

void Uninstaller::RunSecondPhase()
{
    spdlog::info(L"Original Uninstall EXE: {}", UninstExeFile);

    if (!InitializeUninstall()) {
        spdlog::info(L"return after InitializeUninstall()");
        PostQuitMessage(0);
        return;
    }

    UninstallHelper();

    wstring path_for_installation = Path::extractDir(UninstExeFile);

    spdlog::info(L"Running uninstall second phase with path: {}", path_for_installation);

    // Clear any existing auto-login entries.
    Registry::regDeleteProperty(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2", L"username");
    Registry::regDeleteProperty(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2", L"password");

    AuthHelper::removeRegEntriesForAuthHelper(path_for_installation);

    if (!isSilent_) {
        spdlog::info(L"turn off firewall: {}", path_for_installation);
        Utils::instExec(Path::append(path_for_installation, L"WindscribeService.exe"), L"-firewall_off", INFINITE, SW_HIDE);
    }

    spdlog::info(L"uninstall WireGuard service");
    UninstallWireGuardService();

    const auto taskkillExe = Path::append(Utils::getSystemDir(), L"taskkill.exe");

    spdlog::info(L"kill openvpn process");
    Utils::instExec(taskkillExe, L"/f /im windscribeopenvpn.exe", INFINITE, SW_HIDE);

    spdlog::info(L"kill wstunnel process");
    Utils::instExec(taskkillExe, L"/f /im windscribewstunnel.exe", INFINITE, SW_HIDE);

    spdlog::info(L"kill ctrld process");
    Utils::instExec(taskkillExe, L"/f /im windscribectrld.exe", INFINITE, SW_HIDE);

    spdlog::info(L"uninstall split tunnel driver");
    UninstallSplitTunnelDriver();

    spdlog::info(L"uninstall OpenVPN DCO driver");
    UninstallOpenVPNDCODriver(path_for_installation);

    spdlog::info(L"perform uninstall");
    PerformUninstall(&DeleteUninstallDataFiles, path_for_installation);

    // open uninstall screen in browser
    wstring userId;
    Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2\\", L"userId", userId);
    if (!userId.empty()) {
        wstring urlStr = L"https://windscribe.com/uninstall/desktop?user=" + userId;
        ::ShellExecute(nullptr, L"open", urlStr.c_str(), L"", L"", SW_SHOW);
    }

    // remove any existing MAC spoofs from registry
    wstring subkeyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    vector<wstring> networkInterfaceNames = Registry::regGetSubkeyChildNames(HKEY_LOCAL_MACHINE, subkeyPath.c_str());

    for (const auto &networkInterfaceName : networkInterfaceNames) {
        wstring interfacePath = subkeyPath + networkInterfaceName;

        if (Registry::regHasSubkeyProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed")) {
            Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"NetworkAddress");
            Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed");
        }
    }

    spdlog::info(L"Removing directory {}", path_for_installation);
    ::RemoveDirectory(path_for_installation.c_str());

    // remove any leftover files in Program Files, since we write some configs/logs here regardless of installation path
    wstring wsPath = Path::append(Utils::programFilesFolder(), L"Windscribe");
    if (!Path::equivalent(wsPath, path_for_installation)) {
        spdlog::info(L"Removing directory in Program Files");
        RemoveDir::DelTree(wsPath.c_str(), true, true, true, false);
    }

    if (!isSilent_) {
        messageBox(IDS_UNINSTALL_SUCCESS, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
    }

    PostQuitMessage(0);
}

bool Uninstaller::InitializeUninstall()
{
    if (isSilent_) {
        return true;
    }

    HWND hwnd = Utils::appMainWindowHandle();

    while (hwnd) {
        int result = messageBox(IDS_CLOSE_APP, MB_ICONINFORMATION | MB_RETRYCANCEL | MB_SETFOREGROUND);
        if (result == IDCANCEL) {
            return false;
        }

        hwnd = Utils::appMainWindowHandle();
    }

    return true;
}

void Uninstaller::UninstallSplitTunnelDriver()
{
    try {
        wsl::ServiceControlManager svcCtrl;
        svcCtrl.openSCM(SC_MANAGER_ALL_ACCESS);

        error_code ec;
        if (!svcCtrl.deleteService(L"WindscribeSplitTunnel", ec)) {
            throw system_error(ec);
        }

        wstring targetFile = Path::append(Utils::getSystemDir(), L"drivers\\windscribesplittunnel.sys");
        if (filesystem::exists(targetFile)) {
            filesystem::remove(targetFile, ec);
            if (ec) {
                throw system_error(ec);
            }
        }
    }
    catch (system_error& ex) {
        spdlog::warn("WARNING: failed to uninstall the split tunnel driver - {}", ex.what());
    }
}

void Uninstaller::UninstallHelper()
{
    const wstring serviceName = ApplicationInfo::serviceName();
    try {
        wsl::ServiceControlManager svcCtrl;
        svcCtrl.openSCM(SC_MANAGER_ALL_ACCESS);

        if (svcCtrl.isServiceInstalled(serviceName.c_str())) {
            svcCtrl.deleteService(serviceName.c_str());
        }
    }
    catch (system_error& ex) {
        spdlog::warn("WARNING: failed to delete the Windscribe service - {}", ex.what());
    }
}

int Uninstaller::messageBox(const UINT resourceID, const UINT type, HWND ownerWindow)
{
    // TODO: **JDRM** May need to add MB_RTLREADING flag to MessageBox call if the OS is set to a RTL language.

    const int bufSize = 512;
    wchar_t title[bufSize];
    wchar_t message[bufSize];
    ::ZeroMemory(title, bufSize * sizeof(wchar_t));
    ::ZeroMemory(message, bufSize * sizeof(wchar_t));

    HMODULE hModule = ::GetModuleHandle(NULL);
    int result = ::LoadString(hModule, IDS_MSGBOX_TITLE, title, bufSize);
    if (result == 0) {
        spdlog::error(L"Loading of the message box title string resource failed ({}).", ::GetLastError());
        wcscpy_s(title, bufSize, L"Uninstall Windscribe");
    }

    result = ::LoadString(hModule, resourceID, message, bufSize);
    if (result == 0) {
        spdlog::error(L"Loading of string resource {} failed ({}).", resourceID, ::GetLastError());
        wcscpy_s(message, bufSize, L"Windscribe uninstall encountered a technical failure.");
    }

    result = ::MessageBox(ownerWindow, message, title, type);

    return result;
}

void Uninstaller::UninstallOpenVPNDCODriver(const wstring& installationPath)
{
    wstring adapterOEMIdentifier;
    const wstring keyName = ApplicationInfo::installerRegistryKey(false);
    Registry::RegQueryStringValue(HKEY_CURRENT_USER, keyName.c_str(), L"ovpnDCODriverOEMIdentifier", adapterOEMIdentifier);
    if (adapterOEMIdentifier.empty()) {
        spdlog::warn(L"WARNING: failed to find 'ovpnDCODriverOEMIdentifier' entry in the installer registry key.");
        return;
    }

    const wstring appName = Path::append(installationPath, L"devcon.exe");
    const wstring commandLine = wstring(L"dp_delete ") + adapterOEMIdentifier;

    auto result = Utils::instExec(appName, commandLine, 30 * 1000, SW_HIDE);

    if (!result.has_value()) {
        spdlog::warn(L"WARNING: The OpenVPN DCO driver uninstall failed to launch.");
    }
    else if (result.value() == WAIT_TIMEOUT) {
        spdlog::warn(L"WARNING: The OpenVPN DCO driver uninstall stage timed out.");
    }
    else if (result.value() != NO_ERROR) {
        spdlog::warn(L"WARNING: The OpenVPN DCO driver uninstall returned a failure code ({}).", result.value());
    }
    else {
        spdlog::info(L"OpenVPN DCO driver ({}) successfully removed from the Windows driver store", adapterOEMIdentifier);
    }
}

void Uninstaller::UninstallWireGuardService()
{
    try {
        wsl::ServiceControlManager svcCtrl;
        svcCtrl.openSCM(SC_MANAGER_ALL_ACCESS);

        if (svcCtrl.isServiceInstalled(kWireGuardServiceIdentifier.c_str())) {
            spdlog::info(L"UninstallWireGuardService - deleting WireGuard service");
            std::error_code ec;
            if (!svcCtrl.deleteService(kWireGuardServiceIdentifier.c_str(), ec)) {
                throw std::system_error(ec);
            }
        }

        return;
    }
    catch (std::system_error& ex) {
        spdlog::error("Failed to delete the WireGuard service - {}", ex.what());
    }

    spdlog::info(L"UninstallWireGuardService - task killing the WireGuard service");
    Utils::instExec(Path::append(Utils::getSystemDir(), L"taskkill.exe"), L"/f /t /im WireguardService.exe", INFINITE, SW_HIDE);
}
