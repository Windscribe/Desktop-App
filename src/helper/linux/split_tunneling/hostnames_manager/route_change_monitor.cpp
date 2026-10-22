#include "route_change_monitor.h"

#include <cerrno>
#include <cstring>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

namespace {

// How long the netlink stream must stay quiet before the callback runs.
constexpr int kQuietPeriodMs = 500;
// How often a stop request is noticed while idle.
constexpr int kIdlePollMs = 250;

// Table a route message refers to; RTA_TABLE carries ids that do not fit rtm_table.
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

// Reads everything queued on the socket. True when any of it is a change to a `main` route other
// than our own, or messages were lost (ENOBUFS), after which the table has to be re-read anyway.
bool drain(int fd, uint8_t ignoredProtocol)
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
                && msg->nlmsg_len >= NLMSG_LENGTH(sizeof(rtmsg)) && routeTable(msg) == RT_TABLE_MAIN
                && static_cast<const rtmsg *>(NLMSG_DATA(msg))->rtm_protocol != ignoredProtocol) {
                relevant = true;
            }
        }
    }
    return relevant;
}

} // namespace

RouteChangeMonitor::RouteChangeMonitor(std::function<void()> onChanged, uint8_t ignoredRouteProtocol)
    : onChanged_(std::move(onChanged)), ignoredRouteProtocol_(ignoredRouteProtocol), stopRequested_(false)
{
}

RouteChangeMonitor::~RouteChangeMonitor()
{
    stop();
}

void RouteChangeMonitor::start()
{
    if (thread_.joinable()) {
        return;
    }

    const int fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
    if (fd < 0) {
        spdlog::error("RouteChangeMonitor: could not open a netlink socket: {}", strerror(errno));
        return;
    }
    sockaddr_nl addr = {};
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE;
    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        spdlog::error("RouteChangeMonitor: could not subscribe to route changes: {}", strerror(errno));
        close(fd);
        return;
    }

    stopRequested_ = false;
    thread_ = std::thread(&RouteChangeMonitor::run, this, fd);
}

void RouteChangeMonitor::stop()
{
    if (!thread_.joinable()) {
        return;
    }
    stopRequested_ = true;
    thread_.join();
}

void RouteChangeMonitor::run(int fd)
{
    bool pending = false;
    while (!stopRequested_) {
        // While a change is pending, a poll timeout means the stream has been quiet long enough.
        pollfd pfd = {fd, POLLIN, 0};
        const int rc = poll(&pfd, 1, pending ? kQuietPeriodMs : kIdlePollMs);
        // POLLERR too: after an overrun (ENOBUFS) only a recv() clears the error, otherwise poll
        // returns at once forever.
        if (rc > 0 && (pfd.revents & (POLLIN | POLLERR))) {
            pending = drain(fd, ignoredRouteProtocol_) || pending;
        } else if (rc == 0 && pending) {
            pending = false;
            onChanged_();
        }
    }
    close(fd);
}
