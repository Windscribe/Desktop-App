// Unit tests for firewall_incremental::planIncremental, which decides whether FirewallFilter::on may
// update the installed filters in place or must rebuild them. Pure logic, no WFP access.

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include "firewall_incremental.h"

using namespace firewall_incremental;

namespace
{

int g_failures = 0;

bool check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
    return ok;
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

TapSnapshot tap(unsigned long ifIndex, std::vector<std::string> addrs = {"100.64.0.2"}, bool v6 = false)
{
    TapSnapshot t;
    t.ifIndex = ifIndex;
    t.luid = 0x1000 + ifIndex;
    t.localAddrs = std::move(addrs);
    t.hasNonLinkLocalV6 = v6;
    return t;
}

Inputs inputs(const std::wstring &connectingIp, std::vector<std::wstring> ips, std::vector<TapSnapshot> taps = {})
{
    Inputs in;
    in.connectingIp = connectingIp;
    in.ips.insert(ips.begin(), ips.end());
    in.allowLocalTraffic = true;
    in.isCustomConfig = false;
    in.taps = std::move(taps);
    return in;
}

// What FirewallFilter::rebuild would cache for these inputs (filter IDs are arbitrary).
BuiltState builtFrom(const Inputs &in)
{
    BuiltState s;
    s.allowLocalTraffic = in.allowLocalTraffic;
    s.isCustomConfig = in.isCustomConfig;
    s.taps = in.taps;
    s.connectingIp = in.connectingIp;
    uint64_t id = 1;
    if (!in.connectingIp.empty()) {
        s.connectingIpFilterIds = {id++, id++};
    }
    for (const auto &ip : in.ips) {
        s.ipFilterIds[ip] = {id++, id++};
    }
    return s;
}

bool contains(const std::vector<std::wstring> &v, const std::wstring &s)
{
    return std::find(v.begin(), v.end(), s) != v.end();
}

void testNoCacheRebuilds()
{
    const Plan plan = planIncremental(std::nullopt, inputs(L"1.2.3.4", {L"5.6.7.8"}));
    VERIFY(plan.fullRebuild);
    VERIFY(plan.reason == "no cached build");
}

void testIdenticalInputsIsNoOp()
{
    const Inputs in = inputs(L"1.2.3.4", {L"5.6.7.8", L"9.9.9.9"}, {tap(30)});
    const Plan plan = planIncremental(builtFrom(in), in);
    VERIFY(!plan.fullRebuild);
    VERIFY(plan.isNoOp());
}

void testConnectingIpChangeOnly()
{
    const Inputs before = inputs(L"1.2.3.4", {L"5.6.7.8"});
    const Plan plan = planIncremental(builtFrom(before), inputs(L"1.2.3.5", {L"5.6.7.8"}));
    VERIFY(!plan.fullRebuild);
    VERIFY(plan.replaceConnectingIp);
    VERIFY(plan.ipsToAdd.empty());
    VERIFY(plan.ipsToRemove.empty());
}

void testConnectingIpFromAndToEmpty()
{
    // Always On firewall before any connection has no connecting IP.
    const Plan toIp = planIncremental(builtFrom(inputs(L"", {L"5.6.7.8"})), inputs(L"1.2.3.4", {L"5.6.7.8"}));
    VERIFY(!toIp.fullRebuild && toIp.replaceConnectingIp);
    const Plan toEmpty = planIncremental(builtFrom(inputs(L"1.2.3.4", {L"5.6.7.8"})), inputs(L"", {L"5.6.7.8"}));
    VERIFY(!toEmpty.fullRebuild && toEmpty.replaceConnectingIp);
}

void testStaleConnectingIpFilterIsReplaced()
{
    const Inputs in = inputs(L"1.2.3.4", {L"5.6.7.8"});
    BuiltState built = builtFrom(in);
    built.connectingIpFilterStale = true;   // the VPN app IDs changed
    const Plan plan = planIncremental(built, in);
    VERIFY(!plan.fullRebuild);
    VERIFY(plan.replaceConnectingIp);
    VERIFY(!plan.isNoOp());
}

void testIpsAddedAndRemoved()
{
    // The first connect of a session adds the tunnel DNS IP and the connecting IP together.
    const Inputs before = inputs(L"", {L"5.6.7.8", L"9.9.9.9", L"8.8.8.8"});
    const Inputs after = inputs(L"84.20.25.144", {L"5.6.7.8", L"9.9.9.9", L"10.255.255.1"});
    const Plan plan = planIncremental(builtFrom(before), after);
    VERIFY(!plan.fullRebuild);
    VERIFY(plan.replaceConnectingIp);
    VERIFY(plan.ipsToAdd.size() == 1 && contains(plan.ipsToAdd, L"10.255.255.1"));
    VERIFY(plan.ipsToRemove.size() == 1 && contains(plan.ipsToRemove, L"8.8.8.8"));
}

void testDuplicateIpsCollapse()
{
    const Inputs before = inputs(L"1.2.3.4", {L"5.6.7.8"});
    const Inputs after = inputs(L"1.2.3.4", {L"5.6.7.8", L"5.6.7.8", L"7.7.7.7", L"7.7.7.7"});
    VERIFY(after.ips.size() == 2);
    const Plan plan = planIncremental(builtFrom(before), after);
    VERIFY(!plan.fullRebuild);
    VERIFY(plan.ipsToAdd.size() == 1 && contains(plan.ipsToAdd, L"7.7.7.7"));
    VERIFY(plan.ipsToRemove.empty());
}

void testFlagChangesRebuild()
{
    const Inputs in = inputs(L"1.2.3.4", {L"5.6.7.8"});
    Inputs lan = in;
    lan.allowLocalTraffic = false;
    const Plan lanPlan = planIncremental(builtFrom(in), lan);
    VERIFY(lanPlan.fullRebuild && lanPlan.reason == "Allow LAN traffic changed");

    Inputs custom = in;
    custom.isCustomConfig = true;
    const Plan customPlan = planIncremental(builtFrom(in), custom);
    VERIFY(customPlan.fullRebuild && customPlan.reason == "custom config changed");
}

void testAdapterChangesRebuild()
{
    const Inputs base = inputs(L"1.2.3.4", {L"5.6.7.8"}, {tap(30)});
    const BuiltState built = builtFrom(base);

    Inputs appeared = base;
    appeared.taps.push_back(tap(31));
    VERIFY(planIncremental(built, appeared).fullRebuild);

    Inputs gone = base;
    gone.taps.clear();
    VERIFY(planIncremental(built, gone).fullRebuild);

    Inputs newAddress = base;
    newAddress.taps[0].localAddrs = {"100.64.0.3"};
    VERIFY(planIncremental(built, newAddress).fullRebuild);

    Inputs v6 = base;
    v6.taps[0].hasNonLinkLocalV6 = true;
    VERIFY(planIncremental(built, v6).fullRebuild);

    Inputs newLuid = base;
    newLuid.taps[0].luid += 1;
    const Plan plan = planIncremental(built, newLuid);
    VERIFY(plan.fullRebuild && plan.reason == "VPN adapters changed");
}

} // namespace

int main()
{
    testNoCacheRebuilds();
    testIdenticalInputsIsNoOp();
    testConnectingIpChangeOnly();
    testConnectingIpFromAndToEmpty();
    testStaleConnectingIpFilterIsReplaced();
    testIpsAddedAndRemoved();
    testDuplicateIpsCollapse();
    testFlagChangesRebuild();
    testAdapterChangesRebuild();

    if (g_failures == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("%d failure(s)\n", g_failures);
    return 1;
}
