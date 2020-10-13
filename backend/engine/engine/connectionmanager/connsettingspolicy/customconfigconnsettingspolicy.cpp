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

    ccd.connectionNodeType = CONNECTION_NODE_CUSTOM_OVPN_CONFIG;
    ccd.ovpnData = locationInfo_->getOvpnData();
    ccd.ip = locationInfo_->getSelectedIp();
    ccd.remoteCmdLine = locationInfo_->getSelectedRemoteCommand();
    ccd.customConfigFilename = locationInfo_->getFilename();
    ccd.port = locationInfo_->getSelectedPort();
    ccd.protocol = locationInfo_->getSelectedProtocol();

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


