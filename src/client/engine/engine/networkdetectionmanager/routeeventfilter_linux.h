#pragma once

#include <QString>

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include <sys/types.h>

struct DefaultRoute_linux
{
    QString interface;
    QString gateway;

    bool operator==(const DefaultRoute_linux &other) const
    {
        return interface == other.interface && gateway == other.gateway;
    }
};

class DefaultRouteTracker_linux
{
public:
    using Reader = std::function<DefaultRoute_linux()>;

    explicit DefaultRouteTracker_linux(Reader reader);

    DefaultRoute_linux read();
    bool differsFromLastRead() const;

private:
    Reader reader_;
    std::optional<DefaultRoute_linux> lastRead_;
};

class RouteEventFilter_linux
{
public:
    enum class Result { Refresh, TunnelOnly };

    using NameLookup = std::function<bool(int index, std::string &name)>;
    using Clock = std::function<std::chrono::steady_clock::time_point()>;
    using ReceiveCallback = std::function<ssize_t(char *buffer, size_t size, int &error)>;
    using Callback = std::function<void()>;

    static constexpr int kMaxReadsPerWakeup = 64;

    RouteEventFilter_linux(NameLookup nameLookup, Clock clock);

    Result classify(const char *buffer, ssize_t len, size_t bufferSize);
    void processWakeup(const ReceiveCallback &receive, const Callback &refresh, const Callback &tunnelRoutesChanged);

private:
    bool isTunnelIndex(int index);

    NameLookup nameLookup_;
    Clock clock_;
    std::unordered_map<int, std::chrono::steady_clock::time_point> tunnelIndexes_;
};
