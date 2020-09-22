#include "pingtime.h"
#include <QObject>

//const int typeIdPingTime = qRegisterMetaType<locationsmodel::PingTime>("locationsmodel::PingTime");

namespace locationsmodel {

int PingTime::NO_PING_INFO = -2;
int PingTime::PING_FAILED = -1;



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
    else if (timeMs_ < 200)
    {
        return 3;
    }
    else if (timeMs_ < 500)
    {
        return 2;
    }
    else
    {
        return 1;
    }
}

} //namespace locationsmodel
