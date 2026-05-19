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

void BoundRoute::create(const std::string &ipAddress, const std::string &interfaceName)
{
    remove();   // remove previous if exists

    ipAddress_ = ipAddress;
    interfaceName_ = interfaceName;

    spdlog::info("execute: ip route add -net 0.0.0.0 via {} dev {}", ipAddress_, interfaceName_);
    Utils::executeCommand("ip", {"route", "add", "-net", "0.0.0.0", "via", ipAddress_, "dev", interfaceName_});
    isBoundRouteAdded_ = true;
}

void BoundRoute::remove()
{
    if (isBoundRouteAdded_)
    {
        spdlog::info("execute: ip route delete -net 0.0.0.0 via {} dev {}", ipAddress_, interfaceName_);
        Utils::executeCommand("ip", {"route", "delete", "-net", "0.0.0.0", "via", ipAddress_, "dev", interfaceName_});
        isBoundRouteAdded_ = false;
    }
}
