#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

#include "types/splittunneling.h"

namespace LinuxUtils
{
    const QString LAST_INSTALL_PLATFORM_FILE = WS_POSIX_CONFIG_DIR "/platform";
    const QString DEB_PLATFORM_NAME_X64 = QString("linux_deb_x64");
    const QString DEB_PLATFORM_NAME_X64_CLI = QString("linux_deb_x64_cli");
    const QString DEB_PLATFORM_NAME_ARM64 = QString("linux_deb_arm64");
    const QString DEB_PLATFORM_NAME_ARM64_CLI = QString("linux_deb_arm64_cli");
    const QString RPM_PLATFORM_NAME_X64 = QString("linux_rpm_x64");
    const QString RPM_PLATFORM_NAME_X64_CLI = QString("linux_rpm_x64_cli");
    const QString RPM_PLATFORM_NAME_ARM64  = QString("linux_rpm_arm64");
    const QString RPM_PLATFORM_NAME_ARM64_CLI = QString("linux_rpm_arm64_cli");
    const QString RPM_OPENSUSE_PLATFORM_NAME = QString("linux_rpm_opensuse_x64");
    const QString RPM_OPENSUSE_PLATFORM_NAME_CLI = QString("linux_rpm_opensuse_x64_cli");
    const QString ZST_PLATFORM_NAME = QString("linux_zst_x64");
    const QString ZST_PLATFORM_NAME_CLI = QString("linux_zst_x64_cli");

    QString getOsVersionString();
    QString getLinuxKernelVersion();
    QString getDistroName();
    const QString getLastInstallPlatform();
    gid_t getAppGid();
    bool isImmutableDistro();

    // CLI
    bool isAppAlreadyRunning();

    // Split Tunneling
    QMap<QString, QString> enumerateInstalledPrograms();
    QString extractExeName(const QString &execLine);
    QString unescape(const QString &in);
    QString convertToAbsolutePath(const QString &in);

    // Migrate stored split-tunneling app entries that came from older clients (where Flatpak entries
    // stored a binary basename like "firefox") to the Flatpak app ID (e.g. "org.mozilla.firefox"),
    // which is what the helper now matches against via cgroup. Idempotent.
    void migrateSplitTunnelingFlatpakApps(QVector<types::SplitTunnelingApp> &apps);
}
