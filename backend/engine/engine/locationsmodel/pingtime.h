#ifndef LOCATIONSMODEL_PINGTIME_H
#define LOCATIONSMODEL_PINGTIME_H

namespace locationsmodel {

class PingTime
{
public:
    PingTime();
    PingTime(int timeMs);  // cppcheck-suppress noExplicitConstructor

    static int NO_PING_INFO;
    static int PING_FAILED;

    // return connection speed from 0 to 3
    int toConnectionSpeed() const;

    bool operator==(const int &other) const
    {
        return other == timeMs_;
    }

    bool operator!=(const PingTime &other) const
    {
        return other.timeMs_ != timeMs_;
    }

    int toInt() const
    {
        return timeMs_;
    }

private:
    int timeMs_;
};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_PINGTIME_H
