#include "routeeventfilter_linux.h"

#include <cerrno>
#include <cstring>
#include <iterator>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <utility>

namespace {

constexpr auto kTrustWindow = std::chrono::seconds(2);

bool isTunnelName(const std::string &name)
{
    return name.compare(0, 3, "tun") == 0 || name.compare(0, 4, "utun") == 0;
}

}  // namespace

DefaultRouteTracker_linux::DefaultRouteTracker_linux(Reader reader) : reader_(std::move(reader))
{
}

DefaultRoute_linux DefaultRouteTracker_linux::read()
{
    lastRead_ = reader_();
    return *lastRead_;
}

bool DefaultRouteTracker_linux::differsFromLastRead() const
{
    return !lastRead_ || !(*lastRead_ == reader_());
}

RouteEventFilter_linux::RouteEventFilter_linux(NameLookup nameLookup, Clock clock)
    : nameLookup_(std::move(nameLookup)), clock_(std::move(clock))
{
}

RouteEventFilter_linux::Result RouteEventFilter_linux::classify(const char *buffer, ssize_t len, size_t bufferSize)
{
    if (len <= 0 || static_cast<size_t>(len) > bufferSize) {
        return Result::Refresh;
    }
    bool refresh = false;
    bool tunnel = false;
    int remaining = static_cast<int>(len);
    for (const nlmsghdr *nh = reinterpret_cast<const nlmsghdr *>(buffer); NLMSG_OK(nh, remaining);
         nh = NLMSG_NEXT(nh, remaining)) {
        if ((nh->nlmsg_type != RTM_NEWROUTE && nh->nlmsg_type != RTM_DELROUTE) || nh->nlmsg_len < NLMSG_LENGTH(sizeof(rtmsg))) {
            refresh = true;
            continue;
        }
        int oif = 0;
        int attrLen = RTM_PAYLOAD(nh);
        for (const rtattr *rta = RTM_RTA(NLMSG_DATA(nh)); RTA_OK(rta, attrLen); rta = RTA_NEXT(rta, attrLen)) {
            if (rta->rta_type == RTA_OIF && RTA_PAYLOAD(rta) >= sizeof(oif)) {
                memcpy(&oif, RTA_DATA(rta), sizeof(oif));
            }
        }
        if (isTunnelIndex(oif)) {
            tunnel = true;
        } else {
            refresh = true;
        }
    }
    return (refresh || !tunnel || remaining > 0) ? Result::Refresh : Result::TunnelOnly;
}

void RouteEventFilter_linux::processWakeup(const ReceiveCallback &receive, const Callback &refresh, const Callback &tunnelRoutesChanged)
{
    bool tunnelOnly = false;
    for (int i = 0; i < kMaxReadsPerWakeup; ++i) {
        alignas(nlmsghdr) char buffer[4096];
        int error = 0;
        const ssize_t len = receive(buffer, sizeof(buffer), error);
        if (len < 0 && error != ENOBUFS) {
            break;
        }
        if (len >= 0 && classify(buffer, len, sizeof(buffer)) == Result::TunnelOnly) {
            tunnelOnly = true;
        } else {
            refresh();
        }
    }
    if (tunnelOnly) {
        tunnelRoutesChanged();
    }
}

bool RouteEventFilter_linux::isTunnelIndex(int index)
{
    if (index <= 0) {
        return false;
    }
    const auto now = clock_();
    std::string name;
    if (!nameLookup_(index, name)) {
        const auto it = tunnelIndexes_.find(index);
        return it != tunnelIndexes_.end() && now - it->second < kTrustWindow;
    }
    if (isTunnelName(name)) {
        for (auto it = tunnelIndexes_.begin(); it != tunnelIndexes_.end();) {
            it = (now - it->second >= kTrustWindow) ? tunnelIndexes_.erase(it) : std::next(it);
        }
        tunnelIndexes_[index] = now;
        return true;
    }
    tunnelIndexes_.erase(index);
    return false;
}
