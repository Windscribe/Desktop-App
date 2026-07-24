#include "adapterutils_win.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "utils/log/categories.h"

namespace {

types::IpAddress sockaddrToIpAddress(const SOCKADDR *addr)
{
    char buf[INET6_ADDRSTRLEN];
    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in *>(addr)->sin_addr, buf, sizeof(buf));
        return types::IpAddress(std::string(buf));
    } else if (addr->sa_family == AF_INET6) {
        inet_ntop(AF_INET6, &reinterpret_cast<const sockaddr_in6 *>(addr)->sin6_addr, buf, sizeof(buf));
        return types::IpAddress(std::string(buf));
    }
    return types::IpAddress();
}

template <typename AddrEntry>
types::IpAddress findAddressOfFamily(AddrEntry *head, int family)
{
    for (auto *entry = head; entry; entry = entry->Next) {
        if (entry->Address.lpSockaddr->sa_family == family)
            return sockaddrToIpAddress(entry->Address.lpSockaddr);
    }
    return types::IpAddress();
}

// Usable as the adapter's own v6 address: valid, v6, and not link-local (see the comment
// on findNonLinkLocalV6Address for why fe80::/10 must be skipped).
bool isUsableAdapterV6(const types::IpAddress &ip)
{
    if (!ip.isValid() || !ip.isV6())
        return false;
    const uint8_t *b = ip.bytes();
    return !(b[0] == 0xfe && (b[1] & 0xc0) == 0x80);   // fe80::/10
}

// Adapter (unicast) IPv6 selection deliberately skips link-locals. Windows auto-assigns an
// fe80::/10 to every interface with the IPv6 protocol bound, even when no router offers
// IPv6, so "first v6 unicast" on a v4-only network is deterministically fe80. Every
// consumer of adapterIpV6 needs an address usable with global destinations: the withV6
// gating and the split tunnel driver's redirect target (CalloutFilter — redirecting an
// excluded app's source to fe80 blackholes it instead of cleanly blocking v6), bound ::/0
// route next-hops (RoutesManager), and the "IPv6 on default adapter" connect log. An
// adapter whose only v6 is link-local must therefore report no v6 at all.
// Gateways intentionally keep findAddressOfFamily: a v6 default gateway from RA is
// legitimately the router's link-local.
template <typename AddrEntry>
types::IpAddress findNonLinkLocalV6Address(AddrEntry *head)
{
    for (auto *entry = head; entry; entry = entry->Next) {
        if (entry->Address.lpSockaddr->sa_family != AF_INET6)
            continue;
        const types::IpAddress ip = sockaddrToIpAddress(entry->Address.lpSockaddr);
        if (isUsableAdapterV6(ip))
            return ip;
    }
    return types::IpAddress();
}

} // namespace

AdapterGatewayInfo AdapterUtils_win::getDefaultAdapterInfo()
{
    AdapterGatewayInfo info;
    DWORD  bestIfIndex;
    IPAddr ipAddr = htonl(0x08080808); // "8.8.8.8" is well-known Google DNS Server IP.
    if (GetBestInterface(ipAddr, &bestIfIndex) == NO_ERROR)
    {
        info = getAdapterInfo(true, bestIfIndex, QString());
    }
    return info;
}

AdapterGatewayInfo AdapterUtils_win::getConnectedAdapterInfo(const QString &adapterIdentifier)
{
    return getAdapterInfo(false, 0, adapterIdentifier);
}

AdapterGatewayInfo AdapterUtils_win::getAdapterInfo(bool byIfIndex, unsigned long ifIndex, const QString &adapterIdentifier)
{
    ULONG sz = sizeof(IP_ADAPTER_ADDRESSES_LH) * 32;
    QByteArray arr(sz, Qt::Uninitialized);

    const ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, (PIP_ADAPTER_ADDRESSES)arr.data(), &sz);
    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        arr.resize(sz);
        ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, (PIP_ADAPTER_ADDRESSES)arr.data(), &sz);
    }

    if (ret != ERROR_SUCCESS)
    {
        return AdapterGatewayInfo();
    }

    AdapterGatewayInfo info;

    IP_ADAPTER_ADDRESSES_LH *aa = (IP_ADAPTER_ADDRESSES_LH *)arr.data();
    while (aa)
    {
        if (aa->OperStatus == IfOperStatusUp)
        {
            if ((byIfIndex && aa->IfIndex == ifIndex) || (!byIfIndex && (adapterIdentifier.compare(aa->FriendlyName) == 0)))
            {
                info.setIfIndex(aa->IfIndex);
                info.setAdapterName(QString::fromUtf16((const char16_t *)aa->Description));

                // Dump every unicast address: only one address per family is picked into
                // info below (first v4; first non-link-local v6), and when an adapter has
                // several v6 addresses (e.g. temporary + permanent + link-local) it matters
                // for leak diagnostics which one was chosen.
                {
                    QStringList unicastAddrs;
                    for (auto *ua = aa->FirstUnicastAddress; ua; ua = ua->Next) {
                        const types::IpAddress ip = sockaddrToIpAddress(ua->Address.lpSockaddr);
                        if (ip.isValid())
                            unicastAddrs << QString::fromStdString(ip.toString());
                    }
                    qCDebug(LOG_CONNECTION) << "AdapterUtils_win::getAdapterInfo: adapter" << info.adapterName()
                                            << "ifIndex" << aa->IfIndex
                                            << "unicast addresses:" << unicastAddrs.join(", ");
                }

                info.addAdapterIp(findAddressOfFamily(aa->FirstUnicastAddress, AF_INET));
                info.addAdapterIp(findNonLinkLocalV6Address(aa->FirstUnicastAddress));
                info.addGatewayIp(findAddressOfFamily(aa->FirstGatewayAddress, AF_INET));
                info.addGatewayIp(findAddressOfFamily(aa->FirstGatewayAddress, AF_INET6));

                IP_ADAPTER_DNS_SERVER_ADDRESS_XP *dns_address = aa->FirstDnsServerAddress;
                while (dns_address)
                {
                    types::IpAddress dnsIp = sockaddrToIpAddress(dns_address->Address.lpSockaddr);
                    if (dnsIp.isValid())
                        info.addDnsServer(dnsIp);
                    else
                        qCWarning(LOG_CONNECTION) << "AdapterUtils_win::getAdapterInfo - failed to parse DNS server address";

                    dns_address = dns_address->Next;
                }
                break;
            }
        }

        aa = aa->Next;
    }

    return info;
}
