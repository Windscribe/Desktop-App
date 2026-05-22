#include "routes.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

void Routes::add(const types::IpAddress &ip, const types::IpAddress &gateway, uint8_t prefixLength)
{
    if (!ip.isValid() || !gateway.isValid() || ip.family() != gateway.family()) {
        spdlog::error("Routes::add(): destIp/gatewayIp invalid or family mismatch (ip={}, gateway={})",
                      ip.toString(), gateway.toString());
        return;
    }

    RouteDescr rd;
    rd.ip = ip;
    rd.gateway = gateway;
    rd.prefixLength = prefixLength;
    routes_.push_back(rd);

    const std::string dest = ip.toString() + "/" + std::to_string(prefixLength);
    const std::string via = gateway.toString();
    spdlog::info("execute: ip route add {} via {}", dest, via);
    Utils::executeCommand("ip", {"route", "add", dest, "via", via});
}

void Routes::addWithInterface(const types::IpAddress &ip, const std::string &interface, uint8_t prefixLength)
{
    if (!ip.isValid()) {
        spdlog::error("Routes::addWithInterface(): invalid destination IP");
        return;
    }

    RouteDescr rd;
    rd.ip = ip;
    rd.interface = interface;
    rd.prefixLength = prefixLength;
    routes_.push_back(rd);

    const std::string dest = ip.toString() + "/" + std::to_string(prefixLength);
    spdlog::info("execute: ip route add {} dev {}", dest, interface);
    Utils::executeCommand("ip", {"route", "add", dest, "dev", interface});
}


void Routes::clear()
{
    for (auto const &rd : routes_) {
        const std::string dest = rd.ip.toString() + "/" + std::to_string(rd.prefixLength);
        if (rd.interface.empty()) {
            const std::string via = rd.gateway.toString();
            spdlog::info("execute: ip route delete {} via {}", dest, via);
            Utils::executeCommand("ip", {"route", "delete", dest, "via", via});
        } else {
            spdlog::info("execute: ip route delete {} dev {}", dest, rd.interface);
            Utils::executeCommand("ip", {"route", "delete", dest, "dev", rd.interface});
        }
    }
    routes_.clear();
}
