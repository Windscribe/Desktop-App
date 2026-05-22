#include "bound_route.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

BoundRoute::BoundRoute() : isBoundRouteAdded_(false)
{

}

BoundRoute::~BoundRoute()
{
    remove();
}

void BoundRoute::create(const types::IpAddress &ipAddress, const std::string &interfaceName)
{
    remove();   // remove previous if exists

    if (!ipAddress.isValid()) {
        spdlog::error("BoundRoute::create(): invalid gateway IP, skipping");
        return;
    }

    // Hardcodes "-net 0.0.0.0" as the destination, so the gateway must be v4. A v6
    // gateway would silently produce a malformed `ip route` command. Today's caller
    // (RoutesManager) only passes connectStatus.defaultAdapter.gatewayIp (v4 by
    // contract — see ADAPTER_GATEWAY_INFO::gatewayIp vs gatewayIpV6), but enforce
    // here so a future caller can't violate the precondition silently.
    if (!ipAddress.isV4()) {
        spdlog::error("BoundRoute::create(): gateway must be IPv4 (got {}), skipping",
                      ipAddress.toString());
        return;
    }

    ipAddress_ = ipAddress;
    interfaceName_ = interfaceName;

    const std::string via = ipAddress_.toString();
    spdlog::info("execute: ip route add -net 0.0.0.0 via {} dev {}", via, interfaceName_);
    Utils::executeCommand("ip", {"route", "add", "-net", "0.0.0.0", "via", via, "dev", interfaceName_});
    isBoundRouteAdded_ = true;
}

void BoundRoute::remove()
{
    if (isBoundRouteAdded_)
    {
        const std::string via = ipAddress_.toString();
        spdlog::info("execute: ip route delete -net 0.0.0.0 via {} dev {}", via, interfaceName_);
        Utils::executeCommand("ip", {"route", "delete", "-net", "0.0.0.0", "via", via, "dev", interfaceName_});
        isBoundRouteAdded_ = false;
    }
}
