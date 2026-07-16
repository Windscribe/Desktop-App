#pragma once

#include "iconnsettingspolicyfactory.h"

class ConnSettingsPolicyFactory : public IConnSettingsPolicyFactory
{
public:
    BaseConnSettingsPolicy *createPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                         const types::ConnectionSettings &connectionSettings,
                                         const api_responses::PortMap &portMap,
                                         const types::ProxySettings &proxySettings,
                                         const QString &preferredNodeHostname,
                                         bool isFirewallAlwaysOnPlusEnabled,
                                         bool hasUsableCachedWgConfig) override;
};
