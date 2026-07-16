#pragma once

#include "engine/connectionmanager/connectors/iconnectionplatformpolicy.h"
#include "engine/helper/helper.h"

class ConnectionPlatformPolicy : public IConnectionPlatformPolicy
{
public:
    explicit ConnectionPlatformPolicy(Helper *helper);

    bool isLockdownBlockingIkev2() const override;
    void disableDohIfNeeded() override;
    void setGaiIpv4PriorityEnabled(bool isEnabled) override;
    AdapterGatewayInfo detectDefaultAdapter() override;

private:
    Helper *helper_;
};
