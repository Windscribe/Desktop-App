#pragma once

#include <cstdint>
#include <functional>
#include <linux/netlink.h>

namespace NetlinkRoute {

// Table a route message refers to; RTA_TABLE carries ids that do not fit rtm_table.
uint32_t routeTable(const nlmsghdr *msg);

// Reads everything queued on the socket. True when `isRelevant` accepts any added or deleted route,
// or messages were lost (ENOBUFS), after which the table has to be re-read anyway.
bool drain(int fd, const std::function<bool(const nlmsghdr *msg)> &isRelevant);

} // namespace NetlinkRoute
