#pragma once

#include <memory>

class WireGuardCommunicator;

class DefaultRouteMonitor final
{
public:
    DefaultRouteMonitor(std::shared_ptr<WireGuardCommunicator> comm, NET_LUID adapterLuid);
    ~DefaultRouteMonitor();

    bool start();
    void stop();
    bool checkDefaultRoutes();

private:
    std::shared_ptr<WireGuardCommunicator> comm_;
    NET_LUID adapterLuid_;
    HANDLE routeChangeCallback_;
    HANDLE ipInterfaceChangeCallback_;
    NET_IFINDEX lastIfIndex4_;
    NET_IFINDEX lastIfIndex6_;
};