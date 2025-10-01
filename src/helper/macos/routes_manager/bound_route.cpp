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

    std::string cmd = "route add -net 0.0.0.0 " + ipAddress_ + " -ifscope " + interfaceName_;
    spdlog::info("execute: {}", cmd);
    Utils::executeCommand(cmd);
    isBoundRouteAdded_ = true;
}

void BoundRoute::remove()
{
    if (isBoundRouteAdded_)
    {
        std::string cmd = "route delete -net 0.0.0.0 " + ipAddress_ + " -ifscope " + interfaceName_;
        spdlog::info("execute: {}", cmd);
        Utils::executeCommand(cmd);
        isBoundRouteAdded_ = false;
    }
}
