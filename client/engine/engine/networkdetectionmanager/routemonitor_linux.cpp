#include "routemonitor_linux.h"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#include "utils/logger.h"
#include "utils/ws_assert.h"

RouteMonitor_linux::RouteMonitor_linux(QObject *parent) : QThread(parent), fd_(-1)
{
}

RouteMonitor_linux::~RouteMonitor_linux()
{
    close(fd_);
}

void RouteMonitor_linux::run() {
    struct sockaddr_nl addr;

    if ((fd_ = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
        qCDebug(LOG_BASIC) << "Could not open netlink socket";
	WS_ASSERT(false);
        return;
    }

    bzero(&addr, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_ROUTE;

    if (bind(fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        qCDebug(LOG_BASIC) << "Could not bind address";
	WS_ASSERT(false);
        return;
    }

    int bytes = 0;
    char buffer[4096];

    while (1) {
        bytes = recv(fd_, buffer, sizeof(buffer), 0);
        if (bytes < 0) {
            qCDebug(LOG_BASIC) << "Netlink socket received error";
	    WS_ASSERT(false);
	    return;
        }
        emit routesChanged();
    }
}
