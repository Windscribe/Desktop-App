#include "helper_linux.h"

#include <QProcess>
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
    QProcess process;
    process.setProgram("/etc/windscribe/install-update");
    process.setArguments(QStringList() << package);
    int ret = process.startDetached();
    if (!ret) {
        qCDebug(LOG_AUTO_UPDATER) << "Install failed to start" << ret;
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

bool Helper_linux::resetMacAddresses(const QString &ignoreNetwork)
{
    QMutexLocker locker(&mutex_);

    CMD_ANSWER answer;
    CMD_RESET_MAC_ADDRESSES cmd;
    cmd.ignoreNetwork = ignoreNetwork.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_RESET_MAC_ADDRESSES, stream.str(), answer) && answer.executed;
}

