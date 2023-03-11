#include "routes.h"
#include "../logger.h"
#include "../utils.h"

void Routes::add(const std::string &ip, const std::string &gateway, const std::string &mask)
{
    RouteDescr rd;
    rd.ip = ip;
    rd.gateway = gateway;
    rd.mask = mask;
    routes_.push_back(rd);

    std::string cmd = "ip route add " + ip + "/" + mask + " via " + gateway;
    Logger::instance().out("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}

void Routes::addWithInterface(const std::string &ip, const std::string &interface, const std::string &mask)
{
    RouteDescr rd;
    rd.ip = ip;
    rd.interface = interface;
    routes_.push_back(rd);

    std::string cmd = "ip route add " + ip + "/" + mask + " dev " + interface;
    Logger::instance().out("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}


void Routes::clear()
{
    for(auto const& rd: routes_)
    {
        if (rd.interface.empty())
        {
            std::string cmd = "ip route delete " + rd.ip + "/" + rd.mask + " via " + rd.gateway;
            Logger::instance().out("execute: %s", cmd.c_str());
            Utils::executeCommand(cmd);
        }
        else
        {
            std::string cmd = "ip route delete " + rd.ip + "/" + rd.mask + " dev " + rd.interface;
            Logger::instance().out("execute: %s", cmd.c_str());
            Utils::executeCommand(cmd);
        }
    }
    routes_.clear();
}
