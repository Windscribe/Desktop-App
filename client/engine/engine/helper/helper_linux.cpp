#include "helper_linux.h"

#include <stdlib.h>

#include "../../../../backend/posix_common/helper_commands_serialize.h"
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

std::optional<bool> Helper_linux::installUpdate(const QString &package) const
{
    QString bashCmd;
    if (package.endsWith(".deb")) {
        bashCmd = QString("pkexec apt install %1").arg(package);
    }
    else if (package.endsWith(".rpm")) {
        bashCmd = QString("pkexec dnf upgrade -y %1").arg(package);
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

bool Helper_linux::setDnsLeakProtectEnabled(bool bEnabled)
{
    QMutexLocker locker(&mutex_);

    CMD_ANSWER answer;
    CMD_SET_DNS_LEAK_PROTECT_ENABLED cmd;
    cmd.enabled = bEnabled;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_DNS_LEAK_PROTECT_ENABLED, stream.str(), answer);
}

bool Helper_linux::checkForWireGuardKernelModule()
{
    QMutexLocker locker(&mutex_);

    CMD_ANSWER answer;
    return runCommand(HELPER_CMD_CHECK_FOR_WIREGUARD_KERNEL_MODULE, {}, answer) && answer.executed == 1;
}
