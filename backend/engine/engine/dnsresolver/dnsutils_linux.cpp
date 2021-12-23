#include "dnsutils.h"
#include "utils/logger.h"

#include <QProcess>

namespace DnsUtils
{

std::vector<std::wstring> getOSDefaultDnsServers()
{
    std::vector<std::wstring> dnsServers;


    QString strReply;
    FILE *file = popen("(nmcli dev list || nmcli dev show ) 2>/dev/null | grep DNS", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }

    if (strReply.isEmpty())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Can't get OS default DNS list: probably the nmcli utility (network-manager package) is not installed";
        return dnsServers;
    }

    const QStringList lines = strReply.split('\n', QString::SkipEmptyParts);
    qCDebug(LOG_FIREWALL_CONTROLLER) << "Get OS default DNS list:" << lines;
    for (auto &it : lines)
    {
        const QStringList pars = it.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (pars.size() == 2)
        {
            dnsServers.push_back(pars[1].toStdWString());
        }
    }

    return dnsServers;
}

}
