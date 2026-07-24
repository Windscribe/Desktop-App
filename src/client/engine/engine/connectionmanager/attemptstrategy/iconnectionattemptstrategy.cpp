#include "iconnectionattemptstrategy.h"

#include "engine/locationsmodel/mutablelocationinfo.h"
#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"

void IConnectionAttemptStrategy::selectNodeForAttempt(locationsmodel::MutableLocationInfo *locationInfo, types::Protocol nextProtocol,
                                                      const QString &preferredNodeHostname, bool isRetry)
{
    bool isNodeSelected = false;
    if (nextProtocol.isWireGuardProtocol()) {
        const QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
        if (NetworkingValidation::isIp(remoteOverride)) {
            isNodeSelected = locationInfo->selectNodeByIp(remoteOverride);
        }
    }
    if (isNodeSelected) {
        return;
    }

    if (!preferredNodeHostname.isEmpty()) {
        if (isRetry) {
            qCInfo(LOG_CONNECTION) << "Selecting preferred node by hostname on retry: " << preferredNodeHostname;
            if (!locationInfo->selectNodeByHostname(preferredNodeHostname)) {
                locationInfo->selectNextNode();
            }
        } else {
            qCInfo(LOG_CONNECTION) << "Selecting preferred node by hostname: " << preferredNodeHostname;
            if (locationInfo->selectNodeByHostname(preferredNodeHostname)) {
                qCInfo(LOG_CONNECTION) << "Found matching node: " << locationInfo->getLogString();
            }
        }
    } else if (isRetry) {
        locationInfo->selectNextNode();
    }
}

CurrentConnectionDescr IConnectionAttemptStrategy::descrForSelectedNode(locationsmodel::MutableLocationInfo *locationInfo,
                                                                        const api_responses::PortMap &portMap,
                                                                        types::Protocol protocol, uint port)
{
    CurrentConnectionDescr ccd;

    ccd.connectionNodeType = CONNECTION_NODE_DEFAULT;
    ccd.protocol = protocol;
    ccd.port = port;

    int useIpInd = portMap.getUseIpInd(protocol);
    ccd.ip = locationInfo->getIpForSelectedNode(useIpInd);
    if (ccd.ip.isEmpty()) {
        qCWarning(LOG_CONNECTION) << "Could not get IP for selected node.  Port map: ";
        for (const auto &item : portMap.const_items()) {
            qCWarning(LOG_CONNECTION) << "protocol:" << item.protocol.toLongString() << "heading:" << item.heading << "use:" << item.use << "ports:" << item.ports;
        }
    }
    ccd.hostname = locationInfo->getHostnameForSelectedNode();
    ccd.wireGuard.peerPublicKey = locationInfo->getWgPubKeyForSelectedNode();
    ccd.verifyX509name = locationInfo->getVerifyX509name();
    ccd.isIpv6Support = locationInfo->isIpv6SupportForSelectedNode();

    // for static IP set additional fields
    if (locationInfo->locationId().isStaticIpsLocation()) {
        ccd.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
        ccd.staticIps.credentials = api_responses::ServerCredentials(locationInfo->getStaticIpUsername(), locationInfo->getStaticIpPassword());
        ccd.staticIps.ports = locationInfo->getStaticIpPorts();

        // for static ip with wireguard protocol override id to wg_ip
        if (ccd.protocol == types::Protocol::WIREGUARD) {
            ccd.ip = locationInfo->getWgIpForSelectedNode();
        }
    }

    return ccd;
}
