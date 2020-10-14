#ifndef LOCATIONSMODEL_PINGTIME_H
#define LOCATIONSMODEL_PINGTIME_H

namespace locationsmodel {

class PingTime
{
public:
    PingTime();
    PingTime(int timeMs);  // cppcheck-suppress noExplicitConstructor

    static constexpr int NO_PING_INFO = -2;
    static constexpr int PING_FAILED = -1;

    static constexpr int LATENCY_STEP1 = 200;
    static constexpr int LATENCY_STEP2 = 500;
    static constexpr int MAX_LATENCY_FOR_PING_FAILED = 2000;

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
