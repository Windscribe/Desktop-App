#include <cstdio>
#include <string>
#include <vector>

#include "../ip_hostnames/ip_hostnames_manager.h"

namespace {

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

void apply(IpHostnamesManager &manager, const std::vector<std::string> &entries)
{
    std::vector<types::IpAddressRange> ranges;
    for (const auto &entry : entries) {
        ranges.emplace_back(entry);
    }
    manager.setSettings(ranges, {});
    manager.enable();
}

void testCidrEntries()
{
    IpHostnamesManager manager;
    apply(manager, {"134.181.0.0/16", "2001:db8::/32", "10.1.2.3/8"});
    VERIFY(manager.isIpInList("134.181.132.44"));
    VERIFY(!manager.isIpInList("134.182.0.1"));
    VERIFY(manager.isIpInList("2001:db8:1234::1"));
    VERIFY(!manager.isIpInList("2001:db9::1"));
    VERIFY(manager.isIpInList("10.200.0.1"));
    VERIFY(!manager.isIpInList("11.0.0.1"));
    manager.disable();
}

void testSingleAddressEntries()
{
    IpHostnamesManager manager;
    apply(manager, {"1.2.3.4", "2001:0db8::0001"});
    VERIFY(manager.isIpInList("1.2.3.4"));
    VERIFY(!manager.isIpInList("1.2.3.5"));
    VERIFY(manager.isIpInList("2001:db8::1"));
    VERIFY(!manager.isIpInList("2001:db8::2"));
    manager.disable();
}

void testUpdateReplacesEntries()
{
    IpHostnamesManager manager;
    apply(manager, {"134.181.0.0/16"});
    VERIFY(manager.isIpInList("134.181.0.1"));
    apply(manager, {"192.0.2.0/24"});
    VERIFY(!manager.isIpInList("134.181.0.1"));
    VERIFY(manager.isIpInList("192.0.2.200"));
    apply(manager, {});
    VERIFY(!manager.isIpInList("192.0.2.200"));
    manager.disable();
}

void testUnparsableAddressMatchesNothing()
{
    IpHostnamesManager manager;
    apply(manager, {"0.0.0.0/0", "::/0"});
    VERIFY(manager.isIpInList("8.8.8.8"));
    VERIFY(!manager.isIpInList(""));
    VERIFY(!manager.isIpInList("not-an-ip"));
    manager.disable();
}

} // namespace

int main()
{
    testCidrEntries();
    testSingleAddressEntries();
    testUpdateReplacesEntries();
    testUnparsableAddressMatchesNothing();
    if (g_failures == 0) {
        printf("All IP and hostname list tests passed.\n");
        return 0;
    }
    printf("%d check(s) failed.\n", g_failures);
    return 1;
}
