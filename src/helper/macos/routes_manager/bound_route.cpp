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

    spdlog::info("execute: route add -net 0.0.0.0 {} -ifscope {}", ipAddress_, interfaceName_);
    Utils::executeCommand("route", {"add", "-net", "0.0.0.0", ipAddress_, "-ifscope", interfaceName_});
    isBoundRouteAdded_ = true;
}

void BoundRoute::remove()
{
    if (isBoundRouteAdded_)
    {
        spdlog::info("execute: route delete -net 0.0.0.0 {} -ifscope {}", ipAddress_, interfaceName_);
        Utils::executeCommand("route", {"delete", "-net", "0.0.0.0", ipAddress_, "-ifscope", interfaceName_});
        isBoundRouteAdded_ = false;
    }
}
