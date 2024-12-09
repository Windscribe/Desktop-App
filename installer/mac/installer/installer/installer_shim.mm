#include "installer_shim.h"

#include <string>

#include "installer.h"

InstallerShim::InstallerShim()
{
    installer_ = (__bridge_retained void *)([[Installer alloc] init]);
}

InstallerShim::~InstallerShim()
{
    CFRelease(installer_);
}

void InstallerShim::start(bool isUpdating)
{
    [(__bridge Installer *)installer_ start];
}

void InstallerShim::setCallback(std::function<void()> func)
{
    [(__bridge Installer *)installer_ setCallback:func];
}

void InstallerShim::finish()
{
    [(__bridge Installer *)installer_ runLauncher];
}

void InstallerShim::cancel()
{
    [(__bridge Installer *)installer_ cancel];
}

wsl::INSTALLER_STATE InstallerShim::state()
{
    return ((__bridge Installer *)installer_).currentState;
}

int InstallerShim::progress()
{
    return ((__bridge Installer *)installer_).progress;
}

void InstallerShim::setInstallDir(const std::wstring &path)
{
    // not supported on Mac
    return;
}

std::wstring InstallerShim::installDir()
{
    // not supported on Mac
    return L"";
}

bool InstallerShim::isCreateShortcutEnabled()
{
    // not supported on Mac
    return false;
}

void InstallerShim::setCreateShortcut(bool on)
{
    // not supported on Mac
}

bool InstallerShim::isFactoryResetEnabled()
{
    // not supported on Mac
    return false;
}

void InstallerShim::setFactoryReset(bool on)
{
    ((__bridge Installer *)installer_).factoryReset = on;
}

wsl::INSTALLER_ERROR InstallerShim::lastError()
{
    return ((__bridge Installer *)installer_).lastError;
}

bool InstallerShim::isAutoStartEnabled()
{
    // not supported on Mac
    return true;
}

void InstallerShim::setAutoStart(bool on)
{
    // not supported on Mac
}

bool InstallerShim::isInstallDriversEnabled(bool on)
{
    // not supported on Mac
    return true;
}

void InstallerShim::setInstallDrivers(bool on)
{
    // not supported on Mac
}

void InstallerShim::setCredentials(const std::wstring &username, const std::wstring &password)
{
    // not supported on Mac
}

std::string InstallerShim::username()
{
    NSString *name = NSUserName();
    return std::string([name UTF8String]);
}
