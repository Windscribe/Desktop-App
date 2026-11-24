#include <cstdio>
#include <string>
#include <vector>

#include "../wg_dns_routes.h"

namespace {

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

std::vector<std::string> routed(const std::string &dns, const std::vector<std::string> &allowedIps,
                                const std::string &endpoint = "203.0.113.10:443")
{
    std::vector<std::string> out;
    for (const auto &ip : WgDnsRoutes::serversNeedingTunnelRoute(dns, allowedIps, endpoint)) {
        out.push_back(ip.toString());
    }
    return out;
}

void testFullTunnelCandidates()
{
    VERIFY((routed("10.255.255.1", {"0.0.0.0/0"}) == std::vector<std::string>{"10.255.255.1"}));
    VERIFY((routed("10.255.255.1, fd00::1", {"0.0.0.0/0", "::/0"}) == std::vector<std::string>{"10.255.255.1", "fd00::1"}));
    VERIFY((routed("192.168.1.53", {"0.0.0.0/0"}) == std::vector<std::string>{"192.168.1.53"}));
}

void testServerOutsideAllowedIpsIsLeftAlone()
{
    VERIFY(routed("192.168.1.1", {"10.0.0.0/8"}).empty());
    VERIFY(routed("fd00::1", {"0.0.0.0/0"}).empty());
}

void testServerInsideSubnetAllowedIp()
{
    VERIFY((routed("10.255.255.1", {"10.255.255.0/24"}) == std::vector<std::string>{"10.255.255.1"}));
}

void testExactHostAllowedIpAlreadyRoutesIt()
{
    VERIFY(routed("10.255.255.1", {"10.255.255.1/32"}).empty());
    VERIFY(routed("10.255.255.1", {"0.0.0.0/0", "10.255.255.1/32"}).empty());
}

// A DNS server on this machine, or the VPN server itself, must never be sent into the tunnel.
void testLoopbackAndVpnServerAreNeverCandidates()
{
    VERIFY(routed("127.0.0.1", {"0.0.0.0/0"}).empty());
    VERIFY(routed("::1", {"::/0"}).empty());
    VERIFY(routed("203.0.113.10", {"0.0.0.0/0"}, "203.0.113.10:443").empty());
    VERIFY(routed("2001:db8::10", {"::/0"}, "[2001:db8::10]:443").empty());
    VERIFY((routed("203.0.113.10", {"0.0.0.0/0"}, "not-an-endpoint") == std::vector<std::string>{"203.0.113.10"}));
}

void testSeparatorsDuplicatesAndInvalidEntries()
{
    VERIFY((routed("1.1.1.1;1.0.0.1 1.1.1.1,,bogus", {"0.0.0.0/0"}) == std::vector<std::string>{"1.1.1.1", "1.0.0.1"}));
    VERIFY(routed("", {"0.0.0.0/0"}).empty());
    VERIFY(routed("10.255.255.1", {}).empty());
    VERIFY(routed("10.255.255.1", {"not-a-range"}).empty());
}

void testMacRouteGet()
{
    VERIFY(WgDnsRoutes::isDefaultRouteInMacRouteGet("   route to: 1.1.1.1\ndestination: default\n       mask: default\n"
                                                    "    gateway: 10.0.0.1\n  interface: en0\n"));
    VERIFY(WgDnsRoutes::isDefaultRouteInMacRouteGet("   route to: 2606:4700:4700::1111\ndestination: ::\n       mask: default\n"
                                                    "  interface: en0\n"));
    VERIFY(!WgDnsRoutes::isDefaultRouteInMacRouteGet("   route to: 10.0.0.5\ndestination: 10.0.0.0\n       mask: 255.0.0.0\n"
                                                     "  interface: en0\n"));
    VERIFY(!WgDnsRoutes::isDefaultRouteInMacRouteGet("   route to: 127.0.0.1\ndestination: 127.0.0.1\n  interface: lo0\n"));
    VERIFY(!WgDnsRoutes::isDefaultRouteInMacRouteGet("route: writing to routing socket: not in table\n"));
    VERIFY(!WgDnsRoutes::isDefaultRouteInMacRouteGet(""));
}

void testLinuxRouteMatch()
{
    VERIFY(WgDnsRoutes::isDefaultOnlyInLinuxRouteMatch("default via 192.168.1.1 dev eth0 proto dhcp metric 100 \n"));
    VERIFY(WgDnsRoutes::isDefaultOnlyInLinuxRouteMatch("default via 192.168.1.1 dev eth0 metric 100\n"
                                                       "default via 10.0.0.1 dev wlan0 metric 600\n"));
    VERIFY(!WgDnsRoutes::isDefaultOnlyInLinuxRouteMatch("default via 192.168.1.1 dev eth0 metric 100\n"
                                                        "192.168.1.0/24 dev eth0 proto kernel scope link src 192.168.1.20\n"));
    VERIFY(WgDnsRoutes::isDefaultOnlyInLinuxRouteMatch("default proto static metric 100 \n"
                                                       "\tnexthop via 192.168.1.1 dev eth0 weight 1 \n"
                                                       "\tnexthop via 192.168.2.1 dev eth1 weight 1 \n"));
    VERIFY(!WgDnsRoutes::isDefaultOnlyInLinuxRouteMatch("203.0.113.10 via 192.168.1.1 dev eth0\n"));
    VERIFY(!WgDnsRoutes::isDefaultOnlyInLinuxRouteMatch(""));
}

} // namespace

int main()
{
    testFullTunnelCandidates();
    testServerOutsideAllowedIpsIsLeftAlone();
    testServerInsideSubnetAllowedIp();
    testExactHostAllowedIpAlreadyRoutesIt();
    testLoopbackAndVpnServerAreNeverCandidates();
    testSeparatorsDuplicatesAndInvalidEntries();
    testMacRouteGet();
    testLinuxRouteMatch();
    if (g_failures != 0) {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}
