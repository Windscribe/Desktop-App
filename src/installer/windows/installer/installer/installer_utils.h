#pragma once

#include <Windows.h>
#include <string>

namespace InstallerUtils
{

DWORD getOSBuildNumber();
std::wstring getExecutableVersion(const std::wstring &path);
bool installedAppVersionLessThan(const std::wstring &version);
bool installedAppVersion2_3orLess();
std::wstring selectInstallFolder(HWND owner, const std::wstring &title, const std::wstring &initialFolder);

}
