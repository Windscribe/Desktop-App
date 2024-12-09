#pragma once

#include <functional>
#include <string>

#include "installerenums.h"

class InstallerShim
{
public:
    static InstallerShim &instance()
    {
        static InstallerShim shim;
        return shim;
    }

    wsl::INSTALLER_STATE state();
    wsl::INSTALLER_ERROR lastError();
    int progress();
    void setCallback(std::function<void()> func);
    // NB: can't ifdef platform-specific functions out for Mac here because we can't pull in any Qt headers
    std::wstring installDir();
    void setInstallDir(const std::wstring &path);
    bool isFactoryResetEnabled();
    void setFactoryReset(bool on);
    bool isCreateShortcutEnabled();
    void setCreateShortcut(bool on);
    bool isAutoStartEnabled();
    void setAutoStart(bool on);
    bool isInstallDriversEnabled(bool on);
    void setInstallDrivers(bool on);
    void setCredentials(const std::wstring &username, const std::wstring &password);
    std::string username();

    void start(bool isUpdating);
    void cancel();
    void finish();

private:
    void *installer_;

    InstallerShim();
    ~InstallerShim();
};
