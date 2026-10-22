#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>

// Watches rtnetlink for routes added to or removed from `main`. That covers links and addresses
// too: the kernel adds and removes their connected routes with them. A burst (an interface coming
// up) is coalesced: the callback runs on the monitor thread once things have been quiet for a
// moment. Routes with `ignoredRouteProtocol` (the caller's own) are not reported.
class RouteChangeMonitor
{
public:
    RouteChangeMonitor(std::function<void()> onChanged, uint8_t ignoredRouteProtocol);
    ~RouteChangeMonitor();

    // Both are idempotent. stop() joins the monitor thread, so it must not be called while
    // holding a lock the callback takes.
    void start();
    void stop();

private:
    std::function<void()> onChanged_;
    const uint8_t ignoredRouteProtocol_;
    std::thread thread_;
    std::atomic<bool> stopRequested_;

    void run(int fd);
};
