#include "dnsutils.h"
#include <QSet>
#include "utils/log/debug_trace.h"
#include "utils/macutils.h"

namespace DnsUtils
{

std::vector<std::wstring> getOSDefaultDnsServers()
{
    WS_DEBUG_TRACE_SCOPE("DnsUtils::getOSDefaultDnsServers");
    std::vector<std::wstring> dnsServers;

    QSet<QString> servers = MacUtils::getOsDnsServers();
    for (const QString &s : std::as_const(servers)) {
        dnsServers.push_back(s.toStdWString());
    }

    return dnsServers;
}

}
