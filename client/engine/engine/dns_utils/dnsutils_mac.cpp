#include "dnsutils.h"
#include "utils/logger.h"
#include "utils/macutils.h"

#include <QProcess>

namespace DnsUtils
{

std::vector<std::wstring> getOSDefaultDnsServers()
{
    std::vector<std::wstring> dnsServers;

    QSet<QString> servers = MacUtils::getOsDnsServers();
    for (const QString &s : qAsConst(servers)) {
        dnsServers.push_back(s.toStdWString());
    }

    return dnsServers;
}

}
