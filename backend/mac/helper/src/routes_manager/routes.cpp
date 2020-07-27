#include "routes.h"
#include "utils.h"
#include "logger.h"

void Routes::add(const std::string &ip, const std::string &gateway, const std::string &mask)
{
    RouteDescr rd;
    rd.ip = ip;
    rd.gateway = gateway;
    rd.mask = mask;
    routes_.push_back(rd);
    
    std::string cmd = "route add -net " + ip + " " + gateway + " " + mask;
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}
void Routes::clear()
{
    for(auto const& rd: routes_)
    {
        std::string cmd = "route delete -net " + rd.ip + " " + rd.gateway + " " + rd.mask;
        LOG("execute: %s", cmd.c_str());
        Utils::executeCommand(cmd);
    }
    routes_.clear();
}
