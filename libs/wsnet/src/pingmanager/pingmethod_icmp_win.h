#pragma once

#include "ipingmethod.h"
#include <windows.h>
#include "eventcallbackmanager_win.h"

struct _IO_STATUS_BLOCK;

namespace wsnet {


class PingMethodIcmp_win : public IPingMethod
{
public:
    PingMethodIcmp_win(EventCallbackManager_win &eventCallbackManager, std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
                    PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback);

    virtual ~PingMethodIcmp_win();
    void ping(bool isFromDisconnectedVpnState) override;

private:
    EventCallbackManager_win &eventCallbackManager_;
    HANDLE icmpFile_ = INVALID_HANDLE_VALUE;
    std::unique_ptr<unsigned char[]> replyBuffer_;
    DWORD replySize_ = 0;
    std::chrono::time_point<std::chrono::steady_clock> startTime_;
    HANDLE hEvent_;

    void onCallback();
};

} // namespace wsnet
