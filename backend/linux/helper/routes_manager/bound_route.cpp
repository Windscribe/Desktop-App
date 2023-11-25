#include "bound_route.h"
#include "../utils.h"
#include "../logger.h"

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

    std::string cmd = "ip route add -net 0.0.0.0 via " + ipAddress_ + " dev " + interfaceName_;
    Logger::instance().out("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
    isBoundRouteAdded_ = true;
}

void BoundRoute::remove()
{
    if (isBoundRouteAdded_)
    {
        std::string cmd = "ip route delete -net 0.0.0.0 via " + ipAddress_ + " dev " + interfaceName_;
        Logger::instance().out("execute: %s", cmd.c_str());
        Utils::executeCommand(cmd);
        isBoundRouteAdded_ = false;
    }
}
