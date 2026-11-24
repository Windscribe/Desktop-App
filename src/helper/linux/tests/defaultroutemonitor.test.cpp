// DefaultRouteMonitor::affectsDefaultGateway against hand-built netlink datagrams: only changes that can move the
// default gateway should trigger a check (each check runs `ip route`).

#include <cstdio>
#include <cstring>
#include <vector>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

#include "../../common/defaultroutemonitor_posix.h"

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

void appendRoute(std::vector<char> &buf, uint16_t type, uint8_t family, uint8_t dstLen, uint8_t table)
{
    const size_t start = buf.size();
    buf.resize(start + NLMSG_SPACE(sizeof(rtmsg)), 0);
    auto *hdr = reinterpret_cast<nlmsghdr *>(buf.data() + start);
    hdr->nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
    hdr->nlmsg_type = type;
    auto *rt = static_cast<rtmsg *>(NLMSG_DATA(hdr));
    rt->rtm_family = family;
    rt->rtm_dst_len = dstLen;
    rt->rtm_table = table;
}

void appendAddr(std::vector<char> &buf, uint16_t type, uint8_t family)
{
    const size_t start = buf.size();
    buf.resize(start + NLMSG_SPACE(sizeof(ifaddrmsg)), 0);
    auto *hdr = reinterpret_cast<nlmsghdr *>(buf.data() + start);
    hdr->nlmsg_len = NLMSG_LENGTH(sizeof(ifaddrmsg));
    hdr->nlmsg_type = type;
    static_cast<ifaddrmsg *>(NLMSG_DATA(hdr))->ifa_family = family;
}

void appendLink(std::vector<char> &buf, uint16_t type, bool wireless = false)
{
    const size_t start = buf.size();
    const size_t attrSpace = wireless ? RTA_SPACE(4) : 0;
    buf.resize(start + NLMSG_SPACE(sizeof(ifinfomsg) + attrSpace), 0);
    auto *hdr = reinterpret_cast<nlmsghdr *>(buf.data() + start);
    hdr->nlmsg_len = NLMSG_LENGTH(sizeof(ifinfomsg) + attrSpace);
    hdr->nlmsg_type = type;
    if (wireless) {
        auto *rta = IFLA_RTA(static_cast<ifinfomsg *>(NLMSG_DATA(hdr)));
        rta->rta_type = IFLA_WIRELESS;
        rta->rta_len = RTA_LENGTH(4);
    }
}

bool affects(const std::vector<char> &buf, bool truncated = false)
{
    return DefaultRouteMonitor::affectsDefaultGateway(buf.data(), buf.size(), truncated);
}

void testDefaultRouteInMainTable()
{
    std::vector<char> b;
    appendRoute(b, RTM_NEWROUTE, AF_INET, 0, RT_TABLE_MAIN);
    VERIFY(affects(b));
    b.clear();
    appendRoute(b, RTM_DELROUTE, AF_INET, 0, RT_TABLE_MAIN);
    VERIFY(affects(b));
}

void testOtherRoutesIgnored()
{
    std::vector<char> b;
    appendRoute(b, RTM_NEWROUTE, AF_INET, 32, RT_TABLE_MAIN);   // the endpoint route the monitor itself adds
    VERIFY(!affects(b));
    b.clear();
    appendRoute(b, RTM_DELROUTE, AF_INET, 32, RT_TABLE_MAIN);   // a host route deleted: maybe the endpoint route
    VERIFY(affects(b));
    b.clear();
    appendRoute(b, RTM_DELROUTE, AF_INET, 32, RT_TABLE_LOCAL);
    VERIFY(!affects(b));
    b.clear();
    appendRoute(b, RTM_NEWROUTE, AF_INET, 0, RT_TABLE_COMPAT);   // the tunnel's table (ids > 255 are reported as 252)
    VERIFY(!affects(b));
    b.clear();
    appendRoute(b, RTM_NEWROUTE, AF_INET, 0, RT_TABLE_LOCAL);
    VERIFY(!affects(b));
    b.clear();
    appendRoute(b, RTM_NEWROUTE, AF_INET6, 0, RT_TABLE_MAIN);
    VERIFY(!affects(b));
    b.clear();
    appendRoute(b, RTM_NEWROUTE, AF_INET, 24, RT_TABLE_MAIN);   // e.g. a Docker bridge
    VERIFY(!affects(b));
}

void testAddressChanges()
{
    std::vector<char> b;
    appendAddr(b, RTM_DELADDR, AF_INET);
    VERIFY(affects(b));
    b.clear();
    appendAddr(b, RTM_NEWADDR, AF_INET);
    VERIFY(affects(b));
    b.clear();
    appendAddr(b, RTM_NEWADDR, AF_INET6);
    VERIFY(!affects(b));
}

void testLinkChanges()
{
    // `ip link set eth0 down` flushes eth0's routes without route messages; only the link message reports it.
    std::vector<char> b;
    appendLink(b, RTM_NEWLINK);
    VERIFY(affects(b));
    b.clear();
    appendLink(b, RTM_DELLINK);
    VERIFY(affects(b));

    // Wireless-extension events (Wi-Fi scans and roams) arrive as RTM_NEWLINK many times a minute and are ignored.
    b.clear();
    appendLink(b, RTM_NEWLINK, true);
    VERIFY(!affects(b));
    appendLink(b, RTM_NEWLINK);   // followed by a real link change in the same datagram
    VERIFY(affects(b));
}

void testMultipartAndEdgeCases()
{
    // A relevant message after irrelevant ones in the same datagram.
    std::vector<char> b;
    appendRoute(b, RTM_NEWROUTE, AF_INET, 24, RT_TABLE_MAIN);
    appendAddr(b, RTM_NEWADDR, AF_INET6);
    appendRoute(b, RTM_DELROUTE, AF_INET, 0, RT_TABLE_MAIN);
    VERIFY(affects(b));

    VERIFY(!affects({}));
    VERIFY(affects({}, true));   // truncated: assume the worst

    // A header claiming more payload than present stops the walk instead of reading past the end.
    std::vector<char> shortMsg;
    appendRoute(shortMsg, RTM_NEWROUTE, AF_INET, 0, RT_TABLE_MAIN);
    reinterpret_cast<nlmsghdr *>(shortMsg.data())->nlmsg_len = static_cast<uint32_t>(shortMsg.size() + 64);
    VERIFY(!affects(shortMsg));

    // A route message too short to hold an rtmsg is skipped.
    std::vector<char> tiny(NLMSG_SPACE(0), 0);
    reinterpret_cast<nlmsghdr *>(tiny.data())->nlmsg_len = NLMSG_LENGTH(0);
    reinterpret_cast<nlmsghdr *>(tiny.data())->nlmsg_type = RTM_NEWROUTE;
    VERIFY(!affects(tiny));
}

} // namespace

int main()
{
    testDefaultRouteInMainTable();
    testOtherRoutesIgnored();
    testAddressChanges();
    testLinkChanges();
    testMultipartAndEdgeCases();
    if (g_failures) {
        fprintf(stderr, "%d failure(s)\n", g_failures);
        return 1;
    }
    printf("defaultroutemonitor: all tests passed\n");
    return 0;
}
