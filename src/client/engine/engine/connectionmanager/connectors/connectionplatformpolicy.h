#pragma once

#include <optional>

#include "engine/connectionmanager/connectors/iconnectionplatformpolicy.h"
#include "engine/helper/helper.h"

class ConnectionPlatformPolicy : public IConnectionPlatformPolicy
{
public:
    explicit ConnectionPlatformPolicy(Helper *helper);

    bool isLockdownMode() const override;
    bool needsSleepEventAwareDisconnect() const override;
    bool shouldReconnectOnOnlineStateChange() const override;
    void disableDohIfNeeded() override;
    void setGaiIpv4PriorityEnabled(bool isEnabled) override;
    AdapterGatewayInfo detectDefaultAdapter() override;

private:
    Helper *helper_;
    mutable std::optional<bool> cachedLockdownMode_;
};
