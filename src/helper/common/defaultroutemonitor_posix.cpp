#include "defaultroutemonitor_posix.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>

#include "default_gateway_posix.h"
#include "utils_posix.h"

#if defined(__APPLE__)
#include <net/route.h>
#else
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

namespace
{
#if defined(__APPLE__)
constexpr auto kSettle = std::chrono::milliseconds(0);
#else
// Relevant changes come in bursts (a network switch: link, address and route messages over a second or two). Check
// once things are quiet for kSettle, so a burst costs one check and none runs mid-switch.
constexpr auto kSettle = std::chrono::milliseconds(200);
#endif
// Bounds the wait if relevant messages never stop.
constexpr auto kMaxDelay = std::chrono::seconds(2);

int openRouteSocket()
{
#if defined(__APPLE__)
    const int fd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
    if (fd >= 0) {
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
    return fd;
#else
    const int fd = socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
    if (fd < 0) {
        return fd;
    }
    // Without joining these groups the kernel sends this socket nothing, and a gateway change would never be seen.
    sockaddr_nl addr{};
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_IFADDR;
    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        spdlog::error("Failed to subscribe to route changes: errno {}", errno);
        close(fd);
        return -1;
    }
    return fd;
#endif
}

// Reads everything queued on the routing socket; true if any of it can move the default gateway.
bool drainRouteChanges(int fd)
{
    bool changed = false;
#if defined(__APPLE__)
    char message[2048];
#else
    alignas(nlmsghdr) char message[8192];
#endif
    for (;;) {
#if defined(__APPLE__)
        const ssize_t n = recv(fd, message, sizeof(message), MSG_DONTWAIT);
#else
        const ssize_t n = recv(fd, message, sizeof(message), MSG_DONTWAIT | MSG_TRUNC);
#endif
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            // The kernel dropped messages because this socket fell behind; one of them may have moved the gateway.
            if (errno == ENOBUFS) {
                changed = true;
                continue;
            }
            break;
        }
        if (n == 0) {
            break;
        }
#if defined(__APPLE__)
        const auto *rtm = reinterpret_cast<const struct rt_msghdr *>(message);
        if (n >= static_cast<ssize_t>(sizeof(struct rt_msghdr)) && rtm->rtm_version == RTM_VERSION && rtm->rtm_type != RTM_GET) {
            changed = true;
        }
#else
        const bool truncated = static_cast<size_t>(n) > sizeof(message);
        if (DefaultRouteMonitor::affectsDefaultGateway(message, truncated ? sizeof(message) : static_cast<size_t>(n), truncated)) {
            changed = true;
        }
#endif
    }
    return changed;
}

} // namespace

DefaultRouteMonitor::DefaultRouteMonitor()
{
#if defined(__linux__)
    const bool created = pipe2(wakePipe_, O_CLOEXEC) == 0;
#else
    const bool created = pipe(wakePipe_) == 0;
    if (created) {
        fcntl(wakePipe_[0], F_SETFD, FD_CLOEXEC);
        fcntl(wakePipe_[1], F_SETFD, FD_CLOEXEC);
    }
#endif
    if (!created) {
        spdlog::error("DefaultRouteMonitor: pipe failed");
        wakePipe_[0] = wakePipe_[1] = -1;
    }
}

DefaultRouteMonitor::~DefaultRouteMonitor()
{
    stop();
    for (int fd : wakePipe_) {
        if (fd >= 0) {
            close(fd);
        }
    }
}

void DefaultRouteMonitor::monitorLoop()
{
    const int socketFd = openRouteSocket();
    if (socketFd < 0) {
        spdlog::error("Failed to open the routing socket");
        return;
    }
    const int kPollTimeoutMs = 250;
    bool checkPending = false;
    std::chrono::steady_clock::time_point checkDue;
    std::chrono::steady_clock::time_point burstStart;
    while (!doStopThread_) {
        int timeoutMs = kPollTimeoutMs;
        if (checkPending) {
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(checkDue - std::chrono::steady_clock::now());
            using Rep = std::chrono::milliseconds::rep;
            timeoutMs = static_cast<int>(std::max<Rep>(0, std::min<Rep>(left.count(), kPollTimeoutMs)));
        }
        struct pollfd pfd[2];
        pfd[0].fd = socketFd;
        pfd[0].events = POLLIN;
        pfd[0].revents = 0;
        pfd[1].fd = wakePipe_[0];
        pfd[1].events = POLLIN;
        pfd[1].revents = 0;
        if (poll(pfd, 2, timeoutMs) > 0) {
            if (pfd[1].revents & POLLIN) {
                char drained;
                if (read(pfd[1].fd, &drained, 1) < 0) {
                    spdlog::warn("DefaultRouteMonitor: could not read the wake pipe: errno {}", errno);
                }
                continue;
            }
            if ((pfd[0].revents & (POLLIN | POLLERR)) && drainRouteChanges(socketFd)) {
                const auto now = std::chrono::steady_clock::now();
                if (!checkPending) {
                    checkPending = true;
                    burstStart = now;
                }
                checkDue = std::min(now + kSettle, burstStart + kMaxDelay);
            }
        }
        // A failure is usually a moment without a default route while the network changes; keep watching so the route
        // is restored once a gateway is back.
        if (checkPending && std::chrono::steady_clock::now() >= checkDue) {
            checkPending = false;
            if (!checkDefaultRoutes()) {
                spdlog::warn("Default route monitor: no direct route to the endpoint for now");
            }
        }
    }
    close(socketFd);
    spdlog::debug("Default route monitoring finished");
}

#if defined(__linux__)
// static
bool DefaultRouteMonitor::affectsDefaultGateway(const void *buf, size_t len, bool truncated)
{
    if (truncated) {
        return true;
    }
    int remaining = static_cast<int>(len);
    for (auto *msg = static_cast<const nlmsghdr *>(buf); NLMSG_OK(msg, remaining); msg = NLMSG_NEXT(msg, remaining)) {
        switch (msg->nlmsg_type) {
        case RTM_NEWROUTE:
        case RTM_DELROUTE: {
            if (msg->nlmsg_len < NLMSG_LENGTH(sizeof(rtmsg))) {
                break;
            }
            const auto *rt = static_cast<const rtmsg *>(NLMSG_DATA(msg));
            if (rt->rtm_family != AF_INET || rt->rtm_table != RT_TABLE_MAIN) {
                break;
            }
            // A default route changing, or a host route being deleted: the endpoint route may be the one gone (e.g.
            // NetworkManager reapplying a connection's routes while the gateway stays the same). Host-route deletions
            // are rare, and the check puts the endpoint route back.
            if (rt->rtm_dst_len == 0 || (msg->nlmsg_type == RTM_DELROUTE && rt->rtm_dst_len == 32)) {
                return true;
            }
            break;
        }
        case RTM_DELLINK:
            return true;
        case RTM_NEWLINK: {
            // A link going down drops its routes without any route message (and keeps its addresses). Wireless
            // extension events (scans, roams) also arrive as RTM_NEWLINK, many per minute; those carry IFLA_WIRELESS
            // and never change routing.
            if (msg->nlmsg_len < NLMSG_LENGTH(sizeof(ifinfomsg))) {
                break;
            }
            bool wireless = false;
            int attrLen = static_cast<int>(IFLA_PAYLOAD(msg));
            for (auto *rta = IFLA_RTA(static_cast<const ifinfomsg *>(NLMSG_DATA(msg))); RTA_OK(rta, attrLen);
                 rta = RTA_NEXT(rta, attrLen)) {
                if (rta->rta_type == IFLA_WIRELESS) {
                    wireless = true;
                    break;
                }
            }
            if (!wireless) {
                return true;
            }
            break;
        }
        case RTM_NEWADDR:
        case RTM_DELADDR: {
            if (msg->nlmsg_len < NLMSG_LENGTH(sizeof(ifaddrmsg))) {
                break;
            }
            if (static_cast<const ifaddrmsg *>(NLMSG_DATA(msg))->ifa_family == AF_INET) {
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return false;
}
#endif

bool DefaultRouteMonitor::start(const std::string &endpoint)
{
    std::vector<std::string> endpoint_parts;
    boost::split(endpoint_parts, endpoint, boost::is_any_of(":\\/"), boost::token_compress_on); // NOLINT: false positive
    if (endpoint_parts[0] != endpoint_) {
        stop();
        endpoint_ = endpoint_parts[0];
        // The gateway is unchanged, but the new endpoint has no route through it yet.
        lastGateway_.clear();
    }
    if (!monitorThread_.joinable()) {
        doStopThread_ = false;
        monitorThread_ = std::thread(&DefaultRouteMonitor::monitorLoop, this);
    }
    return checkDefaultRoutes();
}

void DefaultRouteMonitor::stop()
{
    if (monitorThread_.joinable()) {
        doStopThread_ = true;
        if (wakePipe_[1] >= 0) {
            const char wake = 0;
            if (write(wakePipe_[1], &wake, 1) != 1) {
                spdlog::warn("DefaultRouteMonitor: could not write the wake pipe: errno {}", errno);
            }
        }
        monitorThread_.join();
    }
    unsetEndpointDirectRoute();
}

bool DefaultRouteMonitor::checkDefaultRoutes()
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string newGateway = getDefaultGateway();
    // In the case where we are switching split tunnel modes, the gateway may temporarily be empty. Ignore this.
    // If the gateway actually legitimately is empty, the main client will detect lack of internet connectivity.
    if (newGateway.empty()) {
        return true;
    }
    if (newGateway == lastGateway_) {
#if defined(__linux__)
        // Same gateway, but the endpoint route may still be gone: a link that went down and came back up within one
        // check took its routes with it. Put it back if so.
        if (!endpoint_.empty()) {
            std::string output;
            Utils::executeCommand("ip", {"route", "show", "exact", endpoint_ + "/32"}, &output, false);
            boost::trim(output);
            if (output.empty()) {
                return setEndpointDirectRoute();
            }
        }
#endif
        return true;
    }

    unsetEndpointDirectRoute();
    lastGateway_ = newGateway;
    return setEndpointDirectRoute();
}

bool DefaultRouteMonitor::executeCommandWithLogging(const std::string &program, const std::vector<std::string> &args) const
{
    std::string output;
    const auto status = Utils::executeCommand(program, args, &output);
    if (!output.empty())
        spdlog::info("{}", output);
    return status == 0;
}

std::string DefaultRouteMonitor::getDefaultGateway() const
{
    std::string output;
#if defined(__APPLE__)
    const auto status = Utils::executeCommand("netstat", {"-nr", "-f", "inet"}, &output, false);
    const std::string gateway = status == 0 ? DefaultGateway::fromMacNetstat(output) : std::string();
#else
    const auto status = Utils::executeCommand("ip", {"-4", "route", "show", "table", "main"}, &output, false);
    const std::string gateway = status == 0 ? DefaultGateway::fromLinuxIpRoute(output) : std::string();
#endif
    if (gateway.empty()) {
        spdlog::warn("Failed to get default gateway (exit status {}): {}", status, output);
    }
    return gateway;
}

bool DefaultRouteMonitor::setEndpointDirectRoute()
{
    if (endpoint_.empty() || lastGateway_.empty())
        return false;
#if defined(__APPLE__)
    return executeCommandWithLogging("route", {"-q", "-n", "add", "-inet", endpoint_, "-gateway", lastGateway_});
#else
    return executeCommandWithLogging("ip", {"route", "add", endpoint_ + "/32", "via", lastGateway_});
#endif
}

void DefaultRouteMonitor::unsetEndpointDirectRoute()
{
    if (endpoint_.empty())
        return;
#if defined(__APPLE__)
    executeCommandWithLogging("route", {"-q", "-n", "delete", "-inet", endpoint_});
#else
    executeCommandWithLogging("ip", {"route", "del", endpoint_});
#endif
}
