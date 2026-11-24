#include <cstdio>
#include <cstring>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "../netlink_route_linux.h"

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

struct Route
{
    uint16_t type = RTM_NEWROUTE;
    unsigned char dstLen = 0;
    unsigned char table = RT_TABLE_MAIN;
    int tableAttr = -1;
    bool truncated = false;
};

std::vector<char> message(const Route &route)
{
    std::vector<char> buf(NLMSG_SPACE(sizeof(rtmsg) + RTA_SPACE(sizeof(uint32_t))), 0);
    auto *hdr = reinterpret_cast<nlmsghdr *>(buf.data());
    hdr->nlmsg_type = route.type;
    hdr->nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
    auto *rtm = static_cast<rtmsg *>(NLMSG_DATA(hdr));
    rtm->rtm_family = AF_INET;
    rtm->rtm_dst_len = route.dstLen;
    rtm->rtm_table = route.table;
    if (route.tableAttr >= 0) {
        auto *attr = reinterpret_cast<rtattr *>(reinterpret_cast<char *>(hdr) + NLMSG_ALIGN(hdr->nlmsg_len));
        attr->rta_type = RTA_TABLE;
        attr->rta_len = RTA_LENGTH(sizeof(uint32_t));
        const uint32_t table = static_cast<uint32_t>(route.tableAttr);
        std::memcpy(RTA_DATA(attr), &table, sizeof(table));
        hdr->nlmsg_len = NLMSG_ALIGN(hdr->nlmsg_len) + RTA_SPACE(sizeof(uint32_t));
    }
    if (route.truncated) {
        hdr->nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg) - 1);
    }
    buf.resize(hdr->nlmsg_len);
    return buf;
}

bool drained(const std::vector<Route> &routes)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) != 0) {
        return false;
    }
    for (const auto &route : routes) {
        const auto buf = message(route);
        if (send(fds[1], buf.data(), buf.size(), 0) != static_cast<ssize_t>(buf.size())) {
            printf("send failed\n");
        }
    }
    const bool relevant = NetlinkRoute::drain(fds[0], [](const nlmsghdr *msg) {
        return static_cast<const rtmsg *>(NLMSG_DATA(msg))->rtm_dst_len == 0 && NetlinkRoute::routeTable(msg) == RT_TABLE_MAIN;
    });
    char probe;
    const bool empty = recv(fds[0], &probe, 1, MSG_DONTWAIT) < 0;
    close(fds[0]);
    close(fds[1]);
    VERIFY(empty);
    return relevant;
}

void testDefaultRouteChanges()
{
    VERIFY(drained({Route{}}));
    VERIFY(drained({Route{RTM_DELROUTE}}));
    VERIFY(drained({Route{RTM_NEWROUTE, 0, RT_TABLE_COMPAT, RT_TABLE_MAIN}}));
}

void testOtherMessagesAreIgnored()
{
    VERIFY(!drained({}));
    VERIFY(!drained({Route{RTM_NEWROUTE, 32}}));
    VERIFY(!drained({Route{RTM_NEWROUTE, 0, 200}}));
    VERIFY(!drained({Route{RTM_NEWROUTE, 0, RT_TABLE_MAIN, 200}}));
    VERIFY(!drained({Route{RTM_NEWLINK}}));
    VERIFY(!drained({Route{RTM_NEWROUTE, 0, RT_TABLE_MAIN, -1, true}}));
}

void testOneDefaultChangeInABatchIsEnough()
{
    VERIFY(drained({Route{RTM_NEWROUTE, 32}, Route{RTM_NEWROUTE, 0, 200}, Route{}, Route{RTM_DELROUTE, 32}}));
}

} // namespace

int main()
{
    testDefaultRouteChanges();
    testOtherMessagesAreIgnored();
    testOneDefaultChangeInABatchIsEnough();
    if (g_failures != 0) {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}
