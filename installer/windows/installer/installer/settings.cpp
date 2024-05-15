#include "settings.h"

#include <QSettings>

#include "../../utils/applicationinfo.h"
#include "../../utils/path.h"
#include "wincryptutils.h"

Settings::Settings()
{
    setPath(ApplicationInfo::defaultInstallPath());
}

void Settings::setPath(const std::wstring &path)
{
    if (!path.empty()) {
        path_ = Path::removeSeparator(path);
    }
}

std::wstring Settings::getPath() const
{
    return path_;
}

void Settings::setCreateShortcut(bool create_shortcut)
{
    isCreateShortcut_ = create_shortcut;
}

bool Settings::getCreateShortcut() const
{
    return isCreateShortcut_;
}

void Settings::setInstallDrivers(bool install)
{
    isInstallDrivers_ = install;
}

bool Settings::getInstallDrivers() const
{
    return isInstallDrivers_;
}

void Settings::setAutoStart(bool autostart)
{
    isAutoStart_ = autostart;
}

bool Settings::getAutoStart() const
{
    return isAutoStart_;
}

void Settings::setFactoryReset(bool erase)
{
    isFactoryReset_ = erase;
}

bool Settings::getFactoryReset() const
{
    return isFactoryReset_;
}

void Settings::setCredentials(const std::wstring &username, const std::wstring &password)
{
    username_ = username;
    password_ = password;
}

void Settings::readFromRegistry()
{
    QSettings reg(QString::fromStdWString(ApplicationInfo::installerRegistryKey()), QSettings::NativeFormat);

    if (reg.contains("applicationPath")) {
        const auto path = reg.value("applicationPath").toString().toStdWString();

        if (!path.empty() && Path::isOnSystemDrive(path)) {
            // For security purposes, ensure the user did not try to 'tweak' the Registry entry
            // to specify installation on a non-system drive.
            setPath(path);
        }
    }

    if (reg.contains("isCreateShortcut")) {
        setCreateShortcut(reg.value("isCreateShortcut").toBool());
    }
}

void Settings::writeToRegistry() const
{
    QSettings reg(QString::fromStdWString(ApplicationInfo::installerRegistryKey()), QSettings::NativeFormat);
    reg.setValue("applicationPath", QString::fromStdWString(path_));
    reg.setValue("isCreateShortcut", isCreateShortcut_);

    if (!username_.empty() && !password_.empty()) {
        const auto encodedUsername = wsl::WinCryptUtils::encrypt(username_, wsl::WinCryptUtils::EncodeHex);
        const auto encodedPassword = wsl::WinCryptUtils::encrypt(password_, wsl::WinCryptUtils::EncodeHex);

        reg.setValue("username", QString::fromStdString(encodedUsername));
        reg.setValue("password", QString::fromStdString(encodedPassword));
        reg.remove("authHash");
    }
}
