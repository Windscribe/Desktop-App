#pragma once

#include "types/enums.h"

class SplitTunnelFailurePolicy
{
public:
    struct Action {
        bool disable = false;
        bool show = false;
    };

    Action handle(SPLIT_TUNNEL_START_FAIL_REASON reason, bool enabled)
    {
        if (!enabled) {
            return {};
        }
        bool sessionEnded = false;
#ifdef Q_OS_MACOS
        sessionEnded = reason == SPLIT_TUNNEL_START_FAIL_REASON_MAC_SESSION_ENDED;
#else
        Q_UNUSED(reason);
#endif
        const bool duplicate = sessionFailureShown_ && sessionEnded;
        const Action action{!sessionEnded, !duplicate};
        sessionFailureShown_ |= sessionEnded;
        return action;
    }

    void onConnectionStateChanged(CONNECT_STATE state)
    {
        if (state != CONNECT_STATE_CONNECTED) {
            sessionFailureShown_ = false;
        }
    }

private:
    bool sessionFailureShown_ = false;
};
