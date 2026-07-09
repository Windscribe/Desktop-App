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

                info.addAdapterIp(findAddressOfFamily(aa->FirstUnicastAddress, AF_INET));
                info.addAdapterIp(findAddressOfFamily(aa->FirstUnicastAddress, AF_INET6));
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
