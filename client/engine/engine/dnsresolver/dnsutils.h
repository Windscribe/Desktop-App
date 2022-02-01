#ifndef DNSUTILS_H
#define DNSUTILS_H

#include <QStringList>
#include <string>
#include <vector>

namespace DnsUtils
{
    std::vector<std::wstring> getOSDefaultDnsServers();
}

#endif // DNSUTILS_H
