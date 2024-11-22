#include "routes.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

void Routes::add(const std::string &ip, const std::string &gateway, const std::string &mask)
{
    RouteDescr rd;
    rd.ip = ip;
    rd.gateway = gateway;
    rd.mask = mask;
    routes_.push_back(rd);

    std::string cmd = "route add -net " + ip + " " + gateway + " " + mask;
    spdlog::info("execute: {}", cmd);
    Utils::executeCommand(cmd);
}

void Routes::addWithInterface(const std::string &ip, const std::string &interface)
{
    RouteDescr rd;
    rd.ip = ip;
    rd.interface = interface;
    routes_.push_back(rd);

    std::string cmd = "route -q -n add -inet " + ip + " -interface " + interface;
    spdlog::info("execute: {}", cmd);
    Utils::executeCommand(cmd);
}


void Routes::clear()
{
    for(auto const& rd: routes_)
    {
        if (rd.interface.empty())
        {
            std::string cmd = "route -q -n delete -net " + rd.ip + " " + rd.gateway + " " + rd.mask;
            spdlog::info("execute: {}", cmd);
            Utils::executeCommand(cmd);
        }
        else
        {
            std::string cmd = "netstat -rn | grep " + rd.interface + " && route -q -n delete -inet " + rd.ip + " -interface " + rd.interface;
            spdlog::info("execute: {}", cmd);
            Utils::executeCommand(cmd);
        }
    }
    routes_.clear();
}
