#include "connsettingspolicyfactory.h"

#include "autoconnsettingspolicy.h"
#include "customconfigconnsettingspolicy.h"
#include "manualconnsettingspolicy.h"

BaseConnSettingsPolicy *ConnSettingsPolicyFactory::createPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                                                const types::ConnectionSettings &connectionSettings,
                                                                const api_responses::PortMap &portMap,
                                                                const types::ProxySettings &proxySettings,
                                                                const QString &preferredNodeHostname,
                                                                bool isFirewallAlwaysOnPlusEnabled,
                                                                bool hasUsableCachedWgConfig)
{
    if (bli->locationId().isCustomConfigsLocation()) {
        return new CustomConfigConnSettingsPolicy(bli);
    } else if (connectionSettings.isAutomatic()) {
        // Under Always On+ we can only attempt WireGuard from a usable cached config; otherwise skip it.
        const bool skipWireguardProtocol = isFirewallAlwaysOnPlusEnabled && !hasUsableCachedWgConfig;
        return new AutoConnSettingsPolicy(bli, portMap, proxySettings.isProxyEnabled(), skipWireguardProtocol, preferredNodeHostname);
    } else {
        // Manual mode is not a fallback chain: substitute WG (->IKEv2/OpenVPN UDP) only when it can't
        // be attempted at all (no usable cached config under Always On+); otherwise attempt WG as-is.
        types::ConnectionSettings overrideConnectionSettings = connectionSettings;
        if (isFirewallAlwaysOnPlusEnabled && connectionSettings.protocol().isWireGuardProtocol() && !hasUsableCachedWgConfig) {
            auto it = portMap.getPortItemByProtocolType(types::Protocol::IKEV2);
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
        return new ManualConnSettingsPolicy(bli, overrideConnectionSettings, portMap, preferredNodeHostname);
    }
}
