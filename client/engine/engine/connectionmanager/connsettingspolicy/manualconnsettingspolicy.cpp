#include "manualconnsettingspolicy.h"
#include "utils/logger.h"

ManualConnSettingsPolicy::ManualConnSettingsPolicy(
    QSharedPointer<locationsmodel::BaseLocationInfo> bli,
    const ConnectionSettings &connectionSettings, const apiinfo::PortMap &portMap) :
        locationInfo_(qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli)),
        portMap_(portMap), connectionSettings_(connectionSettings), failedManualModeCounter_(0)
{
    Q_ASSERT(!locationInfo_.isNull());
    Q_ASSERT(!locationInfo_->locationId().isCustomConfigsLocation());
}

void ManualConnSettingsPolicy::reset()
{
    // nothing todo
}

void ManualConnSettingsPolicy::debugLocationInfoToLog() const
{
    connectionSettings_.logConnectionSettings();
    qCDebug(LOG_CONNECTION) << locationInfo_->getLogString();
}

void ManualConnSettingsPolicy::putFailedConnection()
{
    if (!bStarted_)
    {
        return;
    }

    if (failedManualModeCounter_ >= 2)
    {
        // try switch to another node for manual mode
        locationInfo_->selectNextNode();
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
    if (!connectionSettings_.isInitialized())
    {
        qCDebug(LOG_CONNECTION) << "Fatal error, connectionSettings_ not initialized";
        Q_ASSERT(false);
        ccd.connectionNodeType = CONNECTION_NODE_ERROR;
        return ccd;
    }
    else
    {
        ccd.connectionNodeType = CONNECTION_NODE_DEFAULT;
        ccd.protocol = connectionSettings_.protocol();
        ccd.port = connectionSettings_.port();

        int useIpInd = portMap_.getUseIpInd(connectionSettings_.protocol());
        ccd.ip = locationInfo_->getIpForSelectedNode(useIpInd);
        ccd.hostname = locationInfo_->getHostnameForSelectedNode();
        ccd.dnsHostName = locationInfo_->getDnsName();
        ccd.wgPublicKey = locationInfo_->getWgPubKeyForSelectedNode();
        ccd.verifyX509name = locationInfo_->getVerifyX509name();

        // for static IP set additional fields
        if (locationInfo_->locationId().isStaticIpsLocation())
        {
            ccd.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
            ccd.username = locationInfo_->getStaticIpUsername();
            ccd.password = locationInfo_->getStaticIpPassword();
            ccd.staticIpPorts = locationInfo_->getStaticIpPorts();

            // for static ip with wireguard protocol override id to wg_ip
            if (ccd.protocol.getType() == ProtocolType::PROTOCOL_WIREGUARD )
            {
                ccd.ip = locationInfo_->getWgIpForSelectedNode();
            }
        }
    }

    return ccd;
}

void ManualConnSettingsPolicy::saveCurrentSuccessfullConnectionSettings()
{
    //nothing todo
}

bool ManualConnSettingsPolicy::isAutomaticMode()
{
    return false;
}

void ManualConnSettingsPolicy::resolveHostnames()
{
    //nothing todo
    emit hostnamesResolved();
}
