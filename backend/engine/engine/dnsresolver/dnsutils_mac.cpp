#include "dnsutils.h"
#include "Utils/logger.h"

#include <QProcess>


namespace DnsUtils
{

std::vector<std::wstring> getDnsServers()
{
    std::vector<std::wstring> dnsServers;

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("scutil", QStringList() << "--dns");
    process.waitForFinished(-1);
    QString strAnswer = QString::fromStdString((const char *)process.readAll().data()).toLower();

    QStringList lines = strAnswer.split("\n");

    QSet<QString> ips;
    Q_FOREACH(const QString &line, lines)
    {
        if (line.contains("nameserver"))
        {
            // output example
            // nameserver[0] : 192.168.1.1
            QStringList ss = line.split(":");
            if (ss.count() == 2)
            {
                ips.insert(ss[1].trimmed());
            }
        }
    }

    qCDebug(LOG_FIREWALL_CONTROLLER) << "Get OS default DNS list:" << ips;

    Q_FOREACH(const QString &s, ips)
    {
        dnsServers.push_back(s.toStdWString());
    }

    return dnsServers;
}

}
