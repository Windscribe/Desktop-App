#include "customconfigconnsettingspolicy.h"
#include "utils/logger.h"

CustomConfigConnSettingsPolicy::CustomConfigConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli)
{
    locationInfo_ = qSharedPointerDynamicCast<locationsmodel::CustomConfigLocationInfo>(bli);
    Q_ASSERT(!locationInfo_.isNull());
    Q_ASSERT(locationInfo_->locationId().isCustomConfigsLocation());
}

void CustomConfigConnSettingsPolicy::reset()
{
    // nothing todo
}

void CustomConfigConnSettingsPolicy::debugLocationInfoToLog() const
{
    /*QString strNodes;
    for (int i = 0; i < locationInfo_->nodesCount(); ++i)
    {
        strNodes += getLogForNode(i);
        strNodes += "; ";
    }

    connectionSettings_.logConnectionSettings();
    qCDebug(LOG_CONNECTION) << "Location nodes:" << strNodes;*/
}

void CustomConfigConnSettingsPolicy::putFailedConnection()
{
    /*if (!bStarted_)
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
    }*/
}

bool CustomConfigConnSettingsPolicy::isFailed() const
{
    return false;
}

CurrentConnectionDescr CustomConfigConnSettingsPolicy::getCurrentConnectionSettings() const
{
    CurrentConnectionDescr ccd;
    /*if (!connectionSettings_.isInitialized())
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
    }*/

    return ccd;
}

void CustomConfigConnSettingsPolicy::saveCurrentSuccessfullConnectionSettings()
{
    //nothing todo
}

bool CustomConfigConnSettingsPolicy::isAutomaticMode()
{
    return false;
}


