#pragma once

#include <memory>
#include <Windows.h>

#include "archive/archive.h"
#include "../iinstall_block.h"

class UninstallPrev : public IInstallBlock
{
public:
    virtual int executeStep();
    UninstallPrev(bool isFactoryReset, double weight);

private:
    int state_;
    bool isFactoryReset_;
    std::unique_ptr<Archive> archive_;

    std::wstring getUninstallString();
    bool uninstallOldVersion(const std::wstring &uninstallString, DWORD &lastError);
    bool isPrevInstall64Bit();
    bool extractUninstaller();
    std::wstring removeQuotes(const std::wstring &str);
    void doFactoryReset();
    void stopService();
};
