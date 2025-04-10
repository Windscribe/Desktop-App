#include "installer_shim.h"

#include "installer.h"
#include "settings.h"

InstallerShim::InstallerShim()
{
    installer_ = (void *)new Installer();

    Settings::instance().readFromRegistry();
}

InstallerShim::~InstallerShim()
{
    delete reinterpret_cast<Installer *>(installer_);
}

wsl::INSTALLER_STATE InstallerShim::state()
{
    return reinterpret_cast<Installer *>(installer_)->state();
}

int InstallerShim::progress()
{
    return reinterpret_cast<Installer *>(installer_)->progress();
}

void InstallerShim::start(bool isUpdating)
{
    reinterpret_cast<Installer *>(installer_)->start();
}

void InstallerShim::cancel()
{
    reinterpret_cast<Installer *>(installer_)->cancel();
}

void InstallerShim::setCallback(std::function<void()> func)
{
    reinterpret_cast<Installer *>(installer_)->setCallback(func);
}

void InstallerShim::finish()
{
    // We can persist this custom install path now that we know the install was successful.
    Settings::instance().writeToRegistry();

    reinterpret_cast<Installer *>(installer_)->launchApp();
}

void InstallerShim::setInstallDir(const std::wstring &path)
{
    Settings::instance().setPath(path);
}

std::wstring InstallerShim::installDir()
{
    return Settings::instance().getPath();
}

void InstallerShim::setFactoryReset(bool on)
{
    Settings::instance().setFactoryReset(on);
}

bool InstallerShim::isFactoryResetEnabled()
{
    return Settings::instance().getFactoryReset();
}

void InstallerShim::setCreateShortcut(bool on)
{
    Settings::instance().setCreateShortcut(on);
}

bool InstallerShim::isCreateShortcutEnabled()
{
    return Settings::instance().getCreateShortcut();
}

wsl::INSTALLER_ERROR InstallerShim::lastError()
{
    return reinterpret_cast<Installer *>(installer_)->lastError();
}

bool InstallerShim::isAutoStartEnabled()
{
    return Settings::instance().getAutoStart();
}

void InstallerShim::setAutoStart(bool on)
{
    Settings::instance().setAutoStart(on);
}

void InstallerShim::setCredentials(const std::wstring &username, const std::wstring &password)
{
    Settings::instance().setCredentials(username, password);
}

std::string InstallerShim::username()
{
    // Not needed on Windows
    return "";
}
