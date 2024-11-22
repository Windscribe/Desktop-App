#include "dnsutils.h"
#include <QSet>
#include "utils/macutils.h"

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
