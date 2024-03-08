#pragma once

#include <QString>
#include <QStringList>

namespace LinuxUtils
{
    const QString LAST_INSTALL_PLATFORM_FILE = "/etc/windscribe/platform";
    const QString DEB_PLATFORM_NAME_X64 = QString("linux_deb_x64");
    const QString DEB_PLATFORM_NAME_ARM64 = QString("linux_deb_arm64");
    const QString RPM_PLATFORM_NAME = QString("linux_rpm_x64");
    const QString ZST_PLATFORM_NAME = QString("linux_zst_x64");

    QString getOsVersionString();
    QString getLinuxKernelVersion();
    QString getDistroName();
    const QString getLastInstallPlatform();

    // CLI
    bool isGuiAlreadyRunning();

    // Split Tunneling
    QMap<QString, QString> enumerateInstalledPrograms();
    QString extractExeName(const QString &execLine);
    QString unescape(const QString &in);
    QString convertToAbsolutePath(const QString &in);
}
