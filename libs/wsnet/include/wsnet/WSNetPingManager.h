#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "scapix_object.h"
#include "WSNetCancelableCallback.h"

namespace wsnet {

enum class PingType { kHttp = 0, kIcmp };

typedef std::function<void(const std::string &ip, bool isSuccess, std::int32_t timeMs, bool isFromDisconnectedVpnState)> WSNetPingCallback;

// Useful for testing and debugging purposes
class WSNetPingManager : public scapix_object<WSNetPingManager>
{
public:
    virtual ~WSNetPingManager() {}

    // ip - required
    // hostname - optional for http ping
    // pingType: 0 - HTTP, 1 - ICMP
    virtual std::shared_ptr<WSNetCancelableCallback> ping(const std::string &ip, const std::string &hostname,
                                                          PingType pingType, WSNetPingCallback callback) = 0;
};

} // namespace wsnet
