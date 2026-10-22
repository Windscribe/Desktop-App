#include "main_route_table.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <linux/rtnetlink.h>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>
#include "../../utils.h"

namespace {

std::string stringMember(const rapidjson::Value &obj, const char *name)
{
    auto it = obj.FindMember(name);
    if (it == obj.MemberEnd()) {
        return std::string();
    }
    if (it->value.IsString()) {
        return std::string(it->value.GetString(), it->value.GetStringLength());
    }
    if (it->value.IsUint()) {
        return std::to_string(it->value.GetUint());
    }
    return std::string();
}

// `ip -j` prints metric as a number; accept a decimal string too. Absent means 0.
uint32_t uintMember(const rapidjson::Value &obj, const char *name)
{
    auto it = obj.FindMember(name);
    if (it == obj.MemberEnd()) {
        return 0;
    }
    if (it->value.IsUint()) {
        return it->value.GetUint();
    }
    if (it->value.IsString()) {
        const char *s = it->value.GetString();
        char *end = nullptr;
        const unsigned long value = std::strtoul(s, &end, 10);
        if (end != s && *end == '\0' && value <= std::numeric_limits<uint32_t>::max()) {
            return static_cast<uint32_t>(value);
        }
    }
    return 0;
}

bool flagsContain(const rapidjson::Value &obj, const char *flag)
{
    auto it = obj.FindMember("flags");
    if (it == obj.MemberEnd() || !it->value.IsArray()) {
        return false;
    }
    for (const auto &entry : it->value.GetArray()) {
        if (entry.IsString() && std::string(entry.GetString(), entry.GetStringLength()) == flag) {
            return true;
        }
    }
    return false;
}

// `ip -j` keeps flag names under -N. linkdown/dead nexthops are not selected by the kernel.
bool isDownRoute(const rapidjson::Value &obj)
{
    return flagsContain(obj, "linkdown") || flagsContain(obj, "dead");
}

// proto boot, as printed by `ip -N` ("3") or by name.
bool isBootProtocol(const std::string &protocol)
{
    return protocol == "3" || protocol == "boot";
}

// True when the first `range.prefixLength()` bits of `addr` match those of `range`'s address.
bool rangeContains(const types::IpAddressRange &range, const types::IpAddress &addr)
{
    if (range.family() != addr.family()) {
        return false;
    }
    const uint8_t *a = range.address().bytes();
    const uint8_t *b = addr.bytes();
    const uint8_t prefix = range.prefixLength();
    const std::size_t fullBytes = prefix / 8;
    if (!std::equal(a, a + fullBytes, b)) {
        return false;
    }
    const uint8_t restBits = prefix % 8;
    if (restBits == 0) {
        return true;
    }
    const uint8_t mask = static_cast<uint8_t>(0xFF << (8 - restBits));
    return (a[fullBytes] & mask) == (b[fullBytes] & mask);
}

// True when `outer` contains every address of `inner`.
bool covers(const types::IpAddressRange &outer, const types::IpAddressRange &inner)
{
    return outer.prefixLength() <= inner.prefixLength() && rangeContains(outer, inner.address());
}

// A pin we installed for exactly `dst`: proto ownProtocol on any device, or a legacy proto-boot
// pin on one of pinDevs. Kernel connected routes (proto 2) are not pins.
bool isExactPin(const MainRouteTable::Route &route, const types::IpAddressRange &dst,
                const std::vector<std::string> &pinDevs, const std::string &ownProtocol)
{
    if (route.dev.empty() || route.dst.prefixLength() != dst.prefixLength() || !covers(route.dst, dst)) {
        return false;
    }
    if (route.protocol == ownProtocol) {
        return true;
    }
    return isBootProtocol(route.protocol)
        && std::find(pinDevs.begin(), pinDevs.end(), route.dev) != pinDevs.end();
}

bool onTunnelDev(const MainRouteTable::Route &route, const std::vector<std::string> &tunnelDevs)
{
    return !route.dev.empty()
        && std::find(tunnelDevs.begin(), tunnelDevs.end(), route.dev) != tunnelDevs.end();
}

// True when this route, if it won the lookup, would send the destination around the tunnel.
bool routesAroundTunnel(const MainRouteTable::Route &route, const std::vector<std::string> &tunnelDevs)
{
    // A `throw` falls through to the next rule — the VPN's — instead of deciding the lookup.
    return !route.isThrow && !onTunnelDev(route, tunnelDevs);
}

// Kernel fib order: longer prefix, then lower metric. On an exact tie the kernel's pick depends on
// insertion order, which the dump does not tell, so the route into the tunnel wins: the destination
// is then pinned, as it was before main-table routes were considered at all.
bool beats(const MainRouteTable::Route &candidate, const MainRouteTable::Route &current,
           const std::vector<std::string> &tunnelDevs)
{
    if (candidate.dst.prefixLength() != current.dst.prefixLength()) {
        return candidate.dst.prefixLength() > current.dst.prefixLength();
    }
    if (candidate.metric != current.metric) {
        return candidate.metric < current.metric;
    }
    return !routesAroundTunnel(candidate, tunnelDevs) && routesAroundTunnel(current, tunnelDevs);
}

} // namespace

bool MainRouteTable::load()
{
    routes_.clear();
    loadedV4_ = false;
    loadedV6_ = false;
    for (const auto family : {types::IpAddress::IPv4, types::IpAddress::IPv6}) {
        const std::string familyArg = family == types::IpAddress::IPv4 ? "-4" : "-6";
        std::string output;
        // stdout only: an interleaved warning on stderr would make the JSON unparsable. -N prints the
        // protocol (and type) as numbers, so our pins are recognised whatever rt_protos names them.
        const int rc = Utils::executeCommand("ip", {"-N", familyArg, "-j", "route", "show", "table", "main"}, &output, false);
        if (rc != 0 || !parse(output, family)) {
            spdlog::error("MainRouteTable::load(): could not read the {} main routing table (rc={})", familyArg, rc);
            continue;
        }
        (family == types::IpAddress::IPv4 ? loadedV4_ : loadedV6_) = true;
    }
    return loadedV4_ && loadedV6_;
}

bool MainRouteTable::isLoaded(types::IpAddress::Family family) const
{
    return family == types::IpAddress::IPv4 ? loadedV4_ : loadedV6_;
}

bool MainRouteTable::parse(const std::string &json, types::IpAddress::Family family)
{
    rapidjson::Document doc;
    doc.Parse(json.c_str(), json.size());
    if (doc.HasParseError() || !doc.IsArray()) {
        return false;
    }

    for (const auto &obj : doc.GetArray()) {
        if (!obj.IsObject()) {
            continue;
        }
        const std::string dst = stringMember(obj, "dst");
        Route route;
        if (dst == "default") {
            route.dst = types::IpAddressRange(types::IpAddress(family == types::IpAddress::IPv4 ? "0.0.0.0" : "::"), 0);
        } else {
            // Host routes are printed without a prefix length; IpAddressRange makes those /32 or /128.
            route.dst = types::IpAddressRange(dst);
        }
        if (!route.dst.isValid() || route.dst.family() != family) {
            continue;
        }
        // Numeric under -N; the name is accepted too so dumps taken without it still parse.
        const std::string type = stringMember(obj, "type");
        route.isThrow = type == "throw" || type == std::to_string(RTN_THROW);
        route.isDown = isDownRoute(obj);
        route.dev = stringMember(obj, "dev");
        if (route.dev.empty()) {
            auto nexthops = obj.FindMember("nexthops");
            if (nexthops != obj.MemberEnd() && nexthops->value.IsArray() && !nexthops->value.Empty()
                && nexthops->value[0].IsObject()) {
                route.dev = stringMember(nexthops->value[0], "dev");
                route.isDown = route.isDown || isDownRoute(nexthops->value[0]);
            }
        }
        // `ip` leaves protocol out for proto boot unless run with -d, so absent means boot.
        route.protocol = stringMember(obj, "protocol");
        if (route.protocol.empty()) {
            route.protocol = std::to_string(RTPROT_BOOT);
        }
        route.metric = uintMember(obj, "metric");
        routes_.push_back(std::move(route));
    }
    return true;
}

bool MainRouteTable::hasSpecificRoute(const types::IpAddressRange &dst,
                                      const std::vector<std::string> &ignoredDevs,
                                      const std::string &ownProtocol,
                                      const std::vector<std::string> &pinDevs) const
{
    const Route *best = nullptr;
    for (const auto &route : routes_) {
        if (route.dst.prefixLength() < 2 || !covers(route.dst, dst)) {
            continue;
        }
        // A pin is as specific as the destination it covers, so it would always win and then
        // remove itself on the next pass. The system route underneath is what matters. A down
        // route is one the kernel will not select, so it must not suppress a pin either.
        if (route.isDown || route.protocol == ownProtocol || isExactPin(route, dst, pinDevs, ownProtocol)) {
            continue;
        }
        if (best == nullptr || beats(route, *best, ignoredDevs)) {
            best = &route;
        }
    }
    return best != nullptr && routesAroundTunnel(*best, ignoredDevs);
}

bool MainRouteTable::hasOwnRoute(const types::IpAddressRange &dst, const std::string &ownProtocol) const
{
    for (const auto &route : routes_) {
        if (route.protocol == ownProtocol && route.dst.prefixLength() == dst.prefixLength()
            && covers(route.dst, dst)) {
            return true;
        }
    }
    return false;
}

std::vector<MainRouteTable::ExactPin> MainRouteTable::exactPins(const types::IpAddressRange &dst,
                                                                const std::vector<std::string> &pinDevs,
                                                                const std::string &ownProtocol) const
{
    std::vector<ExactPin> pins;
    for (const auto &route : routes_) {
        if (isExactPin(route, dst, pinDevs, ownProtocol)) {
            pins.push_back(ExactPin{route.dev, route.protocol});
        }
    }
    return pins;
}
