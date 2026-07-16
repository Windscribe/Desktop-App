#pragma once

#include <string>
#include <vector>

#include <QtGlobal>

#if defined(Q_OS_LINUX)
#include <QString>
#include <QStringList>
#endif

namespace DnsUtils
{
    std::vector<std::wstring> getOSDefaultDnsServers();

#if defined(Q_OS_LINUX)
    // Records the currently-connected VPN tunnel's interface name and the DNS servers it pushed, so
    // getOSDefaultDnsServers() can scope them out and still return the physical resolvers even while
    // connected (the firewall exceptions, including the boot ruleset which may be written while
    // connected, must allowlist the real OS resolvers, never the tunnel's). Pass an empty interface and
    // list to clear on disconnect.
    void setVpnContext(const QString &vpnInterfaceName, const QStringList &tunnelDnsServers);
#endif
}
