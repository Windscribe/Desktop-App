#include "installer_utils.h"

#include <shlobj_core.h>
#include <shlwapi.h>
#include <filesystem>
#include <spdlog/spdlog.h>

#include "settings.h"
#include "../../utils/applicationinfo.h"

using namespace std;

namespace InstallerUtils
{

DWORD getOSBuildNumber()
{
    HMODULE hDLL = ::GetModuleHandleA("ntdll.dll");
    if (hDLL == NULL) {
        spdlog::error(L"Failed to load the ntdll module ({})", ::GetLastError());
        return 0;
    }

    typedef NTSTATUS (WINAPI* RtlGetVersionFunc)(LPOSVERSIONINFOEXW lpVersionInformation);

    RtlGetVersionFunc rtlGetVersionFunc = (RtlGetVersionFunc)::GetProcAddress(hDLL, "RtlGetVersion");
    if (rtlGetVersionFunc == NULL) {
        spdlog::error(L"Failed to load RtlGetVersion function ({})", ::GetLastError());
        return 0;
    }

    RTL_OSVERSIONINFOEXW rtlOsVer;
    ::ZeroMemory(&rtlOsVer, sizeof(RTL_OSVERSIONINFOEXW));
    rtlOsVer.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    rtlGetVersionFunc(&rtlOsVer);

    return rtlOsVer.dwBuildNumber;
}

static wstring getVersionInfoItem(const wstring &exeName, const wstring &itemName)
{
    wstring itemValue;

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;

    DWORD dwNotUsed;
    DWORD dwSize = ::GetFileVersionInfoSize(exeName.c_str(), &dwNotUsed);

    if (dwSize > 0) {
        unique_ptr<UCHAR[]> versionInfo(new UCHAR[dwSize]);
        BOOL result = ::GetFileVersionInfo(exeName.c_str(), 0L, dwSize, versionInfo.get());
        if (result) {
            // To get a string value must pass query in the form
            // "\StringFileInfo\<langID><codepage>\keyname"
            // where <langID><codepage> is the languageID concatenated with the code page, in hex.
            UINT nTranslateLen;
            result = ::VerQueryValue(versionInfo.get(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &nTranslateLen);

            if (result) {
                for (UINT i = 0; i < (nTranslateLen/sizeof(struct LANGANDCODEPAGE)); ++i) {
                    wchar_t subBlock[MAX_PATH];
                    swprintf_s(subBlock, MAX_PATH, L"\\StringFileInfo\\%04x%04x\\%s", lpTranslate[i].wLanguage,
                               lpTranslate[i].wCodePage, itemName.c_str());

                    LPVOID lpvi;
                    UINT   nLen;
                    result = ::VerQueryValue(versionInfo.get(), subBlock, &lpvi, &nLen);

                    if (result && nLen > 0) {
                        itemValue = wstring((LPCTSTR)lpvi);
                        break;
                    }
                }
            }
        }
    }

    return itemValue;
}

bool installedAppVersionLessThan(const wstring &version)
{
    filesystem::path appExe(Settings::instance().getPath());
    appExe.append(ApplicationInfo::appExeName());

    error_code ec;
    if (!filesystem::exists(appExe, ec)) {
        return false;
    }

    const wstring installedAppVersion = getVersionInfoItem(appExe.native(), L"ProductVersion");
    if (installedAppVersion.empty()) {
        spdlog::error(L"installedAppVersionLessThan - failed to retrieve installed app version");
        return false;
    }

    int result = ::StrCmpLogicalW(installedAppVersion.c_str(), version.c_str());
    return (result < 0);
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED) {
        wstring tmp = (const wchar_t*)lpData;
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
    return 0;
}

wstring selectInstallFolder(HWND owner, const wstring &title, const wstring &initialFolder)
{
    wstring installFolder;

    BROWSEINFO bi = { 0 };
    bi.hwndOwner = owner;
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)initialFolder.c_str();

    LPITEMIDLIST pidl = ::SHBrowseForFolder(&bi);

    if (pidl != 0) {
        //get the name of the folder and put it in path
        wchar_t path[MAX_PATH];
        auto result = ::SHGetPathFromIDList(pidl, path);
        ::CoTaskMemFree(pidl);

        if (result) {
            installFolder = path;
        }
    }

    return installFolder;
}

}
