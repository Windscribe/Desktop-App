#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class DefaultRouteMonitor final
{
public:
    DefaultRouteMonitor();
    ~DefaultRouteMonitor();

    bool start(const std::string &endpoint);
    void stop();
    bool checkDefaultRoutes();

#if defined(__linux__)
    // Whether a datagram from the link/route/address netlink groups can change the default gateway or remove the
    // endpoint route: an IPv4 default route added to or removed from the main table, a main-table host route deleted, a
    // link change (a link going down drops its routes without route
    // messages) other than a wireless-extension event, or an IPv4 address change (removing an address can drop routes
    // too). Other routes, e.g. the tunnel's own table or the endpoint route set here, don't count. A truncated datagram
    // counts, to be safe. The monitor checks once a burst of relevant changes has been quiet for a moment.
    static bool affectsDefaultGateway(const void *buf, size_t len, bool truncated);
#endif

private:
    bool executeCommandWithLogging(const std::string &program, const std::vector<std::string> &args) const;
    std::string getDefaultGateway() const;
    bool setEndpointDirectRoute();
    void unsetEndpointDirectRoute();

    void monitorLoop();

    std::thread monitorThread_;
    std::atomic<bool> doStopThread_{false};
    int wakePipe_[2] = {-1, -1};
    // checkDefaultRoutes runs on the monitor thread and on the caller's.
    std::mutex mutex_;
    std::string endpoint_;
    std::string lastGateway_;
};
