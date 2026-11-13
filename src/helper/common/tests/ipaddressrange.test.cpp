#include <cstdio>
#include <string>

#include "types/ipaddress.h"

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

bool contains(const std::string &range, const std::string &addr)
{
    return types::IpAddressRange(range).contains(types::IpAddress(addr));
}

void testV4ByteAlignedPrefix()
{
    VERIFY(contains("134.181.0.0/16", "134.181.0.0"));
    VERIFY(contains("134.181.0.0/16", "134.181.132.44"));
    VERIFY(contains("134.181.0.0/16", "134.181.255.255"));
    VERIFY(!contains("134.181.0.0/16", "134.180.255.255"));
    VERIFY(!contains("134.181.0.0/16", "134.182.0.0"));
}

void testV4UnalignedPrefix()
{
    VERIFY(contains("172.16.0.0/12", "172.31.255.255"));
    VERIFY(!contains("172.16.0.0/12", "172.32.0.0"));
    VERIFY(!contains("172.16.0.0/12", "172.15.255.255"));
    VERIFY(contains("10.0.0.0/31", "10.0.0.1"));
    VERIFY(!contains("10.0.0.0/31", "10.0.0.2"));
}

void testHostBitsInRangeAreIgnored()
{
    VERIFY(contains("134.181.132.44/16", "134.181.0.1"));
    VERIFY(!contains("134.181.132.44/16", "134.182.132.44"));
    VERIFY(contains("2001:db8::1/32", "2001:db8:ffff::2"));
}

void testSingleHost()
{
    VERIFY(contains("1.2.3.4", "1.2.3.4"));
    VERIFY(contains("1.2.3.4/32", "1.2.3.4"));
    VERIFY(!contains("1.2.3.4/32", "1.2.3.5"));
    VERIFY(contains("2001:db8::1", "2001:0db8:0000::0001"));
    VERIFY(!contains("2001:db8::1/128", "2001:db8::2"));
}

void testZeroPrefix()
{
    VERIFY(contains("0.0.0.0/0", "255.255.255.255"));
    VERIFY(contains("::/0", "2001:db8::1"));
    VERIFY(!contains("0.0.0.0/0", "2001:db8::1"));
}

void testV6Prefixes()
{
    VERIFY(contains("2001:db8::/32", "2001:db8:1234::1"));
    VERIFY(!contains("2001:db8::/32", "2001:db9::1"));
    VERIFY(contains("2001:db8::/65", "2001:db8::7fff:ffff:ffff:ffff"));
    VERIFY(!contains("2001:db8::/65", "2001:db8::8000:0:0:0"));
}

void testFamilyMismatch()
{
    VERIFY(!contains("134.181.0.0/16", "::ffff:134.181.0.1"));
    VERIFY(!contains("::/0", "1.2.3.4"));
}

void testInvalidInputsMatchNothing()
{
    VERIFY(!contains("not-an-ip/0", "0.0.0.0"));
    VERIFY(!contains("1.2.3.4/33", "1.2.3.4"));
    VERIFY(!contains("", "0.0.0.0"));
    VERIFY(!contains("0.0.0.0/0", "not-an-ip"));
    VERIFY(!contains("0.0.0.0/0", ""));
}

} // namespace

int main()
{
    testV4ByteAlignedPrefix();
    testV4UnalignedPrefix();
    testHostBitsInRangeAreIgnored();
    testSingleHost();
    testZeroPrefix();
    testV6Prefixes();
    testFamilyMismatch();
    testInvalidInputsMatchNothing();
    if (g_failures == 0) {
        printf("All IP address range tests passed.\n");
        return 0;
    }
    printf("%d check(s) failed.\n", g_failures);
    return 1;
}
