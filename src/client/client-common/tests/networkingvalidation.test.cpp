#include <QtTest>

#include "networkingvalidation.test.h"

void TestNetworkingValidation::testIsValidMacAddress()
{
    QVERIFY(NetworkingValidation::isValidMacAddress("00:11:22:aa:bb:CC"));
    QVERIFY(NetworkingValidation::isValidMacAddress("00-11-22-AA-BB-CC"));
    QVERIFY(NetworkingValidation::isValidMacAddress("001122aabbcc"));
    QVERIFY(NetworkingValidation::isValidMacAddress("FFFFFFFFFFFF"));

    QVERIFY(!NetworkingValidation::isValidMacAddress(""));
    QVERIFY(!NetworkingValidation::isValidMacAddress("00:11:22:aa:bb"));        // too short
    QVERIFY(!NetworkingValidation::isValidMacAddress("00:11:22:aa:bb:CC:DD"));  // too long
    QVERIFY(!NetworkingValidation::isValidMacAddress("zz:11:22:aa:bb:cc"));     // non-hex
    QVERIFY(!NetworkingValidation::isValidMacAddress("00:11:22-aa:bb:cc"));     // mixed separator
    QVERIFY(!NetworkingValidation::isValidMacAddress("00112233445Z"));          // 12 chars but non-hex
}

void TestNetworkingValidation::testIsValidInterfaceName()
{
#if defined(Q_OS_WIN)
    QVERIFY(NetworkingValidation::isValidInterfaceName("Ethernet"));
    QVERIFY(NetworkingValidation::isValidInterfaceName("Ethernet 2"));
    QVERIFY(NetworkingValidation::isValidInterfaceName("Wi-Fi"));
    QVERIFY(NetworkingValidation::isValidInterfaceName("Local Area Connection 7"));
    QVERIFY(NetworkingValidation::isValidInterfaceName(QString::fromUtf8("Ethernet \xc3\xa9"))); // Unicode

    QVERIFY(!NetworkingValidation::isValidInterfaceName(""));
    QVERIFY(!NetworkingValidation::isValidInterfaceName(QString("Ethernet") + QChar(0)));  // NUL
    QVERIFY(!NetworkingValidation::isValidInterfaceName(QString("Ether") + QChar('\t'))); // control char
    QVERIFY(!NetworkingValidation::isValidInterfaceName(QString(257, QChar('a'))));        // > 256
#else
    QVERIFY(NetworkingValidation::isValidInterfaceName("en0"));
    QVERIFY(NetworkingValidation::isValidInterfaceName("eth0"));
    QVERIFY(NetworkingValidation::isValidInterfaceName("wlan0"));
    QVERIFY(NetworkingValidation::isValidInterfaceName("utun3"));

    QVERIFY(!NetworkingValidation::isValidInterfaceName(""));
    QVERIFY(!NetworkingValidation::isValidInterfaceName("this_name_is_too_long"));   // >= 16
    QVERIFY(!NetworkingValidation::isValidInterfaceName("eth/0"));                   // slash
    QVERIFY(!NetworkingValidation::isValidInterfaceName("eth:0"));                   // colon
    QVERIFY(!NetworkingValidation::isValidInterfaceName("eth 0"));                   // space
    QVERIFY(!NetworkingValidation::isValidInterfaceName(QString("eth") + QChar(0))); // NUL
#endif
}

void TestNetworkingValidation::testIsValidPort()
{
    QVERIFY(NetworkingValidation::isValidPort(1));
    QVERIFY(NetworkingValidation::isValidPort(80));
    QVERIFY(NetworkingValidation::isValidPort(443));
    QVERIFY(NetworkingValidation::isValidPort(65535));

    QVERIFY(!NetworkingValidation::isValidPort(0));
    QVERIFY(!NetworkingValidation::isValidPort(-1));
    QVERIFY(!NetworkingValidation::isValidPort(65536));
    QVERIFY(!NetworkingValidation::isValidPort(INT_MIN));
    QVERIFY(!NetworkingValidation::isValidPort(INT_MAX));
}

void TestNetworkingValidation::testIsIpAndIsDomain_smoke()
{
    QVERIFY(NetworkingValidation::isIp("1.2.3.4"));
    QVERIFY(NetworkingValidation::isIp("255.255.255.255"));
    QVERIFY(!NetworkingValidation::isIp("256.1.1.1"));
    QVERIFY(!NetworkingValidation::isIp("not an ip"));

    // IPv6 literals are accepted (was the trigger for the v6 client-validation refresh).
    QVERIFY(NetworkingValidation::isIp("::1"));
    QVERIFY(NetworkingValidation::isIp("2001:4860:4860::8888"));
    QVERIFY(NetworkingValidation::isIp("fe80::1"));
    QVERIFY(!NetworkingValidation::isIp("fe80::1%eth0"));    // scope id explicitly rejected
    QVERIFY(!NetworkingValidation::isIp("2001:db8::/64"));   // CIDR is not isIp territory
    QVERIFY(!NetworkingValidation::isIp("::g"));             // bad hex

    QVERIFY(NetworkingValidation::isDomain("foo.example.com"));
    QVERIFY(NetworkingValidation::isDomain("a.bc"));
    QVERIFY(!NetworkingValidation::isDomain(""));
    QVERIFY(!NetworkingValidation::isDomain("no_tld"));
}

void TestNetworkingValidation::testIsIp()
{
    // Valid v4 + v6 literals.
    const char *kValidIps[] = {
        "0.0.0.0",
        "1.2.3.4",
        "255.255.255.255",
        "8.8.8.8",
        "::",
        "::1",
        "fe80::1",
        "2001:db8::1",
        "2001:4860:4860::8888",
        "fd00:abcd::5",
    };
    // Invalid inputs.
    const char *kInvalidIps[] = {
        "",
        "1.2.3",                // missing octet
        "1.2.3.4.5",            // too many octets
        "1.2.3.256",            // octet out of range
        "1.2.3.4/24",           // CIDR not allowed in isIp
        "google.com",           // hostname
        "fe80::1%eth0",         // scoped v6
        "::g",                  // bad hex in v6
        " 1.2.3.4",             // leading whitespace
        "1.2.3.4 ",             // trailing whitespace
        "2001:db8::/64",        // CIDR not allowed in isIp
    };

    for (const char *s : kValidIps)
        QVERIFY2(NetworkingValidation::isIp(s), s);
    for (const char *s : kInvalidIps)
        QVERIFY2(!NetworkingValidation::isIp(s), s);
}

void TestNetworkingValidation::testIsDomain()
{
    // Valid domain names.
    const char *kValidDomainNames[] = {
        "g.co",
        "g.com",
        "google.t.t.co",
        "0-0o.com",
        "0-oz.co.uk",
        "0-google.com.br",
        "0-wh-ao14-0.com-com.net",
        "xn--d1ai6ai.xn--p1ai",
        "xn-fsqu00a.xn-0zwm56d",
        "xn--google.com",
        "google.xn--com",
        "google.com.au",
        "www.google.com",
        "google.com",
        "google123.com",
        "google-info.com",
        "sub.google.com",
        "sub.google-info.com",
        "my.sub-domain.at-123.google.com.au",
        "test-andmoretest-somerandomlettersoflongstring.us-east-2.eu.google.com",
        "a-1234567890-1234567890-1234567890-1234567890-1234567890-1234-z.eu.us",
    };
    // Invalid domain names.
    const char *kInvalidDomainNames[] = {
        "com.g",            // TLD must be 2 or more characters long.
        "google.t.t.c",     // TLD must be 2 or more characters long.
        "google,com",       // Comma is not allowed.
        "google",           // Missing TLD.
        "google.1test",     // TLD must not start with a digit.
        "google.test1",     // TLD must not end with a digit.
        "google.123",       // TLD must not start or end with a digit.
        ".com",             // Empty subdomain.
        "google.com/users", // No TLD.
        "-g.com",           // Subdomain cannot start with a hyphen.
        "-0-0o.com",        // Subdomain cannot start with a hyphen.
        "0-0o_.com",        // Underscore is not allowed.
        "-google.com",      // Subdomain cannot start with a hyphen.
        "google-.com",      // Subdomain cannot end with a hyphen.
        "sub.-google.com",  // Subdomain cannot start with a hyphen.
        "sub.google-.com",  // Subdomain cannot end with a hyphen.
        "a-1234567890-1234567890-1234567890-1234567890-1234567890-12345-z.eu.us", // Too long.
    };

    for (const char *s : kValidDomainNames)
        QVERIFY2(NetworkingValidation::isDomain(s), s);
    for (const char *s : kInvalidDomainNames)
        QVERIFY2(!NetworkingValidation::isDomain(s), s);
}

void TestNetworkingValidation::testIsIpv4AndIsIpv6()
{
    // v4 literals
    QVERIFY(NetworkingValidation::isIpv4("0.0.0.0"));
    QVERIFY(NetworkingValidation::isIpv4("1.2.3.4"));
    QVERIFY(NetworkingValidation::isIpv4("255.255.255.255"));
    QVERIFY(!NetworkingValidation::isIpv6("1.2.3.4"));
    QVERIFY(!NetworkingValidation::isIpv6("255.255.255.255"));

    // v6 literals
    QVERIFY(NetworkingValidation::isIpv6("::"));
    QVERIFY(NetworkingValidation::isIpv6("::1"));
    QVERIFY(NetworkingValidation::isIpv6("fe80::1"));
    QVERIFY(NetworkingValidation::isIpv6("2001:db8::1"));
    QVERIFY(NetworkingValidation::isIpv6("2001:4860:4860::8888"));
    QVERIFY(!NetworkingValidation::isIpv4("::"));
    QVERIFY(!NetworkingValidation::isIpv4("::1"));
    QVERIFY(!NetworkingValidation::isIpv4("2001:db8::1"));

    // Both reject anything isIp() would also reject (hostnames, CIDR, scope-id, junk).
    QVERIFY(!NetworkingValidation::isIpv4(""));
    QVERIFY(!NetworkingValidation::isIpv6(""));
    QVERIFY(!NetworkingValidation::isIpv4("google.com"));
    QVERIFY(!NetworkingValidation::isIpv6("google.com"));
    QVERIFY(!NetworkingValidation::isIpv4("256.1.1.1"));
    // BSD-style shorthand forms that QHostAddress::setAddress would otherwise accept.
    QVERIFY(!NetworkingValidation::isIpv4("1.2.3"));
    QVERIFY(!NetworkingValidation::isIpv4("1.2"));
    QVERIFY(!NetworkingValidation::isIpv4("1"));
    QVERIFY(!NetworkingValidation::isIpv4("01.02.03.04"));
    QVERIFY(!NetworkingValidation::isIpv4("1.2.3.4/24"));
    QVERIFY(!NetworkingValidation::isIpv6("2001:db8::/64"));
    QVERIFY(!NetworkingValidation::isIpv6("fe80::1%eth0"));
    QVERIFY(!NetworkingValidation::isIpv6(" ::1"));
    QVERIFY(!NetworkingValidation::isIpv4("::g"));

    // Mutual exclusivity for valid IPs.
    QVERIFY(!(NetworkingValidation::isIpv4("1.2.3.4") && NetworkingValidation::isIpv6("1.2.3.4")));
    QVERIFY(!(NetworkingValidation::isIpv4("::1") && NetworkingValidation::isIpv6("1.2.3.4")));
}

void TestNetworkingValidation::testIsIpCidr()
{
    // v4 — historical behaviour preserved.
    QVERIFY(NetworkingValidation::isIpCidr("1.2.3.4"));
    QVERIFY(NetworkingValidation::isIpCidr("1.2.3.4/0"));
    QVERIFY(NetworkingValidation::isIpCidr("10.0.0.0/8"));
    QVERIFY(NetworkingValidation::isIpCidr("0.0.0.0/0"));
    QVERIFY(NetworkingValidation::isIpCidr("1.2.3.4/32"));
    QVERIFY(!NetworkingValidation::isIpCidr("1.2.3.4/33"));
    QVERIFY(!NetworkingValidation::isIpCidr("1.2.3.4/"));
    QVERIFY(!NetworkingValidation::isIpCidr("1.2.3.4 /24"));   // whitespace
    QVERIFY(!NetworkingValidation::isIpCidr("1.2.3.4/-1"));    // negative prefix
    QVERIFY(!NetworkingValidation::isIpCidr("1.2.3.4/24/8"));  // double suffix
    QVERIFY(!NetworkingValidation::isIpCidr("garbage/24"));

    // v6 — new behaviour added by the client-side IpValidation refresh.
    QVERIFY(NetworkingValidation::isIpCidr("2001:db8::/64"));
    QVERIFY(NetworkingValidation::isIpCidr("::/0"));
    QVERIFY(NetworkingValidation::isIpCidr("2001:db8::1/128"));
    QVERIFY(NetworkingValidation::isIpCidr("2001:4860:4860::8888"));  // single host v6
    QVERIFY(NetworkingValidation::isIpCidr("fd00::/8"));
    QVERIFY(!NetworkingValidation::isIpCidr("2001:db8::/129"));
    QVERIFY(!NetworkingValidation::isIpCidr("2001:db8::1/abc"));
}

void TestNetworkingValidation::testIsValidIpForCidr()
{
    // v4 — alignment check preserved verbatim (well-tested 32-bit arithmetic).
    QVERIFY(NetworkingValidation::isValidIpForCidr("10.0.0.0/8"));
    QVERIFY(NetworkingValidation::isValidIpForCidr("192.168.0.0/16"));
    QVERIFY(NetworkingValidation::isValidIpForCidr("1.2.3.4"));         // single host
    QVERIFY(NetworkingValidation::isValidIpForCidr("1.2.3.4/32"));
    QVERIFY(NetworkingValidation::isValidIpForCidr("0.0.0.0/0"));
    QVERIFY(!NetworkingValidation::isValidIpForCidr("10.0.0.1/8"));     // host bits set
    QVERIFY(!NetworkingValidation::isValidIpForCidr("192.168.1.0/16")); // host bits set

    // v6 — new byte-walk alignment check.
    QVERIFY(NetworkingValidation::isValidIpForCidr("2001:db8::/64"));
    QVERIFY(NetworkingValidation::isValidIpForCidr("2001:db8::1/128"));
    QVERIFY(NetworkingValidation::isValidIpForCidr("::/0"));
    QVERIFY(!NetworkingValidation::isValidIpForCidr("2001:db8::1/64")); // host bits set in v6
    QVERIFY(!NetworkingValidation::isValidIpForCidr("2001:db8::101/127")); // last bit set, /127 expects it cleared
    QVERIFY(NetworkingValidation::isValidIpForCidr("2001:db8::100/120")); // byte[15]=0, aligned at /120

    // Hostname pass-through preserved — callers (splittunnelingaddressesgroup) pass domains in.
    QVERIFY(NetworkingValidation::isValidIpForCidr("google.com"));

    // CIDR-shaped but bad IP — rejected (old behaviour had latent crash; new code is defined).
    QVERIFY(!NetworkingValidation::isValidIpForCidr("garbage/24"));
}

void TestNetworkingValidation::testIsLocalIp()
{
    // v4 — existing startsWith ranges preserved verbatim.
    QVERIFY(NetworkingValidation::isLocalIp("127.0.0.1"));
    QVERIFY(NetworkingValidation::isLocalIp("10.0.0.5"));
    QVERIFY(NetworkingValidation::isLocalIp("192.168.1.1"));
    QVERIFY(NetworkingValidation::isLocalIp("169.254.10.1"));
    QVERIFY(NetworkingValidation::isLocalIp("172.16.0.1"));
    QVERIFY(NetworkingValidation::isLocalIp("172.31.255.255"));
    QVERIFY(!NetworkingValidation::isLocalIp("172.32.0.1"));   // outside 172.16/12
    QVERIFY(!NetworkingValidation::isLocalIp("172.15.0.1"));   // outside 172.16/12
    QVERIFY(!NetworkingValidation::isLocalIp("8.8.8.8"));

    // Tightening: invalid IP literals no longer accepted via lenient startsWith.
    QVERIFY(!NetworkingValidation::isLocalIp("10.foo"));

    // v6 — new ranges via QHostAddress classifiers.
    QVERIFY(NetworkingValidation::isLocalIp("::1"));            // loopback
    QVERIFY(NetworkingValidation::isLocalIp("fe80::1"));        // link-local
    QVERIFY(NetworkingValidation::isLocalIp("fc00::1"));        // ULA
    QVERIFY(NetworkingValidation::isLocalIp("fd12:3456::1"));   // ULA
    QVERIFY(!NetworkingValidation::isLocalIp("2001:4860:4860::8888"));  // public v6
}

void TestNetworkingValidation::testIsUnspecifiedIp()
{
    QVERIFY(NetworkingValidation::isUnspecifiedIp("0.0.0.0"));
    QVERIFY(NetworkingValidation::isUnspecifiedIp("::"));

    QVERIFY(!NetworkingValidation::isUnspecifiedIp("127.0.0.1"));
    QVERIFY(!NetworkingValidation::isUnspecifiedIp("::1"));
    QVERIFY(!NetworkingValidation::isUnspecifiedIp("8.8.8.8"));
    QVERIFY(!NetworkingValidation::isUnspecifiedIp("0.0.0.1"));
    QVERIFY(!NetworkingValidation::isUnspecifiedIp(""));         // not a valid literal
    QVERIFY(!NetworkingValidation::isUnspecifiedIp("garbage"));
}

void TestNetworkingValidation::testIsCtrldCorrectAddress()
{
    QVERIFY(NetworkingValidation::isCtrldCorrectAddress("1.2.3.4"));
    QVERIFY(NetworkingValidation::isCtrldCorrectAddress("dns.example.com"));
    QVERIFY(NetworkingValidation::isCtrldCorrectAddress("https://cloudflare-dns.com/dns-query"));
    QVERIFY(NetworkingValidation::isCtrldCorrectAddress("sdns://AgMAAAAAAAAACjEuMS4xLjE"));

    QVERIFY(!NetworkingValidation::isCtrldCorrectAddress(""));
    QVERIFY(!NetworkingValidation::isCtrldCorrectAddress("abc;rm -rf"));
}

QTEST_MAIN(TestNetworkingValidation)
