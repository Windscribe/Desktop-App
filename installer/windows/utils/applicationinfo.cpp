#include "applicationinfo.h"

#include <shlwapi.h>
#include <shlobj_core.h>

#include "path.h"
#include "utils.h"
#include "version/windscribe_version.h"

namespace ApplicationInfo {

using namespace std;

wstring name()
{
    return L"Windscribe";
}

std::wstring version()
{
    return WINDSCRIBE_VERSION_STR_UNICODE;
}

wstring uninstaller()
{
    return L"uninstall.exe";
}

std::wstring publisher()
{
    return L"Windscribe Limited";
}

wstring guid()
{
    return L"{fa690e90-ddb0-4f0c-b3f1-136c084e5fc7}";
}

std::wstring publisherUrl()
{
    return L"http://www.windscribe.com/";
}

std::wstring supportUrl()
{
    return L"http://www.windscribe.com/";
}

std::wstring updateUrl()
{
    return L"http://www.windscribe.com/";
}

wstring appExeName()
{
    return L"Windscribe.exe";
}

wstring defaultInstallPath()
{
    wstring programFiles = Utils::programFilesFolder();
    return Path::append(programFiles, name());
}

wstring appRegistryKey()
{
    return L"HKEY_CURRENT_USER\\Software\\Windscribe\\Windscribe2";
}

wstring installerRegistryKey(bool includeRootKey)
{
    wstring keyName;
    if (includeRootKey) {
        keyName += wstring(L"HKEY_CURRENT_USER\\");
    }

    keyName += L"Software\\Windscribe\\Installer";

    return keyName;
}

wstring uninstallerRegistryKey(bool includeRootKey)
{
    wstring keyName;
    if (includeRootKey) {
        keyName += wstring(L"HKEY_LOCAL_MACHINE\\");
    }

    keyName += wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + ApplicationInfo::guid() + wstring(L"_is1");

    return keyName;
}

wstring serviceName()
{
    return L"WindscribeService";
}

}
