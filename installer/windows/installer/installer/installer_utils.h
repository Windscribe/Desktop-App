#pragma once

#include <Windows.h>
#include <string>

namespace InstallerUtils
{

DWORD getOSBuildNumber();
bool installedAppVersionLessThan(const std::wstring &version);
std::wstring selectInstallFolder(HWND owner, const std::wstring &title, const std::wstring &initialFolder);

}
