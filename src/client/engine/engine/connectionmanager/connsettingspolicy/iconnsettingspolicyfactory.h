#pragma once

#include <QSharedPointer>

#include "api_responses/portmap.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "types/connectionsettings.h"
#include "types/proxysettings.h"

class BaseConnSettingsPolicy;

// Selects and constructs the attempt-sequencing policy (auto/manual/custom-config) for a
// connect request. Seam so the selection logic lives outside ConnectionManager and tests
// can hand CM a fake policy.
class IConnSettingsPolicyFactory
{
public:
    virtual ~IConnSettingsPolicyFactory() {}

    virtual BaseConnSettingsPolicy *createPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                                 const types::ConnectionSettings &connectionSettings,
                                                 const api_responses::PortMap &portMap,
                                                 const types::ProxySettings &proxySettings,
                                                 const QString &preferredNodeHostname,
                                                 bool isFirewallAlwaysOnPlusEnabled,
                                                 bool hasUsableCachedWgConfig) = 0;
};
