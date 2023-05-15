#include "defaultroutemonitor.h"
#include "../logger.h"
#include "../utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <poll.h>

namespace
{
void RouteMonitorThread(void *callerContext)
{
    auto *this_ = static_cast<DefaultRouteMonitor *>(callerContext);
    int socket_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (socket_fd < 0) {
        Logger::instance().out("Failed to open PF_NETLINK socket");
        return;
    }
    char message[2048];
    const int kPollTimeoutMs = 250;
    while (this_->isActive()) {
        struct pollfd pfd;
        pfd.fd = socket_fd;
        pfd.events = POLLIN;
        const auto ret = poll(&pfd, 1, kPollTimeoutMs);
        if (ret <= 0)
            continue;
        const auto n = read(socket_fd, message, sizeof(message));
        if (n <= 0)
            continue;
        if ((size_t)n >= sizeof(nlmsghdr)) {
            const auto *msg = reinterpret_cast<const nlmsghdr*>(message);
            if (NLMSG_OK(msg, n)) {
                if (!this_->checkDefaultRoutes())
                    break;
            }
        }
    }
    Logger::instance().out("Default route monitoring finished");
}
}  // namespace

DefaultRouteMonitor::DefaultRouteMonitor(const std::string &deviceName)
    : deviceName_(deviceName), monitorThread_(nullptr), doStopThread_(false)
{
}

DefaultRouteMonitor::~DefaultRouteMonitor()
{
    stop();
}

bool DefaultRouteMonitor::start(const std::string &endpoint)
{
    std::vector<std::string> endpoint_parts;
    boost::split(endpoint_parts, endpoint, boost::is_any_of(":\\/"), boost::token_compress_on);
    if (endpoint_parts[0] != endpoint_) {
        stop();
        endpoint_ = endpoint_parts[0];
    }
    if (!monitorThread_) {
        doStopThread_ = false;
        monitorThread_ = new std::thread(RouteMonitorThread, this);
    }
    return checkDefaultRoutes();
}

void DefaultRouteMonitor::stop()
{
    if (monitorThread_) {
        doStopThread_ = true;
        monitorThread_->join();
        delete monitorThread_;
        monitorThread_ = nullptr;
    }
    unsetEndpointDirectRoute();
}

bool DefaultRouteMonitor::checkDefaultRoutes()
{
    auto newGateway = getDefaultGateway();
    if (newGateway == lastGateway_)
        return true;

    unsetEndpointDirectRoute();
    lastGateway_ = newGateway;
    return setEndpointDirectRoute();
}

bool DefaultRouteMonitor::executeCommandWithLogging(const std::string &command) const
{
    std::string output;
    const auto status = Utils::executeCommand(command, {}, &output);
    if (!output.empty())
        Logger::instance().out("%s", output.c_str());
    return status == 0;
}

std::string DefaultRouteMonitor::getDefaultGateway() const
{
    std::string output;
    const auto status = Utils::executeCommand(
        "ip route | grep 'default' | awk '{print $3}'", {}, &output);
    if (status == 0) {
        std::vector<std::string> gateways;
        boost::trim(output);
        boost::split(gateways, output, boost::is_any_of("\n"), boost::token_compress_on);
        if (!gateways.empty() && !gateways[0].empty())
            return gateways[0];
    }
    Logger::instance().out("Failed to get default gateway (%s)", output.c_str());
    return "";
}

bool DefaultRouteMonitor::setEndpointDirectRoute()
{
    if (endpoint_.empty() || lastGateway_.empty())
        return false;
    if (!executeCommandWithLogging(
        "ip route add " + endpoint_ + "/32 via " + lastGateway_))
        return false;
    return true;
}

void DefaultRouteMonitor::unsetEndpointDirectRoute()
{
    if (endpoint_.empty())
        return;
    executeCommandWithLogging("ip route del " + endpoint_);
}
