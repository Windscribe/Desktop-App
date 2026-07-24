#pragma once

#include "iconnectionattemptstrategyfactory.h"

class ConnectionAttemptStrategyFactory : public IConnectionAttemptStrategyFactory
{
public:
    IConnectionAttemptStrategy *createStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                         const types::ConnectionSettings &connectionSettings,
                                         const api_responses::PortMap &portMap,
                                         const types::ProxySettings &proxySettings,
                                         const QString &preferredNodeHostname,
                                         bool isFirewallAlwaysOnPlusEnabled,
                                         bool hasUsableCachedWgConfig,
                                         bool isLockdownMode) override;
};
