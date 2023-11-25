#pragma once

#include <QDataStream>
#include "utils/ws_assert.h"

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

    bool operator==(const PingTime &other) const
    {
        return other.timeMs_ == timeMs_;
    }

    bool operator!=(const PingTime &other) const
    {
        return !(*this == other);
    }

    int toInt() const  {  return timeMs_;  }

    friend QDataStream& operator <<(QDataStream& stream, const PingTime& p)
    {
        stream << versionForSerialization_;
        stream << p.timeMs_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, PingTime& p)
    {
        quint32 version;
        stream >> version;
        WS_ASSERT(version == versionForSerialization_);
        stream >> p.timeMs_;
        return stream;
    }

private:
    int timeMs_;
    static constexpr quint32 versionForSerialization_ = 1;
};
