#include "pingtime.h"
#include <QObject>

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
    if (timeMs_ == NO_PING_INFO)
    {
        return 3;
    }
    else if (timeMs_ == PING_FAILED)
    {
        return 0;
    }
    else if (timeMs_ < LATENCY_STEP1)
    {
        return 3;
    }
    else if (timeMs_ < LATENCY_STEP2)
    {
        return 2;
    }
    else
    {
        return 1;
    }
}
