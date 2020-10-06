#include "manualconnsettingspolicy.h"
#include "utils/logger.h"

ManualConnSettingsPolicy::ManualConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const ConnectionSettings &connectionSettings, const apiinfo::PortMap &portMap)
{
    connectionSettings_ = connectionSettings;
    portMap_ = portMap;
    failedManualModeCounter_ = 0;
    locationInfo_ = qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli);
    Q_ASSERT(!locationInfo_.isNull());
    Q_ASSERT(!locationInfo_->locationId().isCustomConfigsLocation());
}

void ManualConnSettingsPolicy::reset()
{
    // nothing todo
}

void ManualConnSettingsPolicy::debugLocationInfoToLog() const
{
    QString strNodes;
    for (int i = 0; i < locationInfo_->nodesCount(); ++i)
    {
        strNodes += getLogForNode(i);
        strNodes += "; ";
    }

    connectionSettings_.logConnectionSettings();
    qCDebug(LOG_CONNECTION) << "Location nodes:" << strNodes;
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

QString ManualConnSettingsPolicy::getLogForNode(int ind) const
{
    QString ret = "node" + QString::number(ind + 1);

    ret += " = {";

    const int IPS_COUNT = 3;
    for (int i = 0; i < IPS_COUNT; ++i)
    {
        ret += "ip" + QString::number(i + 1) + " = ";
        ret += locationInfo_->getIpForNode(ind, i);
        if (i != (IPS_COUNT - 1))
        {
            ret += ", ";
        }
    }

    ret += "}";
    return ret;
}
