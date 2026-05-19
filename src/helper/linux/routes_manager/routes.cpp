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

    const std::string dest = ip + "/" + mask;
    spdlog::info("execute: ip route add {} via {}", dest, gateway);
    Utils::executeCommand("ip", {"route", "add", dest, "via", gateway});
}

void Routes::addWithInterface(const std::string &ip, const std::string &interface, const std::string &mask)
{
    RouteDescr rd;
    rd.ip = ip;
    rd.interface = interface;
    rd.mask = mask;
    routes_.push_back(rd);

    const std::string dest = ip + "/" + mask;
    spdlog::info("execute: ip route add {} dev {}", dest, interface);
    Utils::executeCommand("ip", {"route", "add", dest, "dev", interface});
}


void Routes::clear()
{
    for(auto const& rd: routes_)
    {
        const std::string dest = rd.ip + "/" + rd.mask;
        if (rd.interface.empty())
        {
            spdlog::info("execute: ip route delete {} via {}", dest, rd.gateway);
            Utils::executeCommand("ip", {"route", "delete", dest, "via", rd.gateway});
        }
        else
        {
            spdlog::info("execute: ip route delete {} dev {}", dest, rd.interface);
            Utils::executeCommand("ip", {"route", "delete", dest, "dev", rd.interface});
        }
    }
    routes_.clear();
}
