#pragma once

#include "iconnectionattemptstrategy.h"
#include "types/connectionsettings.h"
#include "api_responses/portmap.h"
#include "engine/locationsmodel/mutablelocationinfo.h"

// // manage manual connection mode (only for API and static ips locations)
class ManualConnectionAttemptStrategy : public IConnectionAttemptStrategy
{
    Q_OBJECT
public:
    ManualConnectionAttemptStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const types::ConnectionSettings &connectionSettings,
                             const api_responses::PortMap &portMap, const QString &preferredNodeHostname);
    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    types::Protocol preResolveProtocol() const override { return connectionSettings_.protocol(); }

private:
    QSharedPointer<locationsmodel::MutableLocationInfo> locationInfo_;
    api_responses::PortMap portMap_;
    types::ConnectionSettings connectionSettings_;
    int failedManualModeCounter_;
    QString preferredNodeHostname_;
};
