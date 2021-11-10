#include "dnsscripts_linux.h"
#include "utils/logger.h"
#include <QProcess>

QString DnsScripts_linux::scriptPath() const
{
    if (isResolvConfInstalled_)
    {
        return "/etc/windscribe/update-resolv-conf";
    }
    else
    {
        return "/etc/windscribe/update-systemd-resolved";
    }
}

DnsScripts_linux::DnsScripts_linux()
{
    QProcess process;
    process.start("resolvconf");
    process.waitForFinished();
    process.close();
    isResolvConfInstalled_ = (process.error() == QProcess::UnknownError);
    if (isResolvConfInstalled_)
    {
        qCDebug(LOG_BASIC) << "Resolvconf utility found. The DNS installation method -> resolvconf.";
    }
    else
    {
        qCDebug(LOG_BASIC) << "Resolvconf utility not found. The DNS installation method -> systemd-resolve.";
    }
}
