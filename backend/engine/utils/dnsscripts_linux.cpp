#include "dnsscripts_linux.h"
#include "utils/logger.h"
#include <QProcess>

QString DnsScripts_linux::upScriptPath() const
{
    return "/etc/windscribe/update-resolv-conf";

}

QString DnsScripts_linux::downScriptPath() const
{
    return "/etc/windscribe/update-resolv-conf";
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
