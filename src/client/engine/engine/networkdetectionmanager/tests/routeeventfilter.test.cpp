#include "routeeventfilter.test.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <sys/socket.h>
#include <vector>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <QtTest>

#include "routeeventfilter_linux.h"

namespace {

using Result = RouteEventFilter_linux::Result;

struct Env
{
    std::map<int, std::string> names;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::time_point(std::chrono::seconds(1000));

    RouteEventFilter_linux makeFilter()
    {
        return RouteEventFilter_linux(
            [this](int index, std::string &name) {
                const auto it = names.find(index);
                if (it == names.end()) {
                    return false;
                }
                name = it->second;
                return true;
            },
            [this] { return now; });
    }
};

void appendAttr(std::vector<char> &msg, unsigned short type, const void *data, size_t len)
{
    const size_t start = msg.size();
    msg.resize(start + RTA_SPACE(len), 0);
    rtattr *rta = reinterpret_cast<rtattr *>(msg.data() + start);
    rta->rta_type = type;
    rta->rta_len = static_cast<unsigned short>(RTA_LENGTH(len));
    memcpy(RTA_DATA(rta), data, len);
}

std::vector<char> routeMessage(unsigned short type, int oif)
{
    std::vector<char> msg(NLMSG_SPACE(sizeof(rtmsg)), 0);
    rtmsg *rtm = static_cast<rtmsg *>(NLMSG_DATA(reinterpret_cast<nlmsghdr *>(msg.data())));
    rtm->rtm_family = AF_INET;
    rtm->rtm_table = RT_TABLE_MAIN;
    if (oif != 0) {
        appendAttr(msg, RTA_OIF, &oif, sizeof(oif));
    }
    nlmsghdr *nh = reinterpret_cast<nlmsghdr *>(msg.data());
    nh->nlmsg_type = type;
    nh->nlmsg_len = static_cast<unsigned int>(msg.size());
    return msg;
}

Result classify(RouteEventFilter_linux &filter, const std::vector<char> &datagram)
{
    alignas(nlmsghdr) char buffer[4096];
    memcpy(buffer, datagram.data(), datagram.size());
    return filter.classify(buffer, static_cast<ssize_t>(datagram.size()), sizeof(buffer));
}

std::vector<char> concat(const std::vector<char> &a, const std::vector<char> &b)
{
    std::vector<char> out = a;
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

struct Read
{
    int error = 0;
    std::vector<char> data;
};

struct Socket
{
    std::deque<Read> reads;
    int refreshes = 0;
    int tunnelSignals = 0;

    void process(RouteEventFilter_linux &filter)
    {
        filter.processWakeup(
            [this](char *buffer, size_t size, int &error) -> ssize_t {
                if (reads.empty()) {
                    error = EAGAIN;
                    return -1;
                }
                const Read read = reads.front();
                reads.pop_front();
                if (read.error != 0) {
                    error = read.error;
                    return -1;
                }
                memcpy(buffer, read.data.data(), std::min(size, read.data.size()));
                return static_cast<ssize_t>(read.data.size());
            },
            [this] { ++refreshes; },
            [this] { ++tunnelSignals; });
    }
};

}  // namespace

void TestRouteEventFilter::physicalRouteRefreshes()
{
    Env env;
    env.names = {{2, "eth0"}};
    auto filter = env.makeFilter();
    QCOMPARE(classify(filter, routeMessage(RTM_NEWROUTE, 2)), Result::Refresh);
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 2)), Result::Refresh);
}

void TestRouteEventFilter::tunnelRouteIsTunnelOnly()
{
    Env env;
    env.names = {{5, "tun0"}, {6, "utun420"}};
    auto filter = env.makeFilter();
    QCOMPARE(classify(filter, routeMessage(RTM_NEWROUTE, 5)), Result::TunnelOnly);
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 6)), Result::TunnelOnly);
}

void TestRouteEventFilter::routeWithoutOutputInterfaceRefreshes()
{
    Env env;
    auto filter = env.makeFilter();
    QCOMPARE(classify(filter, routeMessage(RTM_NEWROUTE, 0)), Result::Refresh);
}

void TestRouteEventFilter::multipathThroughTunnelsRefreshes()
{
    Env env;
    env.names = {{5, "tun0"}, {6, "tun1"}};
    auto filter = env.makeFilter();
    std::vector<char> msg = routeMessage(RTM_NEWROUTE, 0);
    std::vector<char> nexthops(sizeof(rtnexthop) * 2, 0);
    for (int i = 0; i < 2; ++i) {
        rtnexthop *nhop = reinterpret_cast<rtnexthop *>(nexthops.data() + i * sizeof(rtnexthop));
        nhop->rtnh_len = sizeof(rtnexthop);
        nhop->rtnh_ifindex = 5 + i;
    }
    appendAttr(msg, RTA_MULTIPATH, nexthops.data(), nexthops.size());
    reinterpret_cast<nlmsghdr *>(msg.data())->nlmsg_len = static_cast<unsigned int>(msg.size());
    QCOMPARE(classify(filter, msg), Result::Refresh);
}

void TestRouteEventFilter::unknownIndexRefreshes()
{
    Env env;
    auto filter = env.makeFilter();
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 9)), Result::Refresh);
}

void TestRouteEventFilter::goneTunnelTrustedOnlyWithinWindow()
{
    Env env;
    env.names = {{5, "tun0"}};
    auto filter = env.makeFilter();
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 5)), Result::TunnelOnly);
    env.names.clear();
    env.now += std::chrono::milliseconds(1500);
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 5)), Result::TunnelOnly);
    env.now += std::chrono::milliseconds(1000);
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 5)), Result::Refresh);
}

void TestRouteEventFilter::renamedTunnelRefreshes()
{
    Env env;
    env.names = {{5, "tun0"}};
    auto filter = env.makeFilter();
    QCOMPARE(classify(filter, routeMessage(RTM_NEWROUTE, 5)), Result::TunnelOnly);
    env.names[5] = "eth9";
    QCOMPARE(classify(filter, routeMessage(RTM_NEWROUTE, 5)), Result::Refresh);
    env.names.clear();
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 5)), Result::Refresh);
}

void TestRouteEventFilter::mixedDatagramRefreshes()
{
    Env env;
    env.names = {{2, "eth0"}, {5, "tun0"}, {6, "tun1"}};
    auto filter = env.makeFilter();
    QCOMPARE(classify(filter, concat(routeMessage(RTM_NEWROUTE, 2), routeMessage(RTM_NEWROUTE, 5))), Result::Refresh);
    QCOMPARE(classify(filter, concat(routeMessage(RTM_NEWROUTE, 5), routeMessage(RTM_NEWROUTE, 6))), Result::TunnelOnly);
    QCOMPARE(classify(filter, concat(routeMessage(RTM_NEWROUTE, 2), routeMessage(RTM_DELROUTE, 6))), Result::Refresh);
    env.names.erase(6);
    QCOMPARE(classify(filter, routeMessage(RTM_DELROUTE, 6)), Result::TunnelOnly);
}

void TestRouteEventFilter::malformedOrTruncatedRefreshes()
{
    Env env;
    env.names = {{5, "tun0"}};
    auto filter = env.makeFilter();
    std::vector<char> msg = routeMessage(RTM_NEWROUTE, 5);
    std::vector<char> trailing = msg;
    trailing.resize(trailing.size() + 8, 0);
    QCOMPARE(classify(filter, trailing), Result::Refresh);
    alignas(nlmsghdr) char buffer[4096];
    memcpy(buffer, msg.data(), msg.size());
    QCOMPARE(filter.classify(buffer, static_cast<ssize_t>(sizeof(buffer) + 1), sizeof(buffer)), Result::Refresh);
    reinterpret_cast<nlmsghdr *>(buffer)->nlmsg_len = static_cast<unsigned int>(msg.size() + 64);
    QCOMPARE(filter.classify(buffer, static_cast<ssize_t>(msg.size()), sizeof(buffer)), Result::Refresh);
    std::vector<char> shortRoute(NLMSG_SPACE(0), 0);
    reinterpret_cast<nlmsghdr *>(shortRoute.data())->nlmsg_type = RTM_NEWROUTE;
    reinterpret_cast<nlmsghdr *>(shortRoute.data())->nlmsg_len = static_cast<unsigned int>(shortRoute.size());
    QCOMPARE(classify(filter, shortRoute), Result::Refresh);
}

void TestRouteEventFilter::nonRouteMessageRefreshes()
{
    Env env;
    auto filter = env.makeFilter();
    std::vector<char> msg = routeMessage(RTM_NEWROUTE, 0);
    reinterpret_cast<nlmsghdr *>(msg.data())->nlmsg_type = RTM_NEWLINK;
    QCOMPARE(classify(filter, msg), Result::Refresh);
}

void TestRouteEventFilter::emptyDatagramRefreshes()
{
    Env env;
    auto filter = env.makeFilter();
    alignas(nlmsghdr) char buffer[4096] = {};
    QCOMPARE(filter.classify(buffer, 0, sizeof(buffer)), Result::Refresh);
    Socket socket;
    socket.reads = {Read{0, {}}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 1);
}

void TestRouteEventFilter::wakeupRefreshesPerPhysicalDatagram()
{
    Env env;
    env.names = {{2, "eth0"}};
    auto filter = env.makeFilter();
    Socket socket;
    socket.reads = {Read{0, routeMessage(RTM_NEWROUTE, 2)}, Read{0, routeMessage(RTM_DELROUTE, 2)}, Read{0, routeMessage(RTM_NEWROUTE, 0)}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 3);
    QCOMPARE(socket.tunnelSignals, 0);
}

void TestRouteEventFilter::wakeupTunnelOnlySignalsOnce()
{
    Env env;
    env.names = {{5, "tun0"}};
    auto filter = env.makeFilter();
    Socket socket;
    socket.reads = {Read{0, routeMessage(RTM_NEWROUTE, 5)}, Read{0, routeMessage(RTM_NEWROUTE, 5)}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 0);
    QCOMPARE(socket.tunnelSignals, 1);
    socket.reads = {Read{0, routeMessage(RTM_DELROUTE, 5)}, Read{0, routeMessage(RTM_DELROUTE, 5)}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 0);
    QCOMPARE(socket.tunnelSignals, 2);
}

void TestRouteEventFilter::wakeupMixedRefreshesAndChecks()
{
    Env env;
    env.names = {{2, "eth0"}, {5, "tun0"}};
    auto filter = env.makeFilter();
    Socket socket;
    socket.reads = {Read{0, routeMessage(RTM_NEWROUTE, 5)}, Read{0, routeMessage(RTM_NEWROUTE, 2)}, Read{0, routeMessage(RTM_NEWROUTE, 5)}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 1);
    QCOMPARE(socket.tunnelSignals, 1);
    socket.reads = {Read{0, routeMessage(RTM_NEWROUTE, 2)}, Read{0, routeMessage(RTM_DELROUTE, 5)}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 2);
    QCOMPARE(socket.tunnelSignals, 2);
}

void TestRouteEventFilter::wakeupOverflowRefreshes()
{
    Env env;
    env.names = {{5, "tun0"}};
    auto filter = env.makeFilter();
    Socket socket;
    socket.reads = {Read{ENOBUFS, {}}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 1);
    QCOMPARE(socket.tunnelSignals, 0);
    socket.reads = {Read{ENOBUFS, {}}, Read{0, routeMessage(RTM_NEWROUTE, 5)}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 2);
    QCOMPARE(socket.tunnelSignals, 1);
}

void TestRouteEventFilter::wakeupStopsOnOtherErrors()
{
    Env env;
    env.names = {{2, "eth0"}};
    auto filter = env.makeFilter();
    Socket socket;
    socket.process(filter);
    QCOMPARE(socket.refreshes, 0);
    socket.reads = {Read{EINTR, {}}, Read{0, routeMessage(RTM_NEWROUTE, 2)}};
    socket.process(filter);
    QCOMPARE(socket.refreshes, 0);
    QCOMPARE(socket.reads.size(), size_t(1));
    QCOMPARE(socket.tunnelSignals, 0);
}

void TestRouteEventFilter::wakeupReadsAtMostCap()
{
    Env env;
    env.names = {{2, "eth0"}};
    auto filter = env.makeFilter();
    Socket socket;
    for (int i = 0; i < RouteEventFilter_linux::kMaxReadsPerWakeup + 6; ++i) {
        socket.reads.push_back(Read{0, routeMessage(RTM_NEWROUTE, 2)});
    }
    socket.process(filter);
    QCOMPARE(socket.refreshes, RouteEventFilter_linux::kMaxReadsPerWakeup);
    QCOMPARE(socket.reads.size(), size_t(6));
}

void TestRouteEventFilter::defaultRouteTrackerComparesWithLastRead()
{
    DefaultRoute_linux current = {"eth0", "192.168.1.1"};
    DefaultRouteTracker_linux tracker([&current] { return current; });
    QVERIFY(tracker.differsFromLastRead());
    QCOMPARE(tracker.read().interface, QString("eth0"));
    QVERIFY(!tracker.differsFromLastRead());
    current = {"eth0", "192.168.1.254"};
    QVERIFY(tracker.differsFromLastRead());
    QVERIFY(tracker.differsFromLastRead());
    tracker.read();
    QVERIFY(!tracker.differsFromLastRead());
    current = {"wlan0", "192.168.1.254"};
    QVERIFY(tracker.differsFromLastRead());
    current = {"", ""};
    tracker.read();
    QVERIFY(!tracker.differsFromLastRead());
}

QTEST_GUILESS_MAIN(TestRouteEventFilter)
