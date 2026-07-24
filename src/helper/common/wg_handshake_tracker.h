#pragma once

#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

// WireGuard reports the last-handshake time as a wall-clock timestamp, so staleness is measured as
// monotonic time since it last advanced, using a clock that ignores clock changes but accrues suspend.
class WgHandshakeTracker
{
public:
    // The server discards our key information if the handshake has not advanced for ~3 minutes.
    static constexpr int64_t kStaleThresholdSec = 180;

    void reset()
    {
        lastHandshake_ = 0;
        lastHandshakeChangeTime_ = 0;
    }

    // Updates the baseline when the handshake value advances; returns seconds since it last advanced.
    int64_t update(uint64_t handshake)
    {
        if (handshake == 0) {
            // No handshake observed yet, so there is nothing to be stale.
            return 0;
        }
        const int64_t now = monotonicSec();
        if (handshake != lastHandshake_) {
            lastHandshake_ = handshake;
            lastHandshakeChangeTime_ = now;
        }
        return now - lastHandshakeChangeTime_;
    }

    bool isStale(uint64_t handshake)
    {
        return update(handshake) > kStaleThresholdSec;
    }

private:
    static int64_t monotonicSec()
    {
#ifdef _WIN32
        return static_cast<int64_t>(::GetTickCount64() / 1000);
#else
        struct timespec ts;
#ifdef __APPLE__
        clock_gettime(CLOCK_MONOTONIC, &ts);
#else
        clock_gettime(CLOCK_BOOTTIME, &ts);
#endif
        return ts.tv_sec;
#endif
    }

    uint64_t lastHandshake_ = 0;
    int64_t lastHandshakeChangeTime_ = 0;
};
