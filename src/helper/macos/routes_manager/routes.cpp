#include "routes.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

namespace {

// macOS `route` infers family from "-inet" / "-inet6"; pick explicitly off the address
// family so dual-stack callers don't have to.
const char *familyFlag(const types::IpAddress &ip)
{
    return ip.isV6() ? "-inet6" : "-inet";
}

std::string cidrString(const types::IpAddress &ip, uint8_t prefixLength)
{
    return ip.toString() + "/" + std::to_string(prefixLength);
}

} // namespace


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

    const std::string dest = cidrString(ip, prefixLength);
    const std::string via = gateway.toString();
    const char *fam = familyFlag(ip);
    spdlog::info("execute: route -q -n add {} {} {}", fam, dest, via);
    Utils::executeCommand("route", {"-q", "-n", "add", fam, dest, via});
}

void Routes::addWithInterface(const types::IpAddress &ip, uint8_t prefixLength, const std::string &interface)
{
    if (!ip.isValid()) {
        spdlog::error("Routes::addWithInterface(): invalid destination IP");
        return;
    }

    RouteDescr rd;
    rd.ip = ip;
    rd.prefixLength = prefixLength;
    rd.interface = interface;
    routes_.push_back(rd);

    const std::string dest = cidrString(ip, prefixLength);
    const char *fam = familyFlag(ip);
    spdlog::info("execute: route -q -n add {} {} -interface {}", fam, dest, interface);
    Utils::executeCommand("route", {"-q", "-n", "add", fam, dest, "-interface", interface});
}

void Routes::addWithInterface(const types::IpAddressRange &range, const std::string &interface)
{
    if (!range.isValid()) {
        spdlog::error("Routes::addWithInterface(): invalid range");
        return;
    }
    addWithInterface(range.address(), range.prefixLength(), interface);
}


void Routes::clear()
{
    for (auto const &rd : routes_) {
        const std::string dest = cidrString(rd.ip, rd.prefixLength);
        const char *fam = familyFlag(rd.ip);
        if (rd.interface.empty()) {
            const std::string via = rd.gateway.toString();
            spdlog::info("execute: route -q -n delete {} {} {}", fam, dest, via);
            Utils::executeCommand("route", {"-q", "-n", "delete", fam, dest, via});
        } else {
            // Only delete if the interface is still present — `route delete` against a
            // gone interface would log a noisy "not in table" error. Preserves the v4
            // helper's previous defensive check.
            std::string netstatOutput;
            Utils::executeCommand("netstat", {"-rn"}, &netstatOutput);
            if (netstatOutput.find(rd.interface) != std::string::npos) {
                spdlog::info("execute: route -q -n delete {} {} -interface {}", fam, dest, rd.interface);
                Utils::executeCommand("route", {"-q", "-n", "delete", fam, dest, "-interface", rd.interface});
            }
        }
    }
    routes_.clear();
}
