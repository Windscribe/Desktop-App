#include "helper_linux.h"

#include <stdlib.h>

#include "utils/logger.h"

#include <chrono>
#include <thread>

const QString Helper_linux::WINDSCRIBE_PATH =            "/usr/local/windscribe/Windscribe";
const QString Helper_linux::USER_ENTER_PASSWORD_STRING = "Please enter admin password that is required by Windscribe VPN client.";

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
        bashCmd = QString("echo \"%1 Auto-update.\"; "
                                        "sudo dpkg -i %2; "
                                        "if [ $? -eq 0 ]; "
                                        "then (nohup %3 &) && exec bash; "
                                        "else notify-send \"Error during update.\" \"Please relaunch Windscribe and try again.\"; exec bash; "
                                        "fi;").arg(USER_ENTER_PASSWORD_STRING, package, WINDSCRIBE_PATH);
    }
    else if(package.endsWith(".rpm")) {
        bashCmd = QString("echo \"%1 Auto-update.\"; "
                                        "sudo rpm -i --replacefiles --replacepkgs %2; "
                                        "if [ $? -eq 0 ]; "
                                        "then (nohup %3 &) && exec bash; "
                                        "else notify-send \"Error during update.\" \"Please relaunch Windscribe and try again.\"; exec bash; "
                                        "fi;").arg(USER_ENTER_PASSWORD_STRING, package, WINDSCRIBE_PATH);
    }
    else {
        return false;
    }

    const QString cmd = QString("gnome-terminal -- bash -c '%1'").arg(bashCmd);
    qCDebug(LOG_BASIC) << "Installing package " << package << " with command " << cmd;
    exitCode = system(cmd.toStdString().c_str());
    if (!WIFEXITED(exitCode) || WEXITSTATUS(exitCode) != 0) {
        return false;
    }
    else
        return true;
}


