#include "pingmethod_icmp_apple.h"

namespace wsnet {

PingMethodIcmp_apple::PingMethodIcmp_apple(PingManager_apple &pingManager_apple, std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
        PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback) :
    IPingMethod(id, ip, hostname, isParallelPing, callback, pingMethodFinishedCallback),
    pingManager_apple_(pingManager_apple)
{
}

PingMethodIcmp_apple::~PingMethodIcmp_apple()
{
}

void PingMethodIcmp_apple::ping(bool isFromDisconnectedVpnState)
{
    pingManager_apple_.ping(ip_, std::bind(&PingMethodIcmp_apple::callback, this, std::placeholders::_1));
}

void PingMethodIcmp_apple::callback(int timeMs)
{
    timeMs_ = timeMs;
    isSuccess_ = (timeMs_ > 0);
    callFinished();
}


} // namespace wsnet
