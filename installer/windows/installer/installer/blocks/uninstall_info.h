#ifndef UNINSTALL_INFO_H
#define UNINSTALL_INFO_H

#include <Windows.h>
#include <vector>

#include "../iinstall_block.h"


//  Register uninstall information so the program can be uninstalled
//  through the Add/Remove Programs Control Panel applet.
class UninstallInfo : public IInstallBlock
{
public:
    UninstallInfo(double weight);
    virtual ~UninstallInfo();

    int executeStep();

private:
    std::wstring uninstallExeFilename_;

    const std::wstring NEWREGSTR_VAL_UNINSTALLER_COMMANDLINE = L"UninstallString";
    const std::wstring NEWREGSTR_VAL_UNINSTALLER_DISPLAYNAME = L"DisplayName";
    const std::wstring NEWREGSTR_PATH_SETUP = L"Software\\Microsoft\\Windows\\CurrentVersion";
    const std::wstring NEWREGSTR_PATH_UNINSTALL = NEWREGSTR_PATH_SETUP + L"\\Uninstall";

    bool setStringValue(HKEY key, const wchar_t* valueName, const std::wstring& data);
    bool setDWordValue(HKEY key, const wchar_t* valueName, DWORD data);
    std::wstring getUninstallRegSubkeyName(const std::wstring& uninstallRegKeyBaseName);

    bool isExistInstallation(HKEY rootKey, const std::wstring& subkeyName);

    typedef std::wstring String;
    typedef std::vector<String> StringVector;
    typedef unsigned long long uint64_t;
    uint64_t calculateDirSize(const String& path, StringVector* errVect = nullptr, uint64_t size = 0);
    bool isBrowsePath(const String& path);
    std::wstring getInstallDateString();
    void registerUninstallInfo(const std::wstring& uninstallRegKeyBaseName);
};

#endif // UNINSTALL_INFO_H
