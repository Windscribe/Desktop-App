#pragma once

#include "baseconnsettingspolicy.h"
#include "types/connectionsettings.h"
#include "api_responses/portmap.h"
#include "engine/locationsmodel/mutablelocationinfo.h"

// // manage manual connection mode (only for API and static ips locations)
class ManualConnSettingsPolicy : public BaseConnSettingsPolicy
{
    Q_OBJECT
public:
    ManualConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const types::ConnectionSettings &connectionSettings,
                             const api_responses::PortMap &portMap);


    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    bool isCustomConfig() override;
    void resolveHostnames() override;
    bool hasProtocolChanged() override;

private:
    QSharedPointer<locationsmodel::MutableLocationInfo> locationInfo_;
    api_responses::PortMap portMap_;
    types::ConnectionSettings connectionSettings_;
    int failedManualModeCounter_;
};
