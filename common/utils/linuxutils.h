#ifndef LINUXUTILS_H
#define LINUXUTILS_H

#include <QString>
#include <QMap>


namespace LinuxUtils
{
    QString getOsVersionString();
    void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName);
    QString getLinuxKernelVersion();
    const QString getLastInstallPlatform();

    // CLI
    bool isGuiAlreadyRunning();

    std::string execCmd(const char *cmd);

    const QString LAST_INSTALL_PLATFORM_FILE = "/etc/windscribe/platform";
    const QString DEB_PLATFORM_NAME = QString("linux_deb_x64");
    const QString RPM_PLATFORM_NAME = QString("linux_rpm_x64");
}


#endif // LINUXUTILS_H
