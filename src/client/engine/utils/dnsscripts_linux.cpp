#include "dnsscripts_linux.h"
#include "utils/log/categories.h"
#include <QProcess>
#include <QFile>

DnsScripts_linux::SCRIPT_TYPE DnsScripts_linux::dnsManager() {
    if (dnsManager_ == DNS_MANAGER_AUTOMATIC) {
        return detectScript();
    } else if (dnsManager_ == DNS_MANAGER_RESOLV_CONF) {
        return RESOLV_CONF;
    } else if (dnsManager_ == DNS_MANAGER_SYSTEMD_RESOLVED) {
        return SYSTEMD_RESOLVED;
    } else {
        return NETWORK_MANAGER;
    }
}

void DnsScripts_linux::setDnsManager(DNS_MANAGER_TYPE d)
{
    dnsManager_ = d;
}

DnsScripts_linux::DnsScripts_linux() : dnsManager_(DNS_MANAGER_AUTOMATIC)
{
}

DnsScripts_linux::SCRIPT_TYPE DnsScripts_linux::detectScript()
{
    // collecting information about DNS settings
    bool isResolvConfInstalled = false;
    QString resolvConfSymlink;
    bool isSystemdResolvedServiceRunning = false;
    QString resolvConfFileSymlink;
    QString resolvConfFileHeader;

    // check if the resolvconf utility is installed and its symlink.
    {
        QProcess process;
        process.start("resolvconf");
        process.waitForFinished();
        isResolvConfInstalled = (process.error() == QProcess::UnknownError);
        if (isResolvConfInstalled)
        {
            resolvConfSymlink = getSymlink("/sbin/resolvconf");
        }
    }

    // check if the systemd-resolved service is running.
    {
        QProcess process;
        process.startCommand("systemctl is-active --quiet systemd-resolved");
        process.waitForFinished();
        isSystemdResolvedServiceRunning = (process.exitCode() == 0);
    }

    // get a symlink to /etc/resolv.conf file
    resolvConfFileSymlink = getSymlink("/etc/resolv.conf");

    // read a header of /etc/resolv.conf file
    {
        QFile file("/etc/resolv.conf");
        if (file.open(QIODevice::ReadOnly))
        {
            QTextStream in(&file);
            while (!in.atEnd())
            {
                QString line = in.readLine();
                if (line.startsWith("#"))
                {
                    resolvConfFileHeader += line;
                }
            }
        }
        else
        {
            qCDebug(LOG_BASIC) << "Can't open /etc/resolv.conf file";
        }
    }

    qCDebug(LOG_BASIC) << "DNS-manager configuration: isResolvConfInstalled =" << isResolvConfInstalled << "; resolvConfSymlink =" << resolvConfSymlink <<
                          "; isSystemdResolvedServiceRunning =" << isSystemdResolvedServiceRunning << "; resolvConfFileSymlink =" << resolvConfFileSymlink;
    qCDebug(LOG_BASIC) << "/etc/resolv.conf header:" << resolvConfFileHeader;


    // choosing a DNS-manager method based on the collected information

    // first method: based on check that the resolvconf utility is installed and it's not a symlink to something else (for example on Fedora it's symlink to resolvectl.
    if (isResolvConfInstalled && resolvConfSymlink == "/sbin/resolvconf")
    {
        qCInfo(LOG_BASIC) << "The DNS installation method -> resolvconf";
        return RESOLV_CONF;
    }

    // second method: we check if the resolv.conf file is symlinked to determine what is being used
    // and if its symlinked to: /run/systemd/resolve/resolv.conf then we know its systemd-resolved, regardless of which package is installed.
    // also, the systemd-resolved service must be running
    if (isSystemdResolvedServiceRunning && resolvConfFileSymlink.contains("/run/systemd", Qt::CaseInsensitive))
    {
        // relevant for Fedora
        if (isResolvConfInstalled && resolvConfSymlink.contains("resolvectl", Qt::CaseInsensitive))
        {
            qCInfo(LOG_BASIC) << "The DNS installation method -> resolvconf";
            return RESOLV_CONF;
        }
        else
        {
            qCInfo(LOG_BASIC) << "The DNS installation method -> systemd-resolve";
            return SYSTEMD_RESOLVED;
        }
    }

    // by default use NetworkManager script
    qCInfo(LOG_BASIC) << "The DNS installation method -> NetworkManager";
    return NETWORK_MANAGER;
}

QString DnsScripts_linux::getSymlink(const QString &path)
{
    QProcess process;
    process.startCommand("readlink -f " + path);
    process.waitForFinished();
    return process.readAll().trimmed();
}
