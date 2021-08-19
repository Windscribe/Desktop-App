#ifndef PINGTIME_H
#define PINGTIME_H


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

#endif // PINGTIME_H
