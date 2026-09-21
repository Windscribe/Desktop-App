// Regression tests for the address validators. These pin the guarantee that a client-supplied
// value carrying an embedded NUL cannot pass validation: the underlying parsers (boost make_address,
// inet_pton) read the c_str() and stop at the first NUL, so without an explicit guard a valid prefix
// would validate while the bytes after the NUL survive into the pf/iptables/OpenVPN text the helper
// writes as root. No external test framework; returns the number of failed checks (0 on success).

#include <cstdio>
#include <string>
#include <vector>

#include "../validation_posix.h"
#include "types/ipaddress.h"

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

// Build a string with an embedded NUL, since a plain literal would terminate at the NUL. The suffix
// carries the smuggled payload (a newline plus an injected directive) that must never reach a sink.
std::string withNul(const std::string &prefix, const std::string &suffix)
{
    std::string s = prefix;
    s.push_back('\0');
    s += suffix;
    return s;
}

const std::string kPayload = "\nplugin /tmp/root-plugin.so\n#";

void testIsValidIpAddress()
{
    VERIFY(Validation::isValidIpAddress("127.0.0.1"));
    VERIFY(Validation::isValidIpAddress("::1"));
    VERIFY(!Validation::isValidIpAddress("not-an-ip"));
    // The reported LPE payload: a valid prefix + NUL + injected directive.
    VERIFY(!Validation::isValidIpAddress(withNul("127.0.0.1", kPayload)));
    VERIFY(!Validation::isValidIpAddress(withNul("::1", kPayload)));
    // A bare trailing NUL (no payload) must also be rejected.
    VERIFY(!Validation::isValidIpAddress(withNul("127.0.0.1", "")));
    // Scope-id suffix stays rejected (pre-existing guard).
    VERIFY(!Validation::isValidIpAddress("fe80::1%eth0"));
}

void testIsValidIpv4Address()
{
    VERIFY(Validation::isValidIpv4Address("10.0.0.1"));
    VERIFY(!Validation::isValidIpv4Address("::1"));
    VERIFY(!Validation::isValidIpv4Address(withNul("10.0.0.1", kPayload)));
    VERIFY(!Validation::isValidIpv4Address(withNul("10.0.0.1", "")));
}

void testIsValidIpCidr()
{
    VERIFY(Validation::isValidIpCidr("10.0.0.0/24"));
    VERIFY(Validation::isValidIpCidr("fd00::/64"));
    VERIFY(Validation::isValidIpCidr("10.0.0.1"));
    VERIFY(!Validation::isValidIpCidr("10.0.0.0/33"));
    // NUL smuggled into the address part ahead of the slash (the slash branch parses via make_address).
    VERIFY(!Validation::isValidIpCidr(withNul("10.0.0.0", "/24")));
    VERIFY(!Validation::isValidIpCidr(withNul("10.0.0.0/24", kPayload)));
}

void testIsValidIpv4Cidr()
{
    VERIFY(Validation::isValidIpv4Cidr("192.168.0.0/16"));
    VERIFY(Validation::isValidIpv4Cidr("192.168.0.1"));
    VERIFY(!Validation::isValidIpv4Cidr("fd00::/64"));
    VERIFY(!Validation::isValidIpv4Cidr(withNul("192.168.0.0", "/16")));
    VERIFY(!Validation::isValidIpv4Cidr(withNul("192.168.0.1", kPayload)));
}

void testIsValidPeerEndpoint()
{
    VERIFY(Validation::isValidPeerEndpoint("1.2.3.4:51820"));
    VERIFY(!Validation::isValidPeerEndpoint("1.2.3.4:70000"));
    VERIFY(!Validation::isValidPeerEndpoint("example.com:51820"));
    // NUL in the host part: rfind(':') still finds the real port, host parses via make_address.
    VERIFY(!Validation::isValidPeerEndpoint(withNul("1.2.3.4", kPayload + ":51820")));
}

void testIsValidDomain()
{
    VERIFY(Validation::isValidDomain("dns.quad9.net"));
    VERIFY(Validation::isValidDomain("1dot1dot1dot1.cloudflare-dns.com"));
    VERIFY(Validation::isValidDomain("*.company.int"));
    VERIFY(!Validation::isValidDomain(""));
    VERIFY(!Validation::isValidDomain("1.2.3.4"));
    VERIFY(!Validation::isValidDomain("127.1"));
    VERIFY(!Validation::isValidDomain("[::1]"));
    VERIFY(!Validation::isValidDomain("::1"));
    VERIFY(!Validation::isValidDomain(withNul("dns.quad9.net", kPayload)));
}

void testNormalizeAddress()
{
    VERIFY(Validation::normalizeAddress("dns.quad9.net") == "dns.quad9.net");
    VERIFY(Validation::normalizeAddress("dns.quad9.net:443") == "dns.quad9.net:443");
    VERIFY(Validation::normalizeAddress("1dot1dot1dot1.cloudflare-dns.com:443") == "1dot1dot1dot1.cloudflare-dns.com:443");
    VERIFY(Validation::normalizeAddress("https://dns.controld.com/abcd?int=ws") == "https://dns.controld.com/abcd?int=ws");
    VERIFY(Validation::normalizeAddress("1dot1dot1dot1.cloudflare-dns.com:0").empty());
    VERIFY(Validation::normalizeAddress("1dot1dot1dot1.cloudflare-dns.com:").empty());
    VERIFY(Validation::normalizeAddress("1dot1dot1dot1.cloudflare-dns.com:65536").empty());
    VERIFY(Validation::normalizeAddress("1dot1dot1dot1.cloudflare-dns.com:+443").empty());
    VERIFY(Validation::normalizeAddress(":443").empty());
    VERIFY(Validation::normalizeAddress("[::1]:443") == "[::1]:443");
    VERIFY(Validation::normalizeAddress("127.1:443").empty());
    VERIFY(Validation::normalizeAddress("[2620:fe::fe]:53") == "[2620:fe::fe]:53");
    VERIFY(Validation::normalizeAddress("[2620:fe::fe]").empty());
    VERIFY(Validation::normalizeAddress("[2620:fe::fe]:0").empty());
    VERIFY(Validation::normalizeAddress("[fe80::1%en0]:53").empty());
    VERIFY(Validation::normalizeAddress("[dns.quad9.net]:53").empty());
    VERIFY(Validation::normalizeAddress(withNul("dns.quad9.net", ":443")).empty());
}

void testIpLists()
{
    VERIFY(Validation::isValidIpList("1.1.1.1, 8.8.8.8"));
    VERIFY(!Validation::isValidIpList(withNul("1.1.1.1", kPayload) + ", 8.8.8.8"));

    VERIFY(Validation::isValidIpCidrList("10.0.0.0/24, fd00::/64"));
    VERIFY(!Validation::isValidIpCidrList(withNul("10.0.0.0", "/24") + ", fd00::/64"));
}

// types::IpAddress::isValid is the validator behind setFirewallOnBoot's ipTable entries, which are
// interpolated verbatim into the persisted boot firewall ruleset.
void testTypesIpAddress()
{
    VERIFY(types::IpAddress("1.2.3.4").isValid());
    VERIFY(types::IpAddress("::1").isValid());
    VERIFY(!types::IpAddress("garbage").isValid());
    VERIFY(!types::IpAddress(withNul("1.2.3.4", kPayload)).isValid());
    VERIFY(!types::IpAddress(withNul("1.2.3.4", "")).isValid());
}

} // namespace

int main()
{
    testIsValidIpAddress();
    testIsValidIpv4Address();
    testIsValidIpCidr();
    testIsValidIpv4Cidr();
    testIsValidPeerEndpoint();
    testIsValidDomain();
    testNormalizeAddress();
    testIpLists();
    testTypesIpAddress();

    if (g_failures == 0) {
        printf("All Validation NUL-smuggling tests passed.\n");
    } else {
        printf("%d check(s) failed.\n", g_failures);
    }
    return g_failures;
}
