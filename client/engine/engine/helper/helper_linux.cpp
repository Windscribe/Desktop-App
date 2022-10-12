#include "helper_linux.h"

#include <stdlib.h>

#include "utils/logger.h"


Helper_linux::Helper_linux(QObject *parent) : Helper_posix(parent)
{
}

Helper_linux::~Helper_linux()
{
}

void Helper_linux::startInstallHelper()
{
    start(QThread::LowPriority);
}

bool Helper_linux::reinstallHelper()
{
    return false;
}

QString Helper_linux::getHelperVersion()
{
    return "";
}

bool Helper_linux::setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    Q_UNUSED(isIkev2);
    Q_UNUSED(ifIndex);
    Q_UNUSED(overrideDnsIpAddress);
    return false;
}

std::optional<bool> Helper_linux::installUpdate(const QString &package) const
{
    QString bashCmd;
    if (package.endsWith(".deb")) {
        bashCmd = QString("pkexec dpkg -i %1").arg(package);
    }
    else if (package.endsWith(".rpm")) {
        bashCmd = QString("pkexec rpm -i --replacefiles --replacepkgs %1").arg(package);
    }
    else if (package.endsWith(".zst")) {
        bashCmd = QString("pkexec pacman -U --noconfirm %1").arg(package);
    }
    else {
        qCDebug(LOG_AUTO_UPDATER) << "Helper_linux::installUpdate unrecognized package type:" << package;
        return std::nullopt;
    }

    qCDebug(LOG_AUTO_UPDATER) << "Installing package " << package << " with command " << bashCmd;

    int exitCode = system(bashCmd.toStdString().c_str());
    if (!WIFEXITED(exitCode) || WEXITSTATUS(exitCode) != 0) {
        qCDebug(LOG_AUTO_UPDATER) << "Install failed with exit code" << exitCode;
        return false;
    }

    return true;
}
