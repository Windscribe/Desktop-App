#pragma once

#include <atomic>
#include <string>
#include <thread>

class DefaultRouteMonitor final
{
public:
    explicit DefaultRouteMonitor(const std::string &deviceName);
    ~DefaultRouteMonitor();

    bool start(const std::string &endpoint);
    void stop();
    bool checkDefaultRoutes();
    bool isActive() const { return !doStopThread_; }

private:
    bool executeCommandWithLogging(const std::string &command) const;
    std::string getDefaultGateway() const;
    bool setEndpointDirectRoute();
    void unsetEndpointDirectRoute();

    std::string deviceName_;
    std::thread *monitorThread_;
    std::atomic<bool> doStopThread_;
    std::string endpoint_;
    std::string lastGateway_;
};
