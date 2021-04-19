#ifndef DNSUTILS_H
#define DNSUTILS_H

#include <QStringList>
#include <string>
#include <vector>
#include "engine/types/types.h"

namespace DnsUtils
{
    std::vector<std::wstring> getOSDefaultDnsServers();
    QStringList dnsPolicyTypeToStringList(DNS_POLICY_TYPE dnsPolicyType);
}

#endif // DNSUTILS_H
