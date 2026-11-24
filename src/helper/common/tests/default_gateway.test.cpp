#include <cstdio>
#include <string>

#include "../default_gateway_posix.h"

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

void testMacNetstat()
{
    const std::string output =
        "Routing tables\n"
        "\n"
        "Internet:\n"
        "Destination        Gateway            Flags               Netif Expire\n"
        "default            link#22            UCSIg               utun420\n"
        "default            192.168.1.1        UGScg                 en0\n"
        "default            10.0.0.1           UGScIg                en1\n"
        "127                127.0.0.1          UCS                   lo0\n";
    VERIFY(DefaultGateway::fromMacNetstat(output) == "192.168.1.1");
    VERIFY(DefaultGateway::fromMacNetstat("Destination        Gateway            Flags\n"
                                          "192.168.1         link#14            UCS                   en0\n").empty());
    VERIFY(DefaultGateway::fromMacNetstat("defaultroute       192.168.1.1        UGScg                 en0\n").empty());
    VERIFY(DefaultGateway::fromMacNetstat("").empty());
}

void testLinuxIpRoute()
{
    VERIFY(DefaultGateway::fromLinuxIpRoute("default via 192.168.1.1 dev eth0 proto dhcp metric 100 \n"
                                            "default via 10.0.0.1 dev wlan0 proto dhcp metric 600 \n"
                                            "192.168.1.0/24 dev eth0 proto kernel scope link src 192.168.1.20\n") == "192.168.1.1");
    VERIFY(DefaultGateway::fromLinuxIpRoute("default dev wg0 scope link \n"
                                            "default via 10.0.0.1 dev wlan0 metric 600 \n") == "10.0.0.1");
    VERIFY(DefaultGateway::fromLinuxIpRoute("default proto static metric 100 \n"
                                            "\tnexthop via 192.168.1.1 dev eth0 weight 1 \n"
                                            "\tnexthop via 192.168.2.1 dev eth1 weight 1 \n").empty());
    VERIFY(DefaultGateway::fromLinuxIpRoute("203.0.113.10 via 192.168.1.1 dev eth0 \n").empty());
    VERIFY(DefaultGateway::fromLinuxIpRoute("").empty());
}

} // namespace

int main()
{
    testMacNetstat();
    testLinuxIpRoute();
    if (g_failures != 0) {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}
