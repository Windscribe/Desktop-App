#include "icons.h"

#include <Windows.h>
#include <shlobj.h>

#include "../settings.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/logger.h"
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

    if (isCreateShortcut_) {
        //C:\\Users\\Public\\Desktop\\Windscribe.lnk
        wstring common_desktop = Utils::DesktopFolder();
        createShortcut(common_desktop + L"\\" + ApplicationInfo::name() + L".lnk", appExe, L"", installPath, L"", 0);
    }

    const wstring group = Utils::StartMenuProgramsFolder();

    //C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Uninstall Windscribe.lnk
    createShortcut(group + L"\\"+ ApplicationInfo::name() + L"\\Uninstall Windscribe.lnk",
                   uninstallExe, L"", installPath, L"", 0);

    //C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Windscribe.lnk
    createShortcut(group + L"\\" + ApplicationInfo::name() + L"\\Windscribe.lnk", appExe, L"", installPath, L"", 0);

    CoUninitialize();
    return 100;
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

    DeleteFile(link.c_str());
    CreateDirectory(dir.c_str(), nullptr);

    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,IID_IShellLink, reinterpret_cast<LPVOID*>(&psl));
    if (result != S_OK) {
        Log::instance().out("Could not create shell link");
        return;
    }
    result = psl->SetPath(target.c_str());
    if (result != S_OK) {
        Log::instance().out("Could not set path");
        return;
    }
    result = psl->SetArguments(params.c_str());
    if (result != S_OK) {
        Log::instance().out("Could not set arguments");
        return;
    }
    result = psl->SetWorkingDirectory(workingDir.c_str());
    if (result != S_OK) {
        Log::instance().out("Could not set working dir");
        return;
    }
    if (!icon.empty()) {
        result = psl->SetIconLocation(icon.c_str(), idx);
        if (result != S_OK) {
            Log::instance().out("Could not set icon");
            return;
        }
    }

    result = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&ppf));
    if (result != S_OK) {
        Log::instance().out("Could not get ppf");
        psl->Release();
        return;
    }

    WCHAR wsz[MAX_PATH];
    wcscpy_s(wsz, link.length() + 1, link.c_str());

    result = ppf->Save(wsz, true);

    if (result != S_OK) {
        Log::instance().out(L"Could not save shell link");
        return;
    }

    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, link.c_str(), nullptr);
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, dir.c_str(), nullptr);
}