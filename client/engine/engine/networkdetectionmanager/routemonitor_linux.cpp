#include "routemonitor_linux.h"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils/log/categories.h"
#include "utils/ws_assert.h"

RouteMonitor_linux::RouteMonitor_linux(QObject *parent) : QObject(parent)
{
}

RouteMonitor_linux::~RouteMonitor_linux()
{
    if (fd_ >= 0) {
        close(fd_);
    }
}

void RouteMonitor_linux::init()
{
    if ((fd_ = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        qCCritical(LOG_BASIC) << "RouteMonitor_linux could not open netlink socket";
        WS_ASSERT(false);
        return;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid    = getpid();
    addr.nl_groups = RTMGRP_IPV4_ROUTE;

    if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        qCCritical(LOG_BASIC) << "RouteMonitor_linux could not bind address";
        WS_ASSERT(false);
        return;
    }

    notifier_ = new QSocketNotifier(fd_, QSocketNotifier::Read, this);
    connect(notifier_, &QSocketNotifier::activated, this, &RouteMonitor_linux::netlinkSocketReady);
    notifier_->setEnabled(true);
}

void RouteMonitor_linux::finish()
{
    if (notifier_) {
        notifier_->setEnabled(false);
    }
}

void RouteMonitor_linux::netlinkSocketReady(QSocketDescriptor socket, QSocketNotifier::Type activationEvent)
{
    Q_UNUSED(socket)
    Q_UNUSED(activationEvent)

    char buffer[4096];
    ssize_t len = recv(fd_, buffer, sizeof(buffer), MSG_DONTWAIT);
    if (len >= 0) {
        emit routesChanged();
    }
}
