#include "manualconnsettingspolicy.h"

#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

ManualConnSettingsPolicy::ManualConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
    const types::ConnectionSettings &connectionSettings, const api_responses::PortMap &portMap) :
        locationInfo_(qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli)),
        portMap_(portMap), connectionSettings_(connectionSettings), failedManualModeCounter_(0)
{
    WS_ASSERT(!locationInfo_.isNull());
    WS_ASSERT(!locationInfo_->locationId().isCustomConfigsLocation());

    QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
    if (IpValidation::isIp(remoteOverride) && connectionSettings_.protocol() == types::Protocol::WIREGUARD) {
        locationInfo_->selectNodeByIp(remoteOverride);
    }
}

void ManualConnSettingsPolicy::reset()
{
    // nothing todo
}

void ManualConnSettingsPolicy::debugLocationInfoToLog() const
{
    qCInfo(LOG_CONNECTION) << "Connection settings:" << connectionSettings_.toJson(true);
    qCInfo(LOG_CONNECTION) << locationInfo_->getLogString();
}

void ManualConnSettingsPolicy::putFailedConnection()
{
    if (!bStarted_)
    {
        return;
    }

    if (failedManualModeCounter_ >= 2)
    {
        QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
        if (!IpValidation::isIp(remoteOverride) || connectionSettings_.protocol() != types::Protocol::WIREGUARD) {
            // try switch to another node for manual mode
            locationInfo_->selectNextNode();
        }
    }
    else
    {
        failedManualModeCounter_++;
    }
}

bool ManualConnSettingsPolicy::isFailed() const
{
    return false;
}

CurrentConnectionDescr ManualConnSettingsPolicy::getCurrentConnectionSettings() const
{
    CurrentConnectionDescr ccd;
    ccd.connectionNodeType = CONNECTION_NODE_DEFAULT;
    ccd.protocol = connectionSettings_.protocol();
    ccd.port = connectionSettings_.port();

    int useIpInd = portMap_.getUseIpInd(connectionSettings_.protocol());
    ccd.ip = locationInfo_->getIpForSelectedNode(useIpInd);
    if (ccd.ip.isEmpty()) {
        qCWarning(LOG_CONNECTION) << "Could not get IP for selected node.  Port map: ";
        for (auto item : portMap_.const_items()) {
            qCWarning(LOG_CONNECTION) << "protocol:" << item.protocol.toLongString() << "heading:" << item.heading << "use:" << item.use << "ports:" << item.ports;
        }
    }
    ccd.hostname = locationInfo_->getHostnameForSelectedNode();
    ccd.dnsHostName = locationInfo_->getDnsName();
    ccd.wgPeerPublicKey = locationInfo_->getWgPubKeyForSelectedNode();
    ccd.verifyX509name = locationInfo_->getVerifyX509name();

    // for static IP set additional fields
    if (locationInfo_->locationId().isStaticIpsLocation())
    {
        ccd.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
        ccd.username = locationInfo_->getStaticIpUsername();
        ccd.password = locationInfo_->getStaticIpPassword();
        ccd.staticIpPorts = locationInfo_->getStaticIpPorts();

        // for static ip with wireguard protocol override id to wg_ip
        if (ccd.protocol == types::Protocol::WIREGUARD )
        {
            ccd.ip = locationInfo_->getWgIpForSelectedNode();
        }
    }

    return ccd;
}

bool ManualConnSettingsPolicy::isAutomaticMode()
{
    return false;
}

bool ManualConnSettingsPolicy::isCustomConfig()
{
    return false;
}

void ManualConnSettingsPolicy::resolveHostnames()
{
    //nothing todo
    emit hostnamesResolved();
}

bool ManualConnSettingsPolicy::hasProtocolChanged()
{
    return false;
}

