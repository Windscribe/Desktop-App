#include "connectionattemptstrategyfactory.h"

#include "autoconnectionattemptstrategy.h"
#include "customconfigconnectionattemptstrategy.h"
#include "engine/connectionmanager/connectors/iconnectionplatformpolicy.h"
#include "engine/locationsmodel/mutablelocationinfo.h"
#include "manualconnectionattemptstrategy.h"
#include "utils/hydraprofile.h"
#include "utils/log/categories.h"

namespace {

// Whether a hydra front stands in for this location's datacenter, matched by the datacenter's id.
bool isHydraFronted(const QSharedPointer<locationsmodel::BaseLocationInfo> &bli)
{
    if (bli->locationId().isStaticIpsLocation()) {
        return false;
    }
    const auto li = qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli);
    if (li.isNull()) {
        return false;
    }
    const auto hydra = HydraProfile::load();
    return hydra && hydra->frontForDatacenter(li->getDatacenterId()) != nullptr;
}

}  // namespace

IConnectionAttemptStrategy *ConnectionAttemptStrategyFactory::createStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                                                const types::ConnectionSettings &connectionSettings,
                                                                const api_responses::PortMap &portMap,
                                                                const types::ProxySettings &proxySettings,
                                                                const QString &preferredNodeHostname,
                                                                bool isFirewallAlwaysOnPlusEnabled,
                                                                bool hasUsableCachedWgConfig,
                                                                bool isLockdownMode)
{
    IConnectionAttemptStrategy *strategy = nullptr;
    if (bli->locationId().isCustomConfigsLocation()) {
        strategy = new CustomConfigConnectionAttemptStrategy(bli);
    } else if (isHydraFronted(bli)) {
        // Fail closed: no other protocol and no direct fallback, in automatic mode or under Always On+. Nodes
        // are picked and retried as usual; every one of them is dialled through the front.
        if (!connectionSettings.isAutomatic() && !connectionSettings.protocol().isWireGuardProtocol()) {
            qCInfo(LOG_CONNECTION) << "A hydra front stands in for this location's datacenter, and the protocol isn't WireGuard: not connecting";
            strategy = new ManualConnectionAttemptStrategy(bli, connectionSettings, portMap, preferredNodeHostname,
                                                           ManualConnectionAttemptStrategy::HydraLock::kRefused);
        } else {
            types::ConnectionSettings wireGuard = connectionSettings;
            if (connectionSettings.isAutomatic()) {
                // The front replaces the port, so any WireGuard port the node offers will do.
                const api_responses::PortItem *it = portMap.getPortItemByProtocolType(types::Protocol::WIREGUARD);
                wireGuard = types::ConnectionSettings(types::Protocol::WIREGUARD, it && !it->ports.isEmpty() ? it->ports[0] : 443, false);
            }
            strategy = new ManualConnectionAttemptStrategy(bli, wireGuard, portMap, preferredNodeHostname,
                                                           ManualConnectionAttemptStrategy::HydraLock::kThroughFront);
        }
    } else if (connectionSettings.isAutomatic()) {
        // Under Always On+ we can only attempt WireGuard from a usable cached config; otherwise skip it.
        const bool skipWireguardProtocol = isFirewallAlwaysOnPlusEnabled && !hasUsableCachedWgConfig;
        strategy = new AutoConnectionAttemptStrategy(bli, portMap, proxySettings.isProxyEnabled(), isLockdownMode, skipWireguardProtocol, preferredNodeHostname);
    } else {
        // Manual mode is not a fallback chain: substitute WG (->IKEv2/OpenVPN UDP) only when it can't
        // be attempted at all (no usable cached config under Always On+); otherwise attempt WG as-is.
        types::ConnectionSettings overrideConnectionSettings = connectionSettings;
        if (isFirewallAlwaysOnPlusEnabled && connectionSettings.protocol().isWireGuardProtocol() && !hasUsableCachedWgConfig) {
            // A substituted IKEv2 would hard-fail the lockdown gate at attempt time; fall through to UDP.
            // Likewise on platforms without IKEv2 support, where ConnectionSettings would silently revert it to WireGuard.
            const api_responses::PortItem *it = nullptr;
            if (types::Protocol::supportedProtocols().contains(types::Protocol(types::Protocol::IKEV2)) &&
                !lockdownBlocksProtocol(isLockdownMode, types::Protocol::IKEV2)) {
                it = portMap.getPortItemByProtocolType(types::Protocol::IKEV2);
            }
            // if no ikev2 in available protocol list then try UDP
            if (!it) {
                it = portMap.getPortItemByProtocolType(types::Protocol::OPENVPN_UDP);
            }
            if (it) {
                // select the first port in the list
                if (it->ports.size() > 0) {
                    overrideConnectionSettings = types::ConnectionSettings(it->protocol, it->ports[0], false);
                }
            }
        }
        strategy = new ManualConnectionAttemptStrategy(bli, overrideConnectionSettings, portMap, preferredNodeHostname);
    }

    // The custom-config strategy never consults the cached-config advice (CM's node-type guard
    // bypasses the Always On+ gate); don't seed state that looks load-bearing.
    if (!bli->locationId().isCustomConfigsLocation()) {
        strategy->setCachedConfigAvailability(hasUsableCachedWgConfig);
    }
    return strategy;
}
