#include "pingtime.h"

constexpr int PingTime::NO_PING_INFO;
constexpr int PingTime::PING_FAILED;

const int typeIdPingTime = qRegisterMetaType<PingTime>("PingTime");

PingTime::PingTime()
{
    timeMs_ = NO_PING_INFO;
}

PingTime::PingTime(int timeMs) : timeMs_(timeMs)
{
}

int PingTime::toConnectionSpeed() const
{
    // Return the number of filled latency bars we should display to the user.
    if (timeMs_ <= 0) {
        // Show no bars if the ping failed, we don't have a ping result yet, or ping is 0.
        return 0;
    } else if (timeMs_ < LATENCY_STEP1) {
        return 3;
    } else if (timeMs_ < LATENCY_STEP2) {
        return 2;
    } else {
        return 1;
    }
}
