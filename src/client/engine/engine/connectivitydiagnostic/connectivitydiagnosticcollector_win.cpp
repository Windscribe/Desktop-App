#include "connectivitydiagnosticcollector_win.h"

#include "connectivitydiagnosticcollector_common.h"
#include "connectivitydiagnosticresult.h"
#include "engine/firewall/firewallcontroller.h"
#include "types/enums.h"
#include "types/networkinterface.h"
#include "utils/network_utils/network_utils_win.h"

using connectivity_diagnostic_collector::determineProxyMode;
using connectivity_diagnostic_collector::interfaceTypeToken;

ConnectivityDiagnosticCollector_win::ConnectivityDiagnosticCollector_win(QObject *parent, FirewallController *firewallController)
    : IConnectivityDiagnosticCollector(parent), firewallController_(firewallController)
{
}

ConnectivityDiagnosticCollector_win::~ConnectivityDiagnosticCollector_win()
{
}

void ConnectivityDiagnosticCollector_win::collectLocalSnapshot(ConnectivityDiagnosticResult &result)
{
    // Online predicates, kept separate. haveActiveInterface() is a definite bool;
    // haveInternetConnectivity() may be unknown (NLM unavailable) -> left as nullopt.
    result.haveActiveInterface = NetworkUtils_win::haveActiveInterface();
    result.haveInternetConnectivity = NetworkUtils_win::haveInternetConnectivity();

    // Default-route presence per family (nullopt if the routing table is unreadable).
    result.haveDefaultRouteV4 = NetworkUtils_win::haveDefaultRouteV4();
    result.haveDefaultRouteV6 = NetworkUtils_win::haveDefaultRouteV6();

    // Selected interface: media type / index, plus address-family presence. We
    // never read the adapter name, network name or SSID (privacy).
    const types::NetworkInterface iface = NetworkUtils_win::currentNetworkInterface();
    if (!iface.isNoNetworkInterface()) {
        result.selectedInterfaceType = interfaceTypeToken(iface.interfaceType);
        result.selectedInterfaceIndex = iface.interfaceIndex;

        bool hasIpv4 = false;
        bool hasGlobalIpv6 = false;
        NetworkUtils_win::interfaceAddressPresence(static_cast<unsigned long>(iface.interfaceIndex), hasIpv4, hasGlobalIpv6);
        result.selectedInterfaceHasIpv4 = hasIpv4;
        result.selectedInterfaceHasGlobalIpv6 = hasGlobalIpv6;
    } else {
        result.selectedInterfaceType = interfaceTypeToken(NETWORK_INTERFACE_NONE);
    }

    // Proxy mode (redacted) and Windscribe firewall state (our WFP sublayer only).
    result.proxyMode = determineProxyMode();
    if (firewallController_ != nullptr) {
        result.windscribeFirewallOn = firewallController_->firewallActualState();
    }
}
