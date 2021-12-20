#include "dnsscripts_linux.h"
#include "utils/logger.h"
#include <QProcess>

QString DnsScripts_linux::scriptPath()
{
    lastIsResolvConf_ = isUseResolvConf(false);
    if (lastIsResolvConf_)
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
    lastIsResolvConf_ = isUseResolvConf(true);
}

bool DnsScripts_linux::isUseResolvConf(bool bForceLogging)
{
    // first method: we check if the resolv.conf file is symlinked to determine what is being used
    // and if its symlinked to: /run/systemd/resolve/resolv.conf then we know its systemd-resolved, regardless of which package is installed.
    {
        QProcess process;
        process.start("readlink -f /etc/resolv.conf");
        process.waitForFinished();

        QString reply = process.readAll();
        if (reply.trimmed().compare("/run/systemd/resolve/resolv.conf", Qt::CaseInsensitive) == 0)
        {
            // log only if last state changed or force logging flag setted
            if (bForceLogging || lastIsResolvConf_ == true)
            {
                qCDebug(LOG_BASIC) << "The DNS installation method based on readlink methos -> systemd-resolve.";
            }
            return false;
        }
        process.close();
    }

    // second method: based on check that the resolvconf utility is installed.
    {
        QProcess process;
        process.start("resolvconf");
        process.waitForFinished();
        process.close();
        bool isResolvConfInstalled = (process.error() == QProcess::UnknownError);
        if (isResolvConfInstalled)
        {
            if (bForceLogging || lastIsResolvConf_ == false)
            {
                qCDebug(LOG_BASIC) << "Resolvconf utility found. The DNS installation method -> resolvconf.";
            }
        }
        else
        {
            if (bForceLogging || lastIsResolvConf_ == true)
            {
                qCDebug(LOG_BASIC) << "Resolvconf utility not found. The DNS installation method -> systemd-resolve.";
            }
        }
        return isResolvConfInstalled;
    }
}
