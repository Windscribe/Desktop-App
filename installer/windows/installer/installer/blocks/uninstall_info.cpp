#include "uninstall_info.h"

#include "../settings.h"
#include "../../../utils/applicationInfo.h"
#include "../../../utils/registry.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../../../client/common/version/windscribe_version.h"

using namespace std;

UninstallInfo::UninstallInfo(double weight) : IInstallBlock(weight, L"uninstall_info")
{
}

UninstallInfo::~UninstallInfo()
{
}

int UninstallInfo::executeStep()
{
    registerUninstallInfo(ApplicationInfo::instance().getId());
    return 100;
}


void UninstallInfo::registerUninstallInfo(const wstring& uninstallRegKeyBaseName)
{
    const wstring& installPath = Settings::instance().getPath();
    wstring uninstallExeFilename = Path::AddBackslash(installPath) + ApplicationInfo::instance().getUninstall();

    HKEY rootKey = HKEY_LOCAL_MACHINE;

    std::wstring subkeyName = getUninstallRegSubkeyName(uninstallRegKeyBaseName);

    if (isExistInstallation(rootKey, subkeyName))
    {
        Registry::RegDeleteKeyIncludingSubkeys1(rvDefault, rootKey, subkeyName.c_str());
    }

    // Create uninstall key
    HKEY hUninstallKey;
    LSTATUS errorCode = Registry::RegCreateKeyExView(rvDefault, rootKey, subkeyName.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hUninstallKey, nullptr);
    if (errorCode != ERROR_SUCCESS)
    {
        return;
    }

    setStringValue(hUninstallKey, L"InstallLocation", Path::AddBackslash(installPath));
    setStringValue(hUninstallKey, NEWREGSTR_VAL_UNINSTALLER_DISPLAYNAME.c_str(), ApplicationInfo::instance().getName());
    setStringValue(hUninstallKey, L"DisplayIcon", Path::AddBackslash(installPath) + ApplicationInfo::instance().getName() + L".exe");
    setStringValue(hUninstallKey, L"UninstallString", L"\"" + uninstallExeFilename + L"\"");
    setStringValue(hUninstallKey, L"QuietUninstallString", L"\"" + uninstallExeFilename + L"\" /SILENT");
    setStringValue(hUninstallKey, L"DisplayVersion", WINDSCRIBE_VERSION_STR_UNICODE);
    setStringValue(hUninstallKey, L"Publisher", ApplicationInfo::instance().getPublisher());
    setStringValue(hUninstallKey, L"URLInfoAbout", ApplicationInfo::instance().getPublisherUrl());
    setStringValue(hUninstallKey, L"HelpLink", ApplicationInfo::instance().getSupportUrl());
    setStringValue(hUninstallKey, L"URLUpdateInfo", ApplicationInfo::instance().getUpdateUrl());

    setDWordValue(hUninstallKey, L"NoModify", 1);
    setDWordValue(hUninstallKey, L"NoRepair", 1);
    setStringValue(hUninstallKey, L"InstallDate", getInstallDateString());

    uint64_t estimatedSize = calculateDirSize(installPath, NULL);
    estimatedSize = estimatedSize / 1024;
    setDWordValue(hUninstallKey, L"EstimatedSize", static_cast<DWORD>(estimatedSize));

    RegCloseKey(hUninstallKey);
}

bool UninstallInfo::setStringValue(HKEY key, const wchar_t* valueName, const wstring& data)
{
    LSTATUS errorCode = RegSetValueEx(key, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(data.c_str()),
        static_cast<DWORD>(data.length() * sizeof(data[0])));
    return errorCode == ERROR_SUCCESS;
}

bool UninstallInfo::setDWordValue(HKEY key, const wchar_t* valueName, DWORD data)
{
    LSTATUS errorCode = RegSetValueEx(key, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&data), sizeof(data));
    return errorCode == ERROR_SUCCESS;
}

wstring UninstallInfo::getUninstallRegSubkeyName(const wstring& uninstallRegKeyBaseName)
{
    return NEWREGSTR_PATH_UNINSTALL + L"\\" + uninstallRegKeyBaseName + L"_is1";
}

bool UninstallInfo::isExistInstallation(HKEY rootKey, const std::wstring& subkeyName)
{
    HKEY k;
    bool result = false;
    if (Registry::RegOpenKeyExView(rvDefault, rootKey, subkeyName.c_str(), 0, KEY_QUERY_VALUE, k) == ERROR_SUCCESS)
    {
        result = true;
        RegCloseKey(k);
    }
    else
    {
        result = false;
    }

    return result;
}

//https://stackoverflow.com/questions/10015341/size-of-a-directory
uint64_t UninstallInfo::calculateDirSize(const String& path, StringVector* errVect, uint64_t size)
{
    WIN32_FIND_DATA data;
    HANDLE sh = nullptr;
    sh = FindFirstFile((path + L"\\*").c_str(), &data);

    if (sh == INVALID_HANDLE_VALUE)
    {
        //if we want, store all happened error
        if (errVect != nullptr)
            errVect->push_back(path);
        return size;
    }

    do
    {
        // skip current and parent
        if (!isBrowsePath(data.cFileName))
        {
            // if found object is ...
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
                // directory, then search it recursievly
                size = calculateDirSize(path + L"\\" + data.cFileName, nullptr, size);
            else
                // otherwise get object size and add it to directory size
                size += static_cast<uint64_t>(data.nFileSizeHigh * (MAXDWORD)+data.nFileSizeLow);
        }

    } while (FindNextFile(sh, &data)); // do

    FindClose(sh);

    return size;
}

bool UninstallInfo::isBrowsePath(const String& path)
{
    return ((path == L".") || (path == L".."));
}

wstring UninstallInfo::getInstallDateString()
{
    SYSTEMTIME ST;
    GetLocalTime(&ST);
    wchar_t buf[256];
    swprintf_s(buf, L"%.4u%.2u%.2u", ST.wYear, ST.wMonth, ST.wDay);

    return buf;
}
