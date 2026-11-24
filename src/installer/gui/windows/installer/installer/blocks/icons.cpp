#include "icons.h"

#include <Windows.h>
#include <shlobj.h>
#include <spdlog/spdlog.h>

#include "../settings.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/path.h"
#include "../../../utils/utils.h"
#include "wsscopeguard.h"

using namespace std;

Icons::Icons(bool isCreateShortcut, double weight) : IInstallBlock(weight, L"icons"), isCreateShortcut_(isCreateShortcut)
{
}

Icons::~Icons()
{
}

int Icons::executeStep()
{
    CoInitialize(nullptr);

    const wstring installPath  = Settings::instance().getPath();
    const wstring uninstallExe = Path::append(installPath, ApplicationInfo::uninstaller());
    const wstring appExe       = Path::append(installPath, ApplicationInfo::appExeName());

    const wstring desktopLink = desktopShortcutPath();
    if (!desktopLink.empty()) {
        if (isCreateShortcut_) {
            createShortcut(desktopLink, appExe, L"", installPath, L"", 0);
        } else {
            DeleteFile(desktopLink.c_str());
        }
    }

    const wstring group = Utils::startMenuProgramsFolder();

    //C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Uninstall Windscribe.lnk
    createShortcut(group + L"\\"+ ApplicationInfo::name() + L"\\Uninstall Windscribe.lnk",
                   uninstallExe, L"", installPath, L"", 0);

    //C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Windscribe.lnk
    createShortcut(group + L"\\" + ApplicationInfo::name() + L"\\Windscribe.lnk", appExe, L"", installPath, L"", 0);

    CoUninitialize();
    return 100;
}

wstring Icons::desktopShortcutPath()
{
    //C:\\Users\\Public\\Desktop\\Windscribe.lnk
    const wstring common_desktop = Utils::desktopFolder();
    if (common_desktop.empty()) {
        return wstring();
    }
    return common_desktop + L"\\" + ApplicationInfo::name() + L".lnk";
}


void Icons::createShortcut(const wstring link, const wstring target, const wstring params, const wstring workingDir, const wstring icon, const int idx)
{
    IShellLink *psl = nullptr;
    IPersistFile *ppf = nullptr;
    wstring dir = Path::extractDir(link);

    auto exitGuard = wsl::wsScopeGuard([&]
    {
        if (ppf != nullptr) {
            ppf->Release();
        }

        if (psl != nullptr) {
            psl->Release();
        }
    });

    CreateDirectory(dir.c_str(), nullptr);

    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,IID_IShellLink, reinterpret_cast<LPVOID*>(&psl));
    if (result != S_OK) {
        spdlog::error(L"Could not create shell link");
        return;
    }

    result = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&ppf));
    if (result != S_OK) {
        spdlog::error(L"Could not get ppf");
        return;
    }

    // Saved over rather than deleted first: Windows forgets a desktop icon's position once the file is gone.
    const DWORD attrs = GetFileAttributes(link.c_str());
    const bool exists = (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);

    result = psl->SetPath(target.c_str());
    if (result != S_OK) {
        spdlog::error(L"Could not set path");
        return;
    }
    result = psl->SetArguments(params.c_str());
    if (result != S_OK) {
        spdlog::error(L"Could not set arguments");
        return;
    }
    result = psl->SetWorkingDirectory(workingDir.c_str());
    if (result != S_OK) {
        spdlog::error(L"Could not set working dir");
        return;
    }
    if (!icon.empty()) {
        result = psl->SetIconLocation(icon.c_str(), idx);
        if (result != S_OK) {
            spdlog::error(L"Could not set icon");
            return;
        }
    }

    result = ppf->Save(link.c_str(), true);

    if (result != S_OK) {
        spdlog::error(L"Could not save shell link");
        return;
    }

    SHChangeNotify(exists ? SHCNE_UPDATEITEM : SHCNE_CREATE, SHCNF_PATH, link.c_str(), nullptr);
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, dir.c_str(), nullptr);
}
