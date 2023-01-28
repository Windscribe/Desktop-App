#include "dnsutils.h"
#include "utils/logger.h"

#include <QProcess>
#include <QRegularExpression>

namespace DnsUtils
{

enum DETECT_METHOD_TYPE { UNKNOWN_METHOD, RESOLVECTL_METHOD, NMCLI_METHOD };
DETECT_METHOD_TYPE methodType = UNKNOWN_METHOD;


std::vector<std::wstring> getOSDefaultDnsServers_NMCLI()
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

    const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);
    qCDebug(LOG_FIREWALL_CONTROLLER) << "Get OS default DNS list (nmcli output):" << lines;
    for (auto &it : lines)
    {
        const QStringList pars = it.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (pars.size() == 2)
        {
            dnsServers.push_back(pars[1].toStdWString());
        }
    }

    return dnsServers;
}

std::vector<std::wstring> getOSDefaultDnsServers_Resolvectl()
{
    std::vector<std::wstring> dnsServers;

    QString strReply;
    FILE *file = popen("resolvectl dns", "r");
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
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Can't get OS default DNS list: resolvectl return empty string";
        return dnsServers;
    }

    const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);
    qCDebug(LOG_FIREWALL_CONTROLLER) << "Get OS default DNS list (resolvectl output):" << lines;
    for (auto &it : lines)
    {
        const QStringList pars = it.split(":", Qt::SkipEmptyParts);
        if (pars.size() == 2)
        {
            if (!pars[0].contains("(utun") && !pars[0].contains("(tun"))
            {
                dnsServers.push_back(pars[1].trimmed().toStdWString());
            }
        }
    }

    return dnsServers;
}


std::vector<std::wstring> getOSDefaultDnsServers()
{
    if (methodType == UNKNOWN_METHOD)
    {
        // check if the resolvectl utility is installed
        QProcess process;
        process.start("resolvectl");
        process.waitForFinished();
        bool isResolvectlInstalled = (process.error() == QProcess::UnknownError);
        methodType = isResolvectlInstalled ? RESOLVECTL_METHOD : NMCLI_METHOD;
    }

    if (methodType == RESOLVECTL_METHOD)
    {
        return getOSDefaultDnsServers_Resolvectl();
    }
    else if (methodType == NMCLI_METHOD)
    {
        return getOSDefaultDnsServers_NMCLI();
    }
    else
    {
        Q_ASSERT(false);
        return std::vector<std::wstring>();
    }
}

}
