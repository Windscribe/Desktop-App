#include "customconfigconnsettingspolicy.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"

CustomConfigConnSettingsPolicy::CustomConfigConnSettingsPolicy(
    QSharedPointer<locationsmodel::BaseLocationInfo> bli) :
        locationInfo_(qSharedPointerDynamicCast<locationsmodel::CustomConfigLocationInfo>(bli))
{
    WS_ASSERT(!locationInfo_.isNull());
    WS_ASSERT(locationInfo_->locationId().isCustomConfigsLocation());
    connect(locationInfo_.data(), SIGNAL(hostnamesResolved()), SLOT(onHostnamesResolved()));
}

void CustomConfigConnSettingsPolicy::reset()
{
    // nothing todo
}

void CustomConfigConnSettingsPolicy::debugLocationInfoToLog() const
{
    qCDebug(LOG_CONNECTION) << locationInfo_->getLogString();
}

void CustomConfigConnSettingsPolicy::putFailedConnection()
{
    if (!bStarted_)
    {
        return;
    }

    locationInfo_->selectNextNode();
}

bool CustomConfigConnSettingsPolicy::isFailed() const
{
    return false;
}

CurrentConnectionDescr CustomConfigConnSettingsPolicy::getCurrentConnectionSettings() const
{
    CurrentConnectionDescr ccd;

    if (locationInfo_->isExistSelectedNode())
    {
        ccd.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
        ccd.ovpnData = locationInfo_->getOvpnData();
        ccd.isAllowFirewallAfterConnection = locationInfo_->isAllowFirewallAfterConnection();
        ccd.ip = locationInfo_->getSelectedIp();
        ccd.remoteCmdLine = locationInfo_->getSelectedRemoteCommand();
        ccd.customConfigFilename = locationInfo_->getFilename();
        ccd.port = locationInfo_->getSelectedPort();
        ccd.protocol = PROTOCOL::fromString(locationInfo_->getSelectedProtocol());
        ccd.wgCustomConfig = locationInfo_->getWireguardCustomConfig(ccd.ip);
    }
    else
    {
        ccd.connectionNodeType = CONNECTION_NODE_ERROR;
    }

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


