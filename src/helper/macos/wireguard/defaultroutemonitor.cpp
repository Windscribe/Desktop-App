#include "defaultroutemonitor.h"
#include "../utils.h"
#include <spdlog/spdlog.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <net/route.h>
#include <poll.h>

namespace
{
void RouteMonitorThread(void *callerContext)
{
    auto *this_ = static_cast<DefaultRouteMonitor *>(callerContext);
    int socket_fd = socket(PF_ROUTE, SOCK_RAW, AF_INET);
    if (socket_fd < 0) {
        spdlog::error("Failed to open PF_ROUTE socket");
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
        if (n >= sizeof(struct rt_msghdr)) {
            const auto *msg = reinterpret_cast<const struct rt_msghdr*>(message);
            if (msg->rtm_version == RTM_VERSION) {
                if (!this_->checkDefaultRoutes())
                    break;
            }
        }
    }
    spdlog::info("Default route monitoring finished");
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

    // In the case where we are switching split tunnel modes, the gateway may temporarily be empty. Ignore this.
    // If the gateway actually legitimately is empty, the main client will detect lack of internet connectivity.
    if (newGateway.empty()) {
        return true;
    }

    unsetEndpointDirectRoute();
    lastGateway_ = newGateway;
    return setEndpointDirectRoute();
}

bool DefaultRouteMonitor::executeCommandWithLogging(const std::string &command) const
{
    std::string output;
    const auto status = Utils::executeCommand(command, {}, &output);
    if (!output.empty())
        spdlog::info("{}", output);
    return status == 0;
}

std::string DefaultRouteMonitor::getDefaultGateway() const
{
    std::string output;
    const auto status = Utils::executeCommand(
        "netstat -nr -f inet | grep 'default' | awk '{print $2}'", {}, &output);
    if (status == 0) {
        std::vector<std::string> gateways;
        boost::trim(output);
        boost::split(gateways, output, boost::is_any_of("\n"), boost::token_compress_on);
        if (!gateways.empty() && !gateways[0].empty())
            return gateways[0];
    }
    spdlog::error("Failed to get default gateway ({})", output);
    return "";
}

bool DefaultRouteMonitor::setEndpointDirectRoute()
{
    if (endpoint_.empty() || lastGateway_.empty())
        return false;
    if (!executeCommandWithLogging(
        "route -q -n add -inet " + endpoint_ + " -gateway " + lastGateway_))
        return false;
    return true;
}

void DefaultRouteMonitor::unsetEndpointDirectRoute()
{
    if (endpoint_.empty())
        return;
    executeCommandWithLogging("route -q -n delete -inet " + endpoint_);
}
