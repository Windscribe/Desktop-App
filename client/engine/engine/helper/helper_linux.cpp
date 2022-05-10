#include "helper_linux.h"

#include <stdlib.h>

#include "utils/logger.h"

#include <chrono>
#include <thread>

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

bool Helper_linux::installUpdate(const QString &package) const
{
    int exitCode = EXIT_SUCCESS;
    QString bashCmd;
    if(package.endsWith(".deb")) {
        bashCmd = QString("pkexec dpkg -i %1").arg(package);
    }
    else if(package.endsWith(".rpm")) {
        bashCmd = QString("pkexec rpm -i --replacefiles --replacepkgs %1").arg(package);
    }
    else {
        return false;
    }

    qCDebug(LOG_BASIC) << "Installing package " << package << " with command " << bashCmd;
    exitCode = system(bashCmd.toStdString().c_str());
    if (!WIFEXITED(exitCode) || WEXITSTATUS(exitCode) != 0)
    {
        qCDebug(LOG_BASIC) << "Install failed";
        bashCmd = QString("notify-send \"Error during update.\" \"Please relaunch Windscribe and try again.\"").arg(package);
        exitCode = system(bashCmd.toStdString().c_str());
        return false;
    }
    else
        return true;
}


