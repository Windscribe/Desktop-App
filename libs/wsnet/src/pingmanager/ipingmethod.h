#pragma once
#include <functional>
#include "WSNetPingManager.h"
#include "utils/cancelablecallback.h"

namespace wsnet {

using PingFinishedCallback = std::shared_ptr<CancelableCallback<WSNetPingCallback>>;
typedef std::function<void(std::uint64_t id)> PingMethodFinishedCallback;

class IPingMethod
{
public:
    IPingMethod(std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
                PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback) :
        id_(id),
        ip_(ip),
        hostname_(hostname),
        isParallelPing_(isParallelPing),
        callback_(callback),
        pingMethodFinishedCallback_(pingMethodFinishedCallback)
    {
    }

    virtual ~IPingMethod() {}

    // A derived class must call the callFinished() when a ping completes
    virtual void ping(bool isFromDisconnectedVpnState) = 0;

    void callCallback()
    {
        callback_->call(ip_, isSuccess_, timeMs_, isFromDisconnectedVpnState_);
    }

    void callFinished()
    {
        pingMethodFinishedCallback_(id_);
    }

    void setIsFromDisconnectedVpnState(bool isFromDisconnectedVpnState)
    {
        isFromDisconnectedVpnState_ = isFromDisconnectedVpnState;
    }

    bool isParallelPing() const { return isParallelPing_; }

protected:
    std::uint64_t id_;
    PingFinishedCallback callback_;
    PingMethodFinishedCallback pingMethodFinishedCallback_;
    std::string ip_;
    std::string hostname_;
    bool isFromDisconnectedVpnState_ = true;
    bool isSuccess_ = false;
    std::int32_t timeMs_ = -1;
    bool isParallelPing_;
};

} // namespace wsnet
