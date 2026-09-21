#include "dnsscripts_linux.h"

#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

#include "utils/log/categories.h"

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
    QString resolvConfText;

    // check if the resolvconf utility is installed and resolve its real target
    {
        const QString resolvconfPath = QStandardPaths::findExecutable("resolvconf");
        isResolvConfInstalled = !resolvconfPath.isEmpty();
        if (isResolvConfInstalled) {
            resolvConfSymlink = getSymlink(resolvconfPath);
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
        if (file.open(QIODevice::ReadOnly)) {
            resolvConfText = QString::fromUtf8(file.readAll());
            QTextStream in(&resolvConfText);
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.startsWith("#")) {
                    resolvConfFileHeader += line;
                }
            }
        } else {
            qCDebug(LOG_BASIC) << "Can't open /etc/resolv.conf file";
        }
    }

    qCDebug(LOG_BASIC) << "DNS-manager configuration: isResolvConfInstalled =" << isResolvConfInstalled << "; resolvConfSymlink =" << resolvConfSymlink <<
                          "; isSystemdResolvedServiceRunning =" << isSystemdResolvedServiceRunning << "; resolvConfFileSymlink =" << resolvConfFileSymlink;
    qCDebug(LOG_BASIC) << "/etc/resolv.conf header:" << resolvConfFileHeader;

    return selectScript(isSystemdResolvedServiceRunning, isResolvConfInstalled, resolvConfSymlink,
                        resolvConfFileSymlink, resolvConfText);
}

DnsScripts_linux::SCRIPT_TYPE DnsScripts_linux::selectScript(bool serviceRunning, bool resolvconfInstalled,
                                                           const QString &resolvconfTarget, const QString &resolvConfTarget,
                                                           const QString &resolvConfText)
{
    static const QRegularExpression stubNameserver(
        QStringLiteral("^[\\t ]*nameserver[\\t ]+127\\.0\\.0\\.(?:53|54)(?=[\\t \\r]*(?:[#;]|$))"),
        QRegularExpression::MultilineOption);
    static const QRegularExpression nameserverLine(
        QStringLiteral("^[\\t ]*nameserver(?=[\\t \\r]|$)[^\\r\\n]*"), QRegularExpression::MultilineOption);
    bool onlyResolvedStubs = false;
    auto nameservers = nameserverLine.globalMatch(resolvConfText);
    while (nameservers.hasNext()) {
        onlyResolvedStubs = stubNameserver.match(nameservers.next().captured()).hasMatch();
        if (!onlyResolvedStubs) {
            break;
        }
    }
    const bool usesResolved = resolvConfTarget == "/run/systemd/resolve/stub-resolv.conf"
        || resolvConfTarget == "/run/systemd/resolve/resolv.conf"
        || resolvConfTarget == "/usr/lib/systemd/resolv.conf"
        || resolvConfTarget == "/lib/systemd/resolv.conf"
        || onlyResolvedStubs;

    // The stub can be configured in a regular file, independently of the installed resolvconf package.
    // Only choose the direct resolved script when its service is running.
    if (serviceRunning && usesResolved) {
        qCInfo(LOG_BASIC) << "The DNS installation method -> systemd-resolved";
        return SYSTEMD_RESOLVED;
    }

    // The resolvectl compatibility command needs resolved configuration; standalone resolvconf does not.
    if (resolvconfInstalled && (!resolvconfTarget.endsWith("resolvectl") || usesResolved)) {
        qCInfo(LOG_BASIC) << "The DNS installation method -> resolvconf";
        return RESOLV_CONF;
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
