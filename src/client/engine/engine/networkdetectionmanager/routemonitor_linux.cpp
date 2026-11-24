#include "routemonitor_linux.h"

#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils/log/categories.h"
#include "utils/ws_assert.h"

RouteMonitor_linux::RouteMonitor_linux(QObject *parent) : QObject(parent),
    filter_(
        [](int index, std::string &name) {
            char buf[IF_NAMESIZE] = {};
            if (if_indextoname(static_cast<unsigned int>(index), buf) == nullptr) {
                return false;
            }
            name = buf;
            return true;
        },
        [] { return std::chrono::steady_clock::now(); })
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

    filter_.processWakeup(
        [this](char *buffer, size_t size, int &error) {
            const ssize_t len = recv(fd_, buffer, size, MSG_DONTWAIT | MSG_TRUNC);
            error = errno;
            return len;
        },
        [this] { emit routesChanged(); },
        [this] { emit tunnelRoutesChanged(); });
}
