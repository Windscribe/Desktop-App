#pragma once

#include <QSharedPointer>

#include "api_responses/portmap.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "types/connectionsettings.h"
#include "types/proxysettings.h"

class IConnectionAttemptStrategy;

// Selects and constructs the attempt-sequencing strategy (auto/manual/custom-config) for a
// connect request. Seam so the selection logic lives outside ConnectionManager and tests
// can hand CM a fake strategy.
class IConnectionAttemptStrategyFactory
{
public:
    virtual ~IConnectionAttemptStrategyFactory() {}

    virtual IConnectionAttemptStrategy *createStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                                 const types::ConnectionSettings &connectionSettings,
                                                 const api_responses::PortMap &portMap,
                                                 const types::ProxySettings &proxySettings,
                                                 const QString &preferredNodeHostname,
                                                 bool isFirewallAlwaysOnPlusEnabled,
                                                 bool hasUsableCachedWgConfig,
                                                 bool isLockdownMode) = 0;
};
