#pragma once

#include <Windows.h>

#include "../iinstall_block.h"

class UninstallPrev : public IInstallBlock
{
public:
    virtual int executeStep();
    UninstallPrev(bool isFactoryReset, double weight);

private:
    int state_;
    bool isFactoryReset_;

    std::wstring getUninstallString() const;
    bool uninstallOldVersion(const std::wstring &uninstallString, DWORD &lastError) const;
    bool isPrevInstall64Bit() const;
    bool extractUninstaller() const;
    std::wstring removeQuotes(const std::wstring &str) const;
    void doFactoryReset() const;
    void stopService() const;
    int taskKill(const std::wstring &exeName) const;
    void terminateProtocolHandlers() const;
    std::wstring getPrevClientVersion() const;
};
