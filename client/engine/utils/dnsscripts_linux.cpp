#include "dnsscripts_linux.h"
#include "utils/logger.h"
#include <QProcess>

QString DnsScripts_linux::scriptPath()
{
    if (dnsManager_ == ProtoTypes::DNS_MANAGER_AUTOMATIC)
    {
        SCRIPT_TYPE scriptType = detectScript();
        if (scriptType == SYSTEMD_RESOLVED)
        {
            return "/etc/windscribe/update-systemd-resolved";
        }
        else if (scriptType == RESOLV_CONF)
        {
            return "/etc/windscribe/update-resolv-conf";
        }
        else if (scriptType == NETWORK_MANAGER)
        {
            return "/etc/windscribe/update-network-manager";
        }
        else
        {
            Q_ASSERT(false);
            return "";
        }
    }
    else if (dnsManager_ == ProtoTypes::DNS_MANAGER_RESOLV_CONF)
    {
        return "/etc/windscribe/update-resolv-conf";
    }
    else if (dnsManager_ == ProtoTypes::DNS_MANAGER_SYSTEMD_RESOLVED)
    {
        return "/etc/windscribe/update-systemd-resolved";
    }
    else if (dnsManager_ == ProtoTypes::DNS_MANAGER_NETWORK_MANAGER)
    {
        return "/etc/windscribe/update-network-manager";
    }
    else
    {
        Q_ASSERT(false);
        return "";
    }
}

void DnsScripts_linux::setDnsManager(ProtoTypes::DnsManagerType d)
{
    dnsManager_ = d;
}

DnsScripts_linux::DnsScripts_linux() : dnsManager_(ProtoTypes::DNS_MANAGER_AUTOMATIC)
{
}

DnsScripts_linux::SCRIPT_TYPE DnsScripts_linux::detectScript()
{
    // first method: based on check that the resolvconf utility is installed.
    {
        QProcess process;
        process.start("resolvconf");
        process.waitForFinished();
        process.close();
        bool isResolvConfInstalled = (process.error() == QProcess::UnknownError);
        if (isResolvConfInstalled)
        {
            qCDebug(LOG_BASIC) << "The DNS installation method -> resolvconf";
            return RESOLV_CONF;
        }
    }

    // second method: we check if the resolv.conf file is symlinked to determine what is being used
    // and if its symlinked to: /run/systemd/resolve/resolv.conf then we know its systemd-resolved, regardless of which package is installed.
    {
        QProcess process;
        process.start("readlink -f /etc/resolv.conf");
        process.waitForFinished();

        QString reply = process.readAll();
        if (reply.trimmed().contains("/run/systemd", Qt::CaseInsensitive))
        {
            qCDebug(LOG_BASIC) << "The DNS installation method -> systemd-resolve";
            return SYSTEMD_RESOLVED;
        }
    }

    // by default use NetworkManager script
    qCDebug(LOG_BASIC) << "The DNS installation method -> NetworkManager";
    return NETWORK_MANAGER;
}
