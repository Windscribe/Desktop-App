#pragma once

#include <Windows.h>

#include "../iinstall_block.h"

namespace wsl { class ServiceControlManager; }

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
    bool terminateService(wsl::ServiceControlManager &scm) const;
    bool killProcess(DWORD processId, const std::wstring &expectedImagePath, int waitMs) const;
    int taskKill(const std::wstring &exeName) const;
    void terminateProtocolHandlers() const;
};
