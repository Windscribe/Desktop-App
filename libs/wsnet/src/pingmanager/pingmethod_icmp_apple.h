#pragma once

#include "ipingmethod.h"
#include "apple/pingmanager_apple.h"

namespace wsnet {

class PingMethodIcmp_apple : public IPingMethod
{
public:
    PingMethodIcmp_apple(PingManager_apple &pingManager_apple, std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
                    PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback);

    virtual ~PingMethodIcmp_apple();
    void ping(bool isFromDisconnectedVpnState) override;

private:
    PingManager_apple &pingManager_apple_;

    void callback(int timeMs);
};


} // namespace wsnet
