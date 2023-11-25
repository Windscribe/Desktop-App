#pragma once

#include <string>

namespace ApplicationInfo
{

std::wstring appExeName();
std::wstring guid();
std::wstring name();
std::wstring publisher();
std::wstring publisherUrl();
std::wstring serviceName();
std::wstring supportUrl();
std::wstring uninstaller();
std::wstring updateUrl();
std::wstring version();

std::wstring appRegistryKey();
std::wstring installerRegistryKey();
std::wstring uninstallerRegistryKey(bool includeRootKey = true);

std::wstring defaultInstallPath();

};
