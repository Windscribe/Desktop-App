#pragma once

#include <functional>
#include <string>

class InstallerShim
{
public:
    enum INSTALLER_STATE { STATE_INIT, STATE_EXTRACTING, STATE_CANCELED, STATE_FINISHED, STATE_ERROR, STATE_LAUNCHED, STATE_EXTRACTED };
    enum INSTALLER_ERROR { ERROR_OTHER = 1, ERROR_PERMISSION, ERROR_KILL, ERROR_CONNECT_HELPER, ERROR_DELETE, ERROR_UNINSTALL, ERROR_MOVE_CUSTOM_DIR };

    static InstallerShim &instance()
    {
        static InstallerShim shim;
        return shim;
    }

    INSTALLER_STATE state();
    INSTALLER_ERROR lastError();
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

