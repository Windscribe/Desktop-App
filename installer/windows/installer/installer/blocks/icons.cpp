#include "icons.h"

#include <versionhelpers.h>

#include "../settings.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/directory.h"
#include "../../../utils/path.h"
#include "../../../utils/registry.h"
#include "../../../utils/logger.h"

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

    const wstring& installPath = Settings::instance().getPath();
    const wstring installPathWithPathSep = Path::AddBackslash(installPath);
    const wstring uninstallExeFilename = installPathWithPathSep + ApplicationInfo::instance().getUninstall();

    if (isCreateShortcut_) {
        //C:\\Users\\Public\\Desktop\\Windscribe.lnk
        wstring common_desktop = pathsToFolders_.GetShellFolder(true, sfDesktop, false);
        createShortcut(common_desktop + L"\\" + ApplicationInfo::instance().getName() + L".lnk",
                       installPathWithPathSep + L"Windscribe.exe", L"", installPath, L"", 0);
    }

    const wstring group = pathsToFolders_.GetShellFolder(true, sfPrograms, false);

    //C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Uninstall Windscribe.lnk
    createShortcut(group + L"\\"+ ApplicationInfo::instance().getName() + L"\\Uninstall Windscribe.lnk",
                   uninstallExeFilename, L"", installPath, L"", 0);

    //C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Windscribe\\Windscribe.lnk
    createShortcut(group + L"\\" + ApplicationInfo::instance().getName() + L"\\Windscribe.lnk",
                   installPathWithPathSep + L"Windscribe.exe", L"", installPath, L"", 0);

    CoUninitialize();
    return 100;
}


void Icons::createShortcut(const wstring link, const wstring target, const wstring params, const wstring workingDir, const wstring icon, const int idx)
{
    HRESULT result;
    IShellLink *psl = nullptr;
    IPersistFile *ppf = nullptr;
    wstring dir = Path::PathExtractPath(link);

    DeleteFile(link.c_str());
    CreateDirectory(dir.c_str(), nullptr);

    result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,IID_IShellLink, reinterpret_cast<LPVOID*>(&psl));
    if (result != S_OK) {
        Log::instance().out("Could not create shell link");
        psl->Release();
        return;
    }
    result = psl->SetPath(target.c_str());
    if (result != S_OK) {
        Log::instance().out("Could not set path");
        psl->Release();
        return;
    }
    result = psl->SetArguments(params.c_str());
    if (result != S_OK) {
        Log::instance().out("Could not set arguments");
        psl->Release();
        return;
    }
    result = psl->SetWorkingDirectory(workingDir.c_str());
    if (result != S_OK) {
        Log::instance().out("Could not set working dir");
        psl->Release();
        return;
    }
    if (!icon.empty()) {
        result = psl->SetIconLocation(icon.c_str(), idx);
        if (result != S_OK) {
            Log::instance().out("Could not set icon");
            psl->Release();
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

    ppf->Release();
    psl->Release();

    if (result != S_OK) {
        Log::instance().out(L"Could not save shell link");
        return;
    }

    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, link.c_str(), nullptr);
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, dir.c_str(), nullptr);
}