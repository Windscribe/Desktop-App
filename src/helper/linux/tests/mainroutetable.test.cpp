// Pins which excluded split-tunnel destinations MainRouteTable reports as already routed around the
// tunnel by the main table (and so left unpinned), using `ip -j route show table main` dumps of the
// two-LAN setup from #2031: primary enp1s0 192.168.77.10/24 (default route), secondary enp7s0
// 192.168.88.10/24 + 2001:db8:88::10/64 (no default route).

#include <cstdio>
#include <string>
#include <vector>

#include "../split_tunneling/hostnames_manager/main_route_table.h"

namespace
{

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        fprintf(stderr, "FAILED line %d: %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

const std::string kOwn = "87";
const std::vector<std::string> kWgDevs = {"wg0"};
const std::vector<std::string> kOvpnDevs = {"tun0"};

// WireGuard: the catch-all lives in its own table, so main only holds the physical routes, the
// on-node DNS route and, once installed, our pin for 8.8.8.8 (proto 87).
const char *const kWireGuardV4 = R"([
    {"dst":"default","gateway":"192.168.77.1","dev":"enp1s0","protocol":"dhcp","metric":100,"flags":[]},
    {"dst":"10.255.255.0/24","dev":"wg0","scope":"link","flags":[]},
    {"dst":"192.168.77.0/24","dev":"enp1s0","protocol":"kernel","scope":"link","prefsrc":"192.168.77.10","metric":100,"flags":[]},
    {"dst":"192.168.88.0/24","dev":"enp7s0","protocol":"kernel","scope":"link","prefsrc":"192.168.88.10","metric":101,"flags":[]},
    {"dst":"8.8.8.8","gateway":"192.168.77.1","dev":"enp1s0","protocol":"87","flags":[]},
    {"type":"blackhole","dst":"203.0.113.0/24","protocol":"static","flags":[]},
    {"type":"throw","dst":"198.51.100.0/24","protocol":"static","flags":[]}
])";

const char *const kWireGuardV6 = R"([
    {"dst":"2001:db8:88::/64","dev":"enp7s0","protocol":"kernel","metric":256,"flags":[],"pref":"medium"},
    {"dst":"fe80::/64","dev":"enp1s0","protocol":"kernel","metric":256,"flags":[],"pref":"medium"},
    {"dst":"default","gateway":"fe80::1","dev":"enp1s0","protocol":"ra","metric":100,"flags":[],"pref":"medium"}
])";

// OpenVPN: its catch-all halves and the server route sit in main next to the physical routes.
const char *const kOpenVpnV4 = R"([
    {"dst":"0.0.0.0/1","gateway":"10.111.0.1","dev":"tun0","flags":[]},
    {"dst":"default","gateway":"192.168.77.1","dev":"enp1s0","protocol":"dhcp","metric":100,"flags":[]},
    {"dst":"128.0.0.0/1","gateway":"10.111.0.1","dev":"tun0","flags":[]},
    {"dst":"10.111.0.0/16","dev":"tun0","protocol":"kernel","scope":"link","prefsrc":"10.111.0.5","flags":[]},
    {"dst":"185.0.0.1","gateway":"192.168.77.1","dev":"enp1s0","flags":[]},
    {"dst":"192.168.88.0/24","dev":"enp7s0","protocol":"kernel","scope":"link","prefsrc":"192.168.88.10","metric":101,"flags":[]},
    {"dst":"default","protocol":"static","metric":200,"flags":[],"nexthops":[{"gateway":"192.168.88.1","dev":"enp7s0","weight":1,"flags":[]}]}
])";

types::IpAddressRange r(const char *cidr)
{
    return types::IpAddressRange(cidr);
}

void testWireGuardSecondaryLanIsCovered()
{
    MainRouteTable t;
    VERIFY(t.parse(kWireGuardV4, types::IpAddress::IPv4));
    VERIFY(t.parse(kWireGuardV6, types::IpAddress::IPv6));

    // The reported case: hosts on the secondary LAN, both families.
    VERIFY(t.hasSpecificRoute(r("192.168.88.1"), kWgDevs, kOwn));
    VERIFY(t.hasSpecificRoute(r("2001:db8:88::1"), kWgDevs, kOwn));
    // A sub-range of it, and the primary's own subnet (no more hairpinning via its gateway).
    VERIFY(t.hasSpecificRoute(r("192.168.88.128/25"), kWgDevs, kOwn));
    VERIFY(t.hasSpecificRoute(r("192.168.77.20"), kWgDevs, kOwn));
    // Host bits set in a configured CIDR do not matter.
    VERIFY(t.hasSpecificRoute(r("192.168.88.5/24"), kWgDevs, kOwn));
}

void testWireGuardDefaultRoutedNeedsPin()
{
    MainRouteTable t;
    VERIFY(t.parse(kWireGuardV4, types::IpAddress::IPv4));
    VERIFY(t.parse(kWireGuardV6, types::IpAddress::IPv6));

    // Only the default route covers these: they keep today's pin via the default adapter.
    VERIFY(!t.hasSpecificRoute(r("1.1.1.1"), kWgDevs, kOwn));
    VERIFY(!t.hasSpecificRoute(r("2606:4700::1111"), kWgDevs, kOwn));
    // A range wider than the secondary subnet is not covered by it.
    VERIFY(!t.hasSpecificRoute(r("192.168.0.0/16"), kWgDevs, kOwn));
    // Our own pin must not count, or the pin would remove itself on the next pass.
    VERIFY(!t.hasSpecificRoute(r("8.8.8.8"), kWgDevs, kOwn));
    // Routes on the tunnel device are the VPN's own.
    VERIFY(!t.hasSpecificRoute(r("10.255.255.1"), kWgDevs, kOwn));
    // A throw route hands the lookup to the next rule (the tunnel), a blackhole decides it.
    VERIFY(!t.hasSpecificRoute(r("198.51.100.7"), kWgDevs, kOwn));
    VERIFY(t.hasSpecificRoute(r("203.0.113.7"), kWgDevs, kOwn));
    // No cross-family matches.
    VERIFY(!t.hasSpecificRoute(r("::ffff:192.168.88.1"), kWgDevs, kOwn));
}

void testOpenVpn()
{
    MainRouteTable t;
    VERIFY(t.parse(kOpenVpnV4, types::IpAddress::IPv4));

    VERIFY(t.hasSpecificRoute(r("192.168.88.1"), kOvpnDevs, kOwn));
    // The server route goes via the primary already.
    VERIFY(t.hasSpecificRoute(r("185.0.0.1"), kOvpnDevs, kOwn));
    // The /1 halves are on the tunnel device and a multipath default is still a default.
    VERIFY(!t.hasSpecificRoute(r("1.1.1.1"), kOvpnDevs, kOwn));
    VERIFY(!t.hasSpecificRoute(r("200.1.1.1"), kOvpnDevs, kOwn));
    VERIFY(!t.hasSpecificRoute(r("10.111.0.7"), kOvpnDevs, kOwn));
    // Without knowing the tunnel device, the /1 halves would still be too short to count.
    VERIFY(!t.hasSpecificRoute(r("1.1.1.1"), {}, kOwn));
}

void testOwnRoute()
{
    MainRouteTable t;
    VERIFY(t.parse(kWireGuardV4, types::IpAddress::IPv4));

    VERIFY(t.hasOwnRoute(r("8.8.8.8/32"), kOwn));
    VERIFY(!t.hasOwnRoute(r("8.8.4.4/32"), kOwn));
    VERIFY(!t.hasOwnRoute(r("8.8.8.0/24"), kOwn));
    // The secondary subnet route is the system's, not ours.
    VERIFY(!t.hasOwnRoute(r("192.168.88.0/24"), kOwn));
}

// The same WireGuard table as `ip -N` prints it: protocol and type as numbers.
const char *const kWireGuardV4Numeric = R"([
    {"dst":"default","gateway":"192.168.77.1","dev":"enp1s0","protocol":"16","metric":100,"flags":[]},
    {"dst":"192.168.88.0/24","dev":"enp7s0","protocol":"2","scope":"253","prefsrc":"192.168.88.10","metric":101,"flags":[]},
    {"dst":"8.8.8.8","gateway":"192.168.77.1","dev":"enp1s0","protocol":"87","flags":[]},
    {"type":"6","dst":"203.0.113.0/24","protocol":"4","flags":[]},
    {"type":"9","dst":"198.51.100.0/24","protocol":"4","flags":[]}
])";

void testNumericOutput()
{
    MainRouteTable t;
    VERIFY(t.parse(kWireGuardV4Numeric, types::IpAddress::IPv4));

    VERIFY(t.hasSpecificRoute(r("192.168.88.1"), kWgDevs, kOwn));
    VERIFY(!t.hasSpecificRoute(r("8.8.8.8"), kWgDevs, kOwn));
    VERIFY(t.hasOwnRoute(r("8.8.8.8/32"), kOwn));
    VERIFY(!t.hasSpecificRoute(r("198.51.100.7"), kWgDevs, kOwn));
    VERIFY(t.hasSpecificRoute(r("203.0.113.7"), kWgDevs, kOwn));
}

// Overlapping routes: the kernel uses the longest prefix, then the lowest metric. A shorter
// physical route must not hide a longer tunnel route, and our own /32 pin must not hide the
// system route underneath it.
void testBestRouteWins()
{
    const char *const json = R"([
        {"dst":"10.0.0.0/8","dev":"tun0","protocol":"4","metric":0,"flags":[]},
        {"dst":"10.1.2.0/24","dev":"enp7s0","protocol":"4","metric":100,"flags":[]},
        {"dst":"192.168.88.0/24","dev":"enp7s0","protocol":"2","metric":100,"flags":[]},
        {"dst":"192.168.88.10","gateway":"10.8.0.1","dev":"tun0","protocol":"4","metric":0,"flags":[]},
        {"dst":"192.168.88.1","gateway":"192.168.77.1","dev":"enp1s0","protocol":"87","metric":0,"flags":[]},
        {"dst":"192.168.50.0/24","dev":"enp7s0","protocol":"4","metric":200,"flags":[]},
        {"dst":"192.168.50.0/24","dev":"tun0","protocol":"4","metric":50,"flags":[]},
        {"dst":"192.168.60.0/24","dev":"enp7s0","protocol":"4","metric":10,"flags":[]},
        {"dst":"192.168.60.0/24","dev":"tun0","protocol":"4","metric":100,"flags":[]},
        {"dst":"198.51.100.0/16","dev":"enp7s0","protocol":"4","metric":100,"flags":[]},
        {"type":"throw","dst":"198.51.100.0/24","protocol":"4","metric":100,"flags":[]},
        {"dst":"203.0.113.0/24","dev":"tun0","protocol":"4","flags":[]},
        {"dst":"203.0.113.0/24","dev":"enp7s0","protocol":"4","metric":5,"flags":[]},
        {"dst":"203.0.114.0/24","dev":"tun0","protocol":"4","metric":0,"flags":[]},
        {"dst":"203.0.114.0/24","dev":"enp7s0","protocol":"4","metric":0,"flags":[]}
    ])";

    MainRouteTable t;
    VERIFY(t.parse(json, types::IpAddress::IPv4));

    // /24 on the secondary LAN beats the /8 on the tunnel. The rest of the /8 still needs a pin.
    VERIFY(t.hasSpecificRoute(r("10.1.2.5"), kOvpnDevs, kOwn));
    VERIFY(!t.hasSpecificRoute(r("10.9.9.9"), kOvpnDevs, kOwn));
    // /32 on the tunnel beats the secondary /24.
    VERIFY(!t.hasSpecificRoute(r("192.168.88.10"), kOvpnDevs, kOwn));
    // Our pin for .1 must not hide the /24 that already routes it around the tunnel.
    VERIFY(t.hasSpecificRoute(r("192.168.88.1"), kOvpnDevs, kOwn));
    // Same prefix: the lower metric wins. A missing metric is 0 and beats an explicit one.
    // An exact tie goes to the tunnel, so the destination keeps its pin.
    VERIFY(!t.hasSpecificRoute(r("192.168.50.1"), kOvpnDevs, kOwn));
    VERIFY(t.hasSpecificRoute(r("192.168.60.1"), kOvpnDevs, kOwn));
    VERIFY(!t.hasSpecificRoute(r("203.0.113.7"), kOvpnDevs, kOwn));
    VERIFY(!t.hasSpecificRoute(r("203.0.114.7"), kOvpnDevs, kOwn));
    // A longer throw hands the lookup to the tunnel even though a shorter physical route exists.
    VERIFY(!t.hasSpecificRoute(r("198.51.100.7"), kOvpnDevs, kOwn));
}

const std::vector<std::string> kPinDevs = {"enp1s0"};

// A linkdown or dead route is not selected by the kernel, so it must not suppress a pin.
void testDownRouteIsNotCoverage()
{
    const char *const json = R"([
        {"dst":"192.168.88.0/24","dev":"enp7s0","protocol":"kernel","flags":["linkdown"]},
        {"dst":"192.168.50.0/24","dev":"enp7s0","protocol":"2","flags":["dead"]},
        {"dst":"192.168.60.0/24","flags":[],"nexthops":[{"gateway":"192.168.60.1","dev":"enp7s0","flags":["linkdown"]}]},
        {"dst":"192.168.88.10","dev":"enp7s0","protocol":"kernel","flags":[]}
    ])";

    MainRouteTable t;
    VERIFY(t.parse(json, types::IpAddress::IPv4));

    VERIFY(!t.hasSpecificRoute(r("192.168.88.1"), kWgDevs, kOwn, kPinDevs));
    VERIFY(!t.hasSpecificRoute(r("192.168.50.1"), kWgDevs, kOwn, kPinDevs));
    VERIFY(!t.hasSpecificRoute(r("192.168.60.1"), kWgDevs, kOwn, kPinDevs));
    // An up route for the same subnet still covers its own destination.
    VERIFY(t.hasSpecificRoute(r("192.168.88.10"), kWgDevs, kOwn, kPinDevs));
}

// A proto-boot (or proto 87) pin as specific as the destination and pointed at the default
// adapter is ours. It must not hide the system route under it, and it is reported so the
// caller can delete it. The same proto on another interface, and a kernel route on the pin
// interface, are the system's. Like the real `ip`, the dump has no protocol for proto boot.
void testLegacyPinDoesNotHideSystemRoute()
{
    const char *const json = R"([
        {"dst":"192.168.88.0/24","dev":"enp7s0","protocol":"2","flags":[]},
        {"dst":"192.168.88.1","gateway":"192.168.77.1","dev":"enp1s0","flags":[]},
        {"dst":"192.168.88.2","gateway":"192.168.77.1","dev":"enp1s0","protocol":"87","flags":[]},
        {"dst":"192.168.77.0/24","dev":"enp1s0","protocol":"kernel","metric":100,"flags":[]},
        {"dst":"192.168.77.0/24","gateway":"192.168.77.1","dev":"enp1s0","flags":[]},
        {"dst":"8.8.8.8","gateway":"192.168.77.1","dev":"enp1s0","flags":[]},
        {"dst":"1.1.1.1","gateway":"10.0.0.1","dev":"enp7s0","flags":[]}
    ])";

    MainRouteTable t;
    VERIFY(t.parse(json, types::IpAddress::IPv4));

    // The secondary /24 still wins over the leftover /32 pins.
    VERIFY(t.hasSpecificRoute(r("192.168.88.1"), kWgDevs, kOwn, kPinDevs));
    VERIFY(t.hasSpecificRoute(r("192.168.88.2"), kWgDevs, kOwn, kPinDevs));
    const auto bootPin = t.exactPins(r("192.168.88.1"), kPinDevs, kOwn);
    VERIFY(bootPin.size() == 1 && bootPin[0].dev == "enp1s0" && bootPin[0].protocol == "3");
    const auto ownPin = t.exactPins(r("192.168.88.2"), kPinDevs, kOwn);
    VERIFY(ownPin.size() == 1 && ownPin[0].dev == "enp1s0" && ownPin[0].protocol == "87");

    // Kernel connected route on the pin interface still covers; the boot /24 is the stale pin.
    VERIFY(t.hasSpecificRoute(r("192.168.77.0/24"), kWgDevs, kOwn, kPinDevs));
    VERIFY(t.hasSpecificRoute(r("192.168.77.20"), kWgDevs, kOwn, kPinDevs));
    const auto legacySubnet = t.exactPins(r("192.168.77.0/24"), kPinDevs, kOwn);
    VERIFY(legacySubnet.size() == 1 && legacySubnet[0].dev == "enp1s0" && legacySubnet[0].protocol == "3");
    VERIFY(t.exactPins(r("192.168.77.20"), kPinDevs, kOwn).empty());

    // Nothing but the leftover pin covers 8.8.8.8, so it still needs a pin — and the leftover is found.
    VERIFY(!t.hasSpecificRoute(r("8.8.8.8"), kWgDevs, kOwn, kPinDevs));
    const auto onlyPin = t.exactPins(r("8.8.8.8"), kPinDevs, kOwn);
    VERIFY(onlyPin.size() == 1 && onlyPin[0].dev == "enp1s0" && onlyPin[0].protocol == "3");

    // proto boot on the other interface is a real route, not ours to delete.
    VERIFY(t.hasSpecificRoute(r("1.1.1.1"), kWgDevs, kOwn, kPinDevs));
    VERIFY(t.exactPins(r("1.1.1.1"), kPinDevs, kOwn).empty());
}

void testMalformedInput()
{
    MainRouteTable t;
    VERIFY(!t.parse("", types::IpAddress::IPv4));
    VERIFY(!t.parse("Error: ipv4: FIB table does not exist.", types::IpAddress::IPv4));
    VERIFY(!t.parse("{}", types::IpAddress::IPv4));
    VERIFY(t.parse("[]", types::IpAddress::IPv4));
    // Entries of the wrong family or without a usable dst are skipped.
    VERIFY(t.parse(R"([{"dst":"2001:db8:88::/64","dev":"enp7s0"},{"dev":"enp7s0"},7])", types::IpAddress::IPv4));
    VERIFY(!t.hasSpecificRoute(r("2001:db8:88::1"), kWgDevs, kOwn));
}

} // namespace

int main()
{
    testWireGuardSecondaryLanIsCovered();
    testWireGuardDefaultRoutedNeedsPin();
    testOpenVpn();
    testOwnRoute();
    testNumericOutput();
    testBestRouteWins();
    testDownRouteIsNotCoverage();
    testLegacyPinDoesNotHideSystemRoute();
    testMalformedInput();

    if (g_failures == 0) {
        printf("All main route table tests passed\n");
    }
    return g_failures;
}
