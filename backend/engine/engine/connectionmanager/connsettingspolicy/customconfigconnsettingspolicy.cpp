#include "customconfigconnsettingspolicy.h"
#include "utils/logger.h"

CustomConfigConnSettingsPolicy::CustomConfigConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli)
{
    locationInfo_ = qSharedPointerDynamicCast<locationsmodel::CustomConfigLocationInfo>(bli);
    Q_ASSERT(!locationInfo_.isNull());
    Q_ASSERT(locationInfo_->locationId().isCustomConfigsLocation());
    connect(locationInfo_.data(), SIGNAL(hostnamesResolved()), SLOT(onHostnamesResolved()));
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
    if (!bStarted_)
    {
        return;
    }

    /*if (failedManualModeCounter_ >= 2)
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

    ccd.connectionNodeType = CONNECTION_NODE_CUSTOM_OVPN_CONFIG;
    ccd.ovpnData = locationInfo_->getOvpnData();
    ccd.ip = locationInfo_->getSelectedIp();
    ccd.customConfigFilename = locationInfo_->getFilename();

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

void CustomConfigConnSettingsPolicy::resolveHostnames()
{
    locationInfo_->resolveHostnames();
}

void CustomConfigConnSettingsPolicy::onHostnamesResolved()
{
    emit hostnamesResolved();
}


