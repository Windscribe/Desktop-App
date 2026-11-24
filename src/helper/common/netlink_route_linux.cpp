#include "netlink_route_linux.h"

#include <cerrno>
#include <cstring>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

namespace NetlinkRoute {

uint32_t routeTable(const nlmsghdr *msg)
{
    const auto *rtm = static_cast<const rtmsg *>(NLMSG_DATA(msg));
    uint32_t table = rtm->rtm_table;
    int len = RTM_PAYLOAD(msg);
    for (const rtattr *attr = RTM_RTA(rtm); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        if (attr->rta_type == RTA_TABLE && RTA_PAYLOAD(attr) >= sizeof(uint32_t)) {
            std::memcpy(&table, RTA_DATA(attr), sizeof(uint32_t));
        }
    }
    return table;
}

bool drain(int fd, const std::function<bool(const nlmsghdr *msg)> &isRelevant)
{
    bool relevant = false;
    char buf[32768];
    while (true) {
        const ssize_t n = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOBUFS) {
                relevant = true;
                continue;
            }
            break;  // EAGAIN: drained.
        }
        if (n == 0) {
            break;
        }
        int len = static_cast<int>(n);
        for (auto *msg = reinterpret_cast<const nlmsghdr *>(buf); NLMSG_OK(msg, len); msg = NLMSG_NEXT(msg, len)) {
            if ((msg->nlmsg_type == RTM_NEWROUTE || msg->nlmsg_type == RTM_DELROUTE)
                && msg->nlmsg_len >= NLMSG_LENGTH(sizeof(rtmsg)) && isRelevant(msg)) {
                relevant = true;
            }
        }
    }
    return relevant;
}

} // namespace NetlinkRoute
