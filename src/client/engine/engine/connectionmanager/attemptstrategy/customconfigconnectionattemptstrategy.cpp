#include "customconfigconnectionattemptstrategy.h"
#include "utils/ws_assert.h"
#include "utils/log/logger.h"

CustomConfigConnectionAttemptStrategy::CustomConfigConnectionAttemptStrategy(
    QSharedPointer<locationsmodel::BaseLocationInfo> bli) :
        locationInfo_(qSharedPointerDynamicCast<locationsmodel::CustomConfigLocationInfo>(bli))
{
    WS_ASSERT(!locationInfo_.isNull());
    WS_ASSERT(locationInfo_->locationId().isCustomConfigsLocation());
    connect(locationInfo_.data(), &locationsmodel::CustomConfigLocationInfo::hostnamesResolved, this, &CustomConfigConnectionAttemptStrategy::hostnamesResolved);
}

void CustomConfigConnectionAttemptStrategy::reset()
{
    // nothing todo
}

void CustomConfigConnectionAttemptStrategy::debugLocationInfoToLog() const
{
    qCInfo(LOG_CONNECTION) << locationInfo_->getLogString();
}

void CustomConfigConnectionAttemptStrategy::putFailedConnection()
{
    locationInfo_->selectNextNode();
}

bool CustomConfigConnectionAttemptStrategy::isFailed() const
{
    return false;
}

CurrentConnectionDescr CustomConfigConnectionAttemptStrategy::getCurrentConnectionSettings() const
{
    CurrentConnectionDescr ccd;

    if (locationInfo_->isExistSelectedNode()) {
        ccd.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
        ccd.ip = locationInfo_->getSelectedIp();
        ccd.port = locationInfo_->getSelectedPort();
        ccd.protocol = types::Protocol::fromString(locationInfo_->getSelectedProtocol());
        if (ccd.protocol.isWireGuardProtocol()) {
            ccd.wireGuard.customConfig = locationInfo_->getWireguardCustomConfig(ccd.ip);
        } else {
            ccd.openVpn.customConfig = locationInfo_->getOvpnConfigForSelectedEndpoint();
        }
    } else {
        ccd.connectionNodeType = CONNECTION_NODE_ERROR;
    }

    return ccd;
}

bool CustomConfigConnectionAttemptStrategy::isAutomaticMode()
{
    return false;
}

void CustomConfigConnectionAttemptStrategy::resolveHostnames()
{
    locationInfo_->resolveHostnames();
}

types::Protocol CustomConfigConnectionAttemptStrategy::preResolveProtocol() const
{
    if (locationInfo_->configType() == CUSTOM_CONFIG_WIREGUARD) {
        return types::Protocol::WIREGUARD;
    }
    return types::Protocol::OPENVPN_UDP;
}
