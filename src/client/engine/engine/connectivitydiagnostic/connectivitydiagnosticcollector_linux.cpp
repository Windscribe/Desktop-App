#include "connectivitydiagnosticcollector_linux.h"

#include <QByteArray>
#include <QScopeGuard>
#include <QString>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/wireless.h>

#include "connectivitydiagnosticcollector_common.h"
#include "connectivitydiagnosticresult.h"
#include "engine/firewall/firewallcontroller.h"
#include "types/enums.h"
#include "utils/network_utils/network_utils_linux.h"

using connectivity_diagnostic_collector::determineProxyMode;
using connectivity_diagnostic_collector::interfaceTypeToken;

namespace {

// True if the interface carries the wireless extensions (SIOCGIWNAME succeeds).
// Used only to classify the selected interface as wifi vs eth.
bool isWirelessInterface(const QString &ifname)
{
    if (ifname.isEmpty())
        return false;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return false;
    auto guard = qScopeGuard([&] { close(fd); });

    struct iwreq pwrq;
    memset(&pwrq, 0, sizeof(pwrq));
    const QByteArray nameBytes = ifname.toUtf8();
    strncpy(pwrq.ifr_name, nameBytes.constData(), IFNAMSIZ - 1);
    return ioctl(fd, SIOCGIWNAME, &pwrq) != -1;
}

// Fills hasIpv4 / hasGlobalIpv6 for a single interface by name. A global IPv6 is
// any non-link-local, non-loopback, non-multicast, non-unspecified address. Only
// counts addresses on an interface that is both IFF_UP and IFF_RUNNING so a stale
// admin-up-but-link-down interface is not reported as having connectivity.
void interfaceAddressPresence(const QString &ifname, bool &hasIpv4, bool &hasGlobalIpv6)
{
    hasIpv4 = false;
    hasGlobalIpv6 = false;

    if (ifname.isEmpty())
        return;

    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
        return;
    auto guard = qScopeGuard([&] { freeifaddrs(ifaddr); });

    const QByteArray nameBytes = ifname.toUtf8();
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        if (qstrcmp(ifa->ifa_name, nameBytes.constData()) != 0)
            continue;
        if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING))
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            hasIpv4 = true;
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            const sockaddr_in6 *addrIn = reinterpret_cast<const sockaddr_in6 *>(ifa->ifa_addr);
            if (!IN6_IS_ADDR_LINKLOCAL(&addrIn->sin6_addr) &&
                !IN6_IS_ADDR_LOOPBACK(&addrIn->sin6_addr) &&
                !IN6_IS_ADDR_MULTICAST(&addrIn->sin6_addr) &&
                !IN6_IS_ADDR_UNSPECIFIED(&addrIn->sin6_addr)) {
                hasGlobalIpv6 = true;
            }
        }
    }
}

// True if any non-loopback interface is both IFF_UP and IFF_RUNNING (link up).
// This is the Linux analogue of the Windows "media connected" predicate and is
// kept separate from default-route presence, which is reported on its own.
bool haveAnyActiveInterface()
{
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
        return false;
    auto guard = qScopeGuard([&] { freeifaddrs(ifaddr); });

    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        if (ifa->ifa_flags & IFF_LOOPBACK)
            continue;
        if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING))
            return true;
    }
    return false;
}

} // namespace

ConnectivityDiagnosticCollector_linux::ConnectivityDiagnosticCollector_linux(QObject *parent, FirewallController *firewallController)
    : IConnectivityDiagnosticCollector(parent), firewallController_(firewallController)
{
}

ConnectivityDiagnosticCollector_linux::~ConnectivityDiagnosticCollector_linux()
{
}

void ConnectivityDiagnosticCollector_linux::collectLocalSnapshot(ConnectivityDiagnosticResult &result)
{
    // Online predicates, kept separate. haveActiveInterface() is a definite bool
    // derived from link state. Linux has no purely-local internet-connectivity
    // oracle (no NLM equivalent), so haveInternetConnectivity is left as nullopt
    // (unknown); default-route presence below is reported on its own instead.
    result.haveActiveInterface = haveAnyActiveInterface();

    // Default-route presence per family. Includes any ::/0 or 0.0.0.0/0 route
    // (VPN tun routes are not excluded, so the real routing state is reported).
    QString gatewayV4, interfaceV4, adapterIpV4;
    NetworkUtils_linux::getDefaultRoute(gatewayV4, interfaceV4, adapterIpV4, false);
    result.haveDefaultRouteV4 = !interfaceV4.isEmpty();

    QString gatewayV6, interfaceV6, adapterIpV6;
    NetworkUtils_linux::getDefaultRouteV6(gatewayV6, interfaceV6, adapterIpV6, false);
    result.haveDefaultRouteV6 = !interfaceV6.isEmpty();

    // Selected interface: media type / index, plus address-family presence. Use
    // the v4 default-route interface, falling back to the v6 one. We never read
    // the network name or SSID (privacy).
    const QString selectedName = !interfaceV4.isEmpty() ? interfaceV4 : interfaceV6;
    if (!selectedName.isEmpty()) {
        result.selectedInterfaceType = interfaceTypeToken(isWirelessInterface(selectedName) ? NETWORK_INTERFACE_WIFI
                                                                                            : NETWORK_INTERFACE_ETH);
        const unsigned int ifIndex = if_nametoindex(selectedName.toUtf8().constData());
        result.selectedInterfaceIndex = (ifIndex == 0) ? -1 : static_cast<int>(ifIndex);

        bool hasIpv4 = false;
        bool hasGlobalIpv6 = false;
        interfaceAddressPresence(selectedName, hasIpv4, hasGlobalIpv6);
        result.selectedInterfaceHasIpv4 = hasIpv4;
        result.selectedInterfaceHasGlobalIpv6 = hasGlobalIpv6;
    } else {
        result.selectedInterfaceType = interfaceTypeToken(NETWORK_INTERFACE_NONE);
    }

    // Proxy mode (redacted) and Windscribe firewall state (our iptables rules).
    result.proxyMode = determineProxyMode();
    if (firewallController_ != nullptr) {
        result.windscribeFirewallOn = firewallController_->firewallActualState();
    }
}
