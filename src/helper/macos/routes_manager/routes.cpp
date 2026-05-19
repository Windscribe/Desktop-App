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

    spdlog::info("execute: route add -net {} {} {}", ip, gateway, mask);
    Utils::executeCommand("route", {"add", "-net", ip, gateway, mask});
}

void Routes::addWithInterface(const std::string &ip, const std::string &interface)
{
    RouteDescr rd;
    rd.ip = ip;
    rd.interface = interface;
    routes_.push_back(rd);

    spdlog::info("execute: route -q -n add -inet {} -interface {}", ip, interface);
    Utils::executeCommand("route", {"-q", "-n", "add", "-inet", ip, "-interface", interface});
}


void Routes::clear()
{
    for(auto const& rd: routes_)
    {
        if (rd.interface.empty())
        {
            spdlog::info("execute: route -q -n delete -net {} {} {}", rd.ip, rd.gateway, rd.mask);
            Utils::executeCommand("route", {"-q", "-n", "delete", "-net", rd.ip, rd.gateway, rd.mask});
        }
        else
        {
            std::string netstatOutput;
            Utils::executeCommand("netstat", {"-rn"}, &netstatOutput);
            if (netstatOutput.find(rd.interface) != std::string::npos)
            {
                spdlog::info("execute: route -q -n delete -inet {} -interface {}", rd.ip, rd.interface);
                Utils::executeCommand("route", {"-q", "-n", "delete", "-inet", rd.ip, "-interface", rd.interface});
            }
        }
    }
    routes_.clear();
}
