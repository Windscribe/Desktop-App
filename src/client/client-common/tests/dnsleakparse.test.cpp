// Regression tests for the OS resolver snapshot parsers, shared by the helper's nft DROP list and the
// client's firewall allowlist. Both used to hold their own copy, and every case below is one where at
// least one copy was wrong.
//
// The load-bearing case is `resolvectl dns` wrapping: with no tty, columns() falls back to 80 and the
// server list continues on lines indented 8 spaces with no "Link N (ifc):" label. Splitting every line on
// the first colon truncated a leading IPv6 into an invalid token and dropped colon-less IPv4 lines whole.
//
// The rest cover the extended-form suffixes — port, DoT server name, zone id — which the helper's copy
// never peeled and therefore discarded.
//
// Addresses are RFC 5737 / RFC 3849 documentation ranges. No test framework; exit status is pass/fail.

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "types/ipaddress.h"
#include "utils/dns_utils/dnsleakparse.h"

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

// Self-registering so a case cannot be written and then silently not run: defining it enrolls it, and
// there is no hand-maintained call list in main() to forget. Statics in one TU initialize in definition
// order, so the run order is still the order below.
std::vector<void (*)()> &registry()
{
    static std::vector<void (*)()> r;
    return r;
}

struct Register {
    explicit Register(void (*fn)()) { registry().push_back(fn); }
};

#define TEST(name)                     \
    static void name();                \
    static const Register reg_##name(&name); \
    static void name()

bool has(const std::vector<std::string> &v, const std::string &s)
{
    return std::find(v.begin(), v.end(), s) != v.end();
}

// The continuation indent resolvectl emits, spelled out once so the fixtures below can't drift from it.
const char *const kInd = "        ";

TEST(testUnwrappedStillWorks)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Global:\n"
                            "Link 2 (enp0s31f6): 192.0.2.1 192.0.2.2\n"
                            "Link 3 (wlp0s20f3): 2001:db8::1\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(v4.size() == 2);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v4, "192.0.2.2"));
    VERIFY(v6.size() == 1);
    VERIFY(has(v6, "2001:db8::1"));
}

// Defect 1: the continuation begins with an IPv6 address, so the old first-colon split yielded the
// remainder after the first hextet ("db8::1"), which failed validation and was discarded.
TEST(testWrappedIpv6Continuation)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 2 (enp0s31f6): 192.0.2.1 192.0.2.2 2001:db8:aaaa:bbbb::1\n")
                            + kInd + "2001:db8::1\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(has(v6, "2001:db8:aaaa:bbbb::1"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(!has(v6, "db8::1"));
    VERIFY(v6.size() == 2);
    VERIFY(v4.size() == 2);
}

// Defect 2 (the #1893 regression): the continuation holds only IPv4 addresses, so it contains no colon
// at all and the old `if (colon == npos) continue;` discarded the whole line.
TEST(testWrappedIpv4OnlyContinuation)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 2 (enp0s31f6): 192.0.2.1 192.0.2.2 192.0.2.3 192.0.2.4 192.0.2.5 192.0.2.6\n")
                            + kInd + "192.0.2.7 192.0.2.8\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(v4.size() == 8);
    VERIFY(has(v4, "192.0.2.7"));
    VERIFY(has(v4, "192.0.2.8"));
    VERIFY(v6.empty());
}

// A continuation mixing both families: the leading v6 token used to be truncated while the rest of the
// line survived, so this pins that no token is lost.
TEST(testWrappedMixedContinuation)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 2 (eth0): 192.0.2.1 192.0.2.2 192.0.2.3 2001:db8::1\n")
                            + kInd + "2001:db8::2 192.0.2.9\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(has(v6, "2001:db8::2"));
    VERIFY(has(v4, "192.0.2.9"));
    VERIFY(v4.size() == 4);
    VERIFY(v6.size() == 2);
}

// The "Global:" label wraps through the same code path as a link label.
TEST(testWrappedGlobalLine)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Global: 192.0.2.1 192.0.2.2 192.0.2.3 192.0.2.4 192.0.2.5 192.0.2.6\n")
                            + kInd + "192.0.2.7\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(v4.size() == 7);
    VERIFY(has(v4, "192.0.2.7"));
}

// A continuation carries no label, so the tunnel exemption has to be inherited from the header above
// it — otherwise the VPN's own resolvers land in the drop list on a wrapped line.
TEST(testTunnelContinuationInherited)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 5 (wgwindscribe0): 10.255.255.2 2001:db8:ffff::1 2001:db8:ffff::2\n")
                            + kInd + "10.255.255.3 2001:db8:ffff::3\n";
    DnsLeakParse::parseResolvectl(out, "wgwindscribe0", v4, v6);
    VERIFY(v4.empty());
    VERIFY(v6.empty());
}

// ...and the exemption must not persist past the tunnel's own block onto the next link.
TEST(testExemptionResetsAtNextLink)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 5 (wgwindscribe0): 10.255.255.2 2001:db8:ffff::1 2001:db8:ffff::2\n")
                            + kInd + "10.255.255.3\n"
                            + "Link 2 (enp0s31f6): 192.0.2.1 192.0.2.2 192.0.2.3 192.0.2.4 192.0.2.5 192.0.2.6\n"
                            + kInd + "192.0.2.7\n";
    DnsLeakParse::parseResolvectl(out, "wgwindscribe0", v4, v6);
    VERIFY(!has(v4, "10.255.255.2"));
    VERIFY(!has(v4, "10.255.255.3"));
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v4, "192.0.2.7"));
    VERIFY(v4.size() == 7);
    VERIFY(v6.empty());
}

// A label-less, colon-less line is not a header, so it must clear any pending exemption rather than
// letting the tunnel's skip carry onto whatever follows (fail toward blocking, not toward leaking).
TEST(testColonlessHeaderClearsExemption)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 5 (wgwindscribe0): 10.255.255.2\n")
                            + "no-colon-noise\n"
                            + kInd + "192.0.2.7\n";
    DnsLeakParse::parseResolvectl(out, "wgwindscribe0", v4, v6);
    VERIFY(has(v4, "192.0.2.7"));
    VERIFY(!has(v4, "10.255.255.2"));
}

// Snapshot entries are stored canonical (IpAddress::toString) because removeMatching strips the tunnel's
// own resolvers by address value; a non-canonical spelling would slip that and get blocked. The zone id
// is the numeric ifindex systemd actually emits, not an interface name.
TEST(testCanonicalizationAndZoneStrip)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 2 (eth0): 2001:0DB8:0000:0000:0000:0000:0000:0001\n")
                            + kInd + "fe80::1%3\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(has(v6, "fe80::1"));
    VERIFY(v6.size() == 2);
}

// Duplicates across links (the common case: the same router on two interfaces) collapse via pushUnique.
TEST(testDedup)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Link 2 (eth0): 192.0.2.1\n"
                            "Link 3 (wlan0): 192.0.2.1\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(v4.size() == 1);
}

// systemd prints the extended server form, so a DNS-over-TLS resolver arrives with a "#servername"
// suffix. Peeling it is what gets the address into the snapshot at all; whether the resulting rule
// covers port 853 is the emitter's business, not this parser's.
TEST(testServerNameSuffix)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Link 2 (eth0): 192.0.2.1#dns.example.com 2001:db8::1#dns.example.com\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.size() == 1);
}

// An explicit port is retained in the packed form, because it is the only port that resolver answers on.
TEST(testPortSuffix)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Link 2 (eth0): 192.0.2.1:5353 [2001:db8::1]:853\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(has(v4, "192.0.2.1:5353"));
    VERIFY(has(v6, "[2001:db8::1]:853"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.size() == 1);
}

// All three suffixes at once, on a wrapped continuation. systemd emits the zone id as a numeric ifindex
// AFTER the closing bracket, not as an interface name inside it: in_addr_port_ifindex_name_to_string
// formats the ported IPv6 case as "[%s]:%u%%%i" plus the server name.
TEST(testAllSuffixesCombined)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 2 (eth0): [2001:db8::1]:853#dns.example.com\n")
                            + kInd + "[fe80::1]:5353%3#dns.example.com 192.0.2.1:5353#dns.example.com\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(has(v6, "[2001:db8::1]:853"));
    VERIFY(has(v6, "[fe80::1]:5353"));
    VERIFY(has(v4, "192.0.2.1:5353"));
    VERIFY(v6.size() == 2);
    VERIFY(v4.size() == 1);
}

// A single colon means "v4:port", but two or more must be left alone or every bare IPv6 literal would
// be truncated to its first hextet — the original defect, reintroduced from the other direction. "::1" is
// here to show it survives the split and is then dropped as a stub, not truncated; the minimal two-colon
// forms are pinned against the codec directly in testEndpointCodec.
TEST(testBareIpv6NotTreatedAsPort)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Link 2 (eth0): ::1 fe80::1 2001:db8::1\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(!has(v6, "::1"));
    VERIFY(has(v6, "fe80::1"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(v6.size() == 2);
    VERIFY(v4.empty());
}

TEST(testUnterminatedBracketDropped)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): [2001:db8::1 192.0.2.1\n", "", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(v6.empty());
}

// The client passes "(tun"/"(utun" because its VPN context may not be set yet...
TEST(testExtraExemptHeaders)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Link 2 (eth0): 192.0.2.1\n"
                            "Link 7 (tun0): 192.0.2.50\n"
                            "Link 8 (utun3): 192.0.2.51\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6, {"(tun", "(utun"});
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(!has(v4, "192.0.2.50"));
    VERIFY(!has(v4, "192.0.2.51"));
    VERIFY(v4.size() == 1);
}

// ...and the helper passes none, so another VPN's tun resolvers stay in its DROP list. Blanket-skipping
// tun here would leak them.
TEST(testNoExtraExemptBlocksOtherTunnels)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Link 7 (tun0): 192.0.2.50\n"
                            "Link 9 (wgwindscribe0): 10.255.255.2\n";
    DnsLeakParse::parseResolvectl(out, "wgwindscribe0", v4, v6);
    VERIFY(has(v4, "192.0.2.50"));
    VERIFY(!has(v4, "10.255.255.2"));
    VERIFY(v4.size() == 1);
}

// parseNmcli is shared too; pin its device-scoped skip.
TEST(testNmcli)
{
    std::vector<std::string> v4, v6;
    const std::string out = "GENERAL.DEVICE:      enp0s31f6\n"
                            "IP4.DNS[1]:          192.0.2.1\n"
                            "IP6.DNS[1]:          2001:db8::1\n"
                            "GENERAL.DEVICE:      wgwindscribe0\n"
                            "IP4.DNS[1]:          10.255.255.2\n";
    DnsLeakParse::parseNmcli(out, "wgwindscribe0", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(!has(v4, "10.255.255.2"));
    VERIFY(v4.size() == 1);
}

// Family is decided by the address, never by the row label. Routing on the label would pass every other
// nmcli case here, and would put a v6 address in the v4 list, rendering `ip daddr <v6>` — malformed nft
// that fails the whole transaction and leaves no drop rules at all.
TEST(testNmcliFamilyFromAddressNotLabel)
{
    std::vector<std::string> v4, v6;
    const std::string out = "GENERAL.DEVICE:      eth0\n"
                            "IP6.DNS[1]:          192.0.2.1\n"
                            "IP4.DNS[1]:          2001:db8::1\n";
    DnsLeakParse::parseNmcli(out, "", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.size() == 1);
}

// ...and the device exemption must reset at the next GENERAL.DEVICE row, or every device following the
// tunnel's block would be dropped. Turning curDev into a bool skipDev — as parseResolvectl above is
// written — and forgetting the reset would pass every other nmcli case here.
TEST(testNmcliExemptionResetsAtNextDevice)
{
    std::vector<std::string> v4, v6;
    const std::string out = "GENERAL.DEVICE:      wgwindscribe0\n"
                            "IP4.DNS[1]:          10.255.255.2\n"
                            "GENERAL.DEVICE:      enp0s31f6\n"
                            "IP4.DNS[1]:          192.0.2.1\n";
    DnsLeakParse::parseNmcli(out, "wgwindscribe0", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(!has(v4, "10.255.255.2"));
    VERIFY(v4.size() == 1);
}

// The client's nmcli copy pushed the raw field value with no suffix strip and no validation, so a zoned
// or DoT-suffixed row reached the firewall allowlist verbatim. Shared parsing peels and validates.
TEST(testNmcliSuffixesAndValidation)
{
    std::vector<std::string> v4, v6;
    const std::string out = "GENERAL.DEVICE:      enp0s31f6\n"
                            "IP6.DNS[1]:          fe80::1%enp0s31f6\n"
                            "IP4.DNS[1]:          192.0.2.1#dns.example.com\n"
                            "IP4.DNS[2]:          not-an-address\n";
    DnsLeakParse::parseNmcli(out, "", v4, v6);
    VERIFY(has(v6, "fe80::1"));
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.size() == 1);
}

// The client's copy bailed on any row that did not split into exactly two fields, so a multi-server row
// was discarded whole.
TEST(testNmcliMultiServerRow)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseNmcli("GENERAL.DEVICE:      eth0\nIP4.DNS[1]:          192.0.2.1 192.0.2.2\n",
                             "", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v4, "192.0.2.2"));
    VERIFY(v4.size() == 2);
}

// Field names sit at column 0, so the match is anchored: an address appearing in some other field's
// value must not be mistaken for a DNS row.
TEST(testNmcliFieldMatchAnchored)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseNmcli("GENERAL.DEVICE:      eth0\nGENERAL.CONNECTION:  my IP4.DNS 192.0.2.9\n",
                             "", v4, v6);
    VERIFY(v4.empty());
    VERIFY(v6.empty());
}

// resolv.conf: the client's copy stripped "%zone" but not "#servername", and validated nothing.
TEST(testResolvConf)
{
    std::vector<std::string> v4, v6;
    const std::string text = "# comment\n"
                             "search example.com\n"
                             "nameserver 192.0.2.1\n"
                             "nameserver fe80::1%3\n"
                             "nameserver 2001:db8::1#dns.example.com\n"
                             "nameserver garbage\n"
                             "options edns0\n";
    DnsLeakParse::parseResolvConfText(text, v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v6, "fe80::1"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.size() == 2);
}

// Only the first argument of a nameserver row is a resolver, as glibc reads it. An address inside a
// trailing comment must not become one: on the client these are firewall-allowlisted, so an address the
// user never configured would become reachable with the kill switch on.
TEST(testResolvConfIgnoresTrailingComment)
{
    std::vector<std::string> v4, v6;
    const std::string text = "nameserver 192.0.2.1 # was 198.51.100.9\n"
                             "nameserver 192.0.2.2 ; also 198.51.100.10\n";
    DnsLeakParse::parseResolvConfText(text, v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v4, "192.0.2.2"));
    VERIFY(!has(v4, "198.51.100.9"));
    VERIFY(!has(v4, "198.51.100.10"));
    VERIFY(v4.size() == 2);
    VERIFY(v6.empty());
}

// "nameserver" must be the leading keyword, not merely present on the line.
TEST(testResolvConfKeywordAnchored)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvConfText("# nameserver 192.0.2.9\nsortlist nameserver 192.0.2.8\n", v4, v6);
    VERIFY(v4.empty());
    VERIFY(v6.empty());
}

// parseResolvConfFile is the entry point both consumers actually call, yet every resolv.conf case above
// exercises only the text form, leaving the file wrapper unguarded.
TEST(testResolvConfFile)
{
    // Per-pid so two checkouts building on one machine cannot delete each other's fixture mid-test.
    const std::string path = "/tmp/dnsleakparse.test." + std::to_string(getpid()) + ".resolv.conf";
    {
        std::ofstream f(path);
        f << "# written by dnsleakparse.test\n"
             "nameserver 192.0.2.1\n"
             "nameserver 2001:db8::1\n";
        VERIFY(f.good());
    }
    std::vector<std::string> v4, v6;
    VERIFY(DnsLeakParse::parseResolvConfFile(path, v4, v6));
    std::remove(path.c_str());
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.size() == 1);

    // An absent file reports false rather than throwing or looking like "no nameserver lines" — this is
    // the fallback of the fallback, so the caller must be able to tell those apart.
    std::vector<std::string> missingV4, missingV6;
    VERIFY(!DnsLeakParse::parseResolvConfFile("/tmp/dnsleakparse.test.no-such-file", missingV4, missingV6));
    VERIFY(missingV4.empty());
    VERIFY(missingV6.empty());
}

// The exemption matches "(iface)" with the parentheses, not a bare substring, so a link whose name merely
// begins with the tunnel's must not inherit it and escape the drop list.
TEST(testExemptionRequiresFullLinkName)
{
    std::vector<std::string> v4, v6;
    const std::string out = "Link 4 (wgwindscribe0.100): 192.0.2.50\n"
                            "Link 5 (wgwindscribe0): 10.255.255.2\n";
    DnsLeakParse::parseResolvectl(out, "wgwindscribe0", v4, v6);
    VERIFY(has(v4, "192.0.2.50"));
    VERIFY(!has(v4, "10.255.255.2"));
    VERIFY(v4.size() == 1);
}

// glibc and musl both anchor the keyword at column 0, so an indented row is a disabled entry — indenting
// is a common way to comment one out. Reading it back would allowlist, through the client's firewall, an
// address the resolver never queries.
TEST(testResolvConfIgnoresIndentedLine)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvConfText("nameserver 192.0.2.1\n"
                                      "    nameserver 198.51.100.9\n"
                                      "\tnameserver 198.51.100.10\n",
                                      v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(!has(v4, "198.51.100.9"));
    VERIFY(!has(v4, "198.51.100.10"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.empty());
}

// addAddress is the module's only public way in, and the helper calls it directly rather than through a
// parser — with an empty string whenever the pre-VPN gateway is unknown, which is every apply on the
// second entry point. No parser can produce an empty token, so this is the only cover for that branch.
TEST(testAddAddressDirect)
{
    std::vector<std::string> v4, v6, rejected;
    const auto sink = [&rejected](const std::string &t) { rejected.push_back(t); };

    DnsLeakParse::addAddress("", v4, v6, sink);
    VERIFY(v4.empty());
    VERIFY(v6.empty());
    VERIFY(rejected.empty());

    // The shape the helper feeds it from inet_ntop, which must land canonical and unported.
    DnsLeakParse::addAddress("192.0.2.1", v4, v6, sink);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(v4.size() == 1);
    VERIFY(rejected.empty());

    // A token that is nothing but suffix strips to empty and is dropped, not rejected.
    DnsLeakParse::addAddress("#dns.example.com", v4, v6, sink);
    DnsLeakParse::addAddress("%3", v4, v6, sink);
    VERIFY(v4.size() == 1);
    VERIFY(rejected.empty());
}

// The packed form is the codec's business, not each consumer's. Round-trip both families with and without
// a port, and reject anything joinEndpoint could never have produced.
TEST(testEndpointCodec)
{
    VERIFY(types::joinEndpoint("192.0.2.1", 0) == "192.0.2.1");
    VERIFY(types::joinEndpoint("192.0.2.1", 5353) == "192.0.2.1:5353");
    VERIFY(types::joinEndpoint("2001:db8::1", 0) == "2001:db8::1");
    VERIFY(types::joinEndpoint("2001:db8::1", 853) == "[2001:db8::1]:853");

    std::string addr;
    uint16_t port = 0;
    VERIFY(types::splitEndpoint("192.0.2.1", addr, port) && addr == "192.0.2.1" && port == 0);
    VERIFY(types::splitEndpoint("192.0.2.1:5353", addr, port) && addr == "192.0.2.1" && port == 5353);
    // A bare IPv6 must never be split on its own colons — the whole reason the form is bracketed.
    VERIFY(types::splitEndpoint("2001:db8::1", addr, port) && addr == "2001:db8::1" && port == 0);
    VERIFY(types::splitEndpoint("::1", addr, port) && addr == "::1" && port == 0);
    VERIFY(types::splitEndpoint("[2001:db8::1]:853", addr, port) && addr == "2001:db8::1" && port == 853);
    VERIFY(types::splitEndpoint("[2001:db8::1]", addr, port) && addr == "2001:db8::1" && port == 0);

    VERIFY(!types::splitEndpoint("[2001:db8::1", addr, port));
    VERIFY(!types::splitEndpoint("[2001:db8::1]x853", addr, port));
    VERIFY(!types::splitEndpoint("192.0.2.1:0", addr, port));
    VERIFY(!types::splitEndpoint("192.0.2.1:70000", addr, port));
    VERIFY(!types::splitEndpoint("192.0.2.1:abc", addr, port));
}

// removeMatching compares the ADDRESS, so the tunnel's own resolver is stripped whatever port it carries.
// Miss it and the helper emits a drop rule against the VPN's own DNS.
TEST(testRemoveMatchingIgnoresPort)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): 10.255.255.2:5353 192.0.2.1\n", "", v4, v6);
    VERIFY(v4.size() == 2);
    DnsLeakParse::removeMatching(v4, {"10.255.255.2"});
    VERIFY(!has(v4, "10.255.255.2:5353"));
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(v4.size() == 1);
}

// A port on a wrapped continuation — the interaction between the two fixes most likely to break.
TEST(testPortOnWrappedContinuation)
{
    std::vector<std::string> v4, v6;
    const std::string out = std::string("Link 2 (eth0): 192.0.2.1 192.0.2.2 192.0.2.3 192.0.2.4 192.0.2.5\n")
                            + kInd + "192.0.2.6:5353\n";
    DnsLeakParse::parseResolvectl(out, "", v4, v6);
    VERIFY(has(v4, "192.0.2.6:5353"));
    VERIFY(v4.size() == 6);
}

// An out-of-range port means we misread the token, so the whole entry goes rather than being kept with a
// guessed port.
TEST(testInvalidPortRejected)
{
    std::vector<std::string> v4, v6, rejected;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): 192.0.2.1:70000 192.0.2.2\n", "", v4, v6, {},
                                  [&rejected](const std::string &t) { rejected.push_back(t); });
    VERIFY(has(v4, "192.0.2.2"));
    VERIFY(v4.size() == 1);
    VERIFY(std::find(rejected.begin(), rejected.end(), "192.0.2.1:70000") != rejected.end());
}

// The local stub resolver is never blocked and never allowlisted, so it must not enter the snapshot at
// all: counted as a discovered resolver it suppressed both callers' "no resolvers found" warnings while
// producing no rule, which reads as success on a host with no protection.
TEST(testLocalStubNeverEntersSnapshot)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvConfText("nameserver 127.0.0.53\n", v4, v6);
    VERIFY(v4.empty());
    VERIFY(v6.empty());

    DnsLeakParse::parseResolvectl("Link 2 (eth0): 127.0.0.1 0.0.0.0 ::1 :: 192.0.2.1\n", "", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(v4.size() == 1);
    VERIFY(v6.empty());

    // A ported stub is still a stub.
    std::vector<std::string> pv4, pv6;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): 127.0.0.53:5353\n", "", pv4, pv6);
    VERIFY(pv4.empty());
}

// Every parser APPENDS and dedups against what is already present. The helper accumulates resolvectl and
// then nmcli into one pair of vectors, so a parser that cleared them would wipe the first source's
// resolvers out of the DROP list while every single-call test still passed.
TEST(testParsersAccumulate)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): 192.0.2.1 2001:db8::1\n", "", v4, v6);
    DnsLeakParse::parseNmcli("GENERAL.DEVICE:      wlan0\n"
                             "IP4.DNS[1]:          192.0.2.2\n"
                             "IP4.DNS[2]:          192.0.2.1\n"
                             "IP6.DNS[1]:          2001:db8::2\n",
                             "", v4, v6);
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(has(v4, "192.0.2.2"));
    VERIFY(has(v6, "2001:db8::1"));
    VERIFY(has(v6, "2001:db8::2"));
    // 192.0.2.1 is reported by both sources and must appear once.
    VERIFY(v4.size() == 2);
    VERIFY(v6.size() == 2);
    DnsLeakParse::parseResolvConfText("nameserver 192.0.2.3\n", v4, v6);
    VERIFY(v4.size() == 3);
}

// removeMatching is what keeps the tunnel's own resolvers out of the caller's list. It must compare by
// address value: the snapshot is canonical, so a differently spelled allowed entry would slip an exact
// string compare and the helper would emit a DROP rule against the VPN's own resolver.
TEST(testRemoveMatchingIgnoresSpelling)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): 192.0.2.1 2001:db8::1 2001:db8::2\n", "", v4, v6);
    DnsLeakParse::removeMatching(v6, {"2001:0DB8:0000:0000:0000:0000:0000:0001"});
    VERIFY(!has(v6, "2001:db8::1"));
    VERIFY(has(v6, "2001:db8::2"));
    VERIFY(v6.size() == 1);
    DnsLeakParse::removeMatching(v4, {"192.0.2.1"});
    VERIFY(v4.empty());
}

// An unparseable allowed entry is dropped and reported, not treated as a wildcard that strips the list.
TEST(testRemoveMatchingRejectsInvalid)
{
    std::vector<std::string> v4, v6;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): 192.0.2.1\n", "", v4, v6);
    std::vector<std::string> rejected;
    DnsLeakParse::removeMatching(v4, {"not-an-ip"},
                                 [&rejected](const std::string &t) { rejected.push_back(t); });
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(v4.size() == 1);
    VERIFY(rejected.size() == 1);
    VERIFY(std::find(rejected.begin(), rejected.end(), "not-an-ip") != rejected.end());
}

// The reject sink is how a dropped resolver stays diagnosable; pin that it actually fires and carries the
// offending token.
TEST(testRejectHandlerReceivesBadTokens)
{
    std::vector<std::string> v4, v6, rejected;
    DnsLeakParse::parseResolvectl("Link 2 (eth0): 192.0.2.1 garbage [2001:db8::1\n", "", v4, v6, {},
                                  [&rejected](const std::string &t) { rejected.push_back(t); });
    VERIFY(has(v4, "192.0.2.1"));
    VERIFY(rejected.size() == 2);
    VERIFY(std::find(rejected.begin(), rejected.end(), "garbage") != rejected.end());
    VERIFY(std::find(rejected.begin(), rejected.end(), "[2001:db8::1") != rejected.end());
}

} // namespace

int main()
{
    // ctest gives us a pipe, so stdout is fully buffered by default and every FAIL line printed before an
    // abort is discarded. Line-buffer it so a crash still leaves the trail that led up to it.
    setvbuf(stdout, nullptr, _IOLBF, 0);

    for (const auto &fn : registry()) {
        fn();
    }
    VERIFY(!registry().empty());

    if (g_failures == 0) {
        printf("All DnsLeakParse resolvectl wrap tests passed.\n");
    } else {
        printf("%d check(s) failed.\n", g_failures);
    }
    // Collapse to 1 rather than returning the count: an exit status is only 8 bits, so a 256-failure run
    // would report success. Not reachable today, but this file is a growing corpus.
    return g_failures ? 1 : 0;
}
