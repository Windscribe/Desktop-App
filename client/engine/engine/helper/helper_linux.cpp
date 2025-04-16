#include "helper_linux.h"
#include <QProcess>
#include "utils/log/categories.h"

Helper_linux::Helper_linux(std::unique_ptr<IHelperBackend> backend) : Helper_posix(std::move(backend))
{
}

bool Helper_linux::installUpdate(const QString &package) const
{
    QProcess process;
    process.setProgram("/etc/windscribe/install-update");
    process.setArguments(QStringList() << package);
    int ret = process.startDetached();
    if (!ret) {
        qCCritical(LOG_AUTO_UPDATER) << "Install failed to start" << ret;
        return false;
    }
    return true;
}

void Helper_linux::setDnsLeakProtectEnabled(bool bEnabled)
{
    sendCommand(HelperCommand::setDnsLeakProtectEnabled, bEnabled);
}

void Helper_linux::resetMacAddresses(const QString &ignoreNetwork)
{
    sendCommand(HelperCommand::resetMacAddresses, ignoreNetwork.toStdString());
}

