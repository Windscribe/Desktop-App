// Differential regression tests for the OpenVPN directive filter. These pin the filter's behavior
// against OpenVPN's own inline-block parsing so a config the filter accepts can never smuggle a
// blocked top-level directive (e.g. "plugin") past it. No external test framework; returns the
// number of failed checks (0 on success).

#include <cstdio>
#include <string>

#include "../ovpn_directive_whitelist.h"

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

bool filters(const std::string &config, std::string &out)
{
    return OvpnDirectiveWhitelist::filterConfig(config, out);
}

bool contains(const std::string &haystack, const std::string &needle)
{
    return haystack.find(needle) != std::string::npos;
}

// The reported LPE payload: OpenVPN prefix-matches "</ca>X" as a close and parses "plugin" at the
// top level, so the filter must not treat the plugin line as inline CA data.
void testCloseTagPrefixDifferential()
{
    const std::string payload =
        "<ca>\n"
        "</ca>X\n"
        "plugin /attacker/owned/root-plugin.so\n"
        "\"<ca>\"\n"
        "</ca>\n";
    std::string out;
    VERIFY(filters(payload, out));
    VERIFY(!contains(out, "plugin"));
    VERIFY(!contains(out, "root-plugin.so"));
}

// Variations on the prefix close: any trailing byte after the canonical close must end the block,
// so the standalone "plugin" directive on the next line is seen at top level and stripped. (Trailing
// text on the close line itself is discarded by OpenVPN, so we key off the plugin's argument.)
void testCloseTagTrailingVariants()
{
    const char *closers[] = {"</ca>x", "</ca> foo", "</ca>\tbar", "</ca>#c"};
    for (const char *closer : closers) {
        std::string config = std::string("<ca>\ndata\n") + closer + "\nplugin /injected.so\n";
        std::string out;
        VERIFY(filters(config, out));
        VERIFY(!contains(out, "/injected.so"));
    }
}

// A legitimate, well-formed config must still pass its inline blocks through untouched.
void testLegitimateConfigPassesThrough()
{
    const std::string config =
        "client\n"
        "remote example.com 1194\n"
        "<ca>\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBdummycertdata\n"
        "-----END CERTIFICATE-----\n"
        "</ca>\n";
    std::string out;
    VERIFY(filters(config, out));
    VERIFY(contains(out, "-----BEGIN CERTIFICATE-----"));
    VERIFY(contains(out, "MIIBdummycertdata"));
    VERIFY(contains(out, "remote example.com 1194"));
    // A close with trailing junk is normalized to the canonical close, matching OpenVPN,
    // which discards anything after the closing tag on that line.
    std::string norm;
    VERIFY(filters("<ca>\nMIIBdummy\n</ca>trailing-junk\n", norm));
    VERIFY(contains(norm, "</ca>\n"));
    VERIFY(!contains(norm, "trailing-junk"));
    // Case of the closing tag is preserved so it still matches its opener.
    std::string mixed;
    VERIFY(filters("<CA>\nMIIBdummy\n</CA>x\n", mixed));
    VERIFY(contains(mixed, "</CA>\n"));
    // A close with only trailing whitespace remains a valid close.
    const std::string trailingWs = "<ca>\nMIIBdummy\n</ca>   \nplugin /e.so\n";
    std::string out2;
    VERIFY(filters(trailingWs, out2));
    VERIFY(contains(out2, "MIIBdummy"));
    VERIFY(!contains(out2, "plugin"));
}

// A fake opener with trailing data is not an opener for OpenVPN, so the filter must not enter
// pass-through on it and let a following blocked directive survive.
void testOpenerTrailingDataNotBlock()
{
    const std::string config =
        "<ca> trailing\n"
        "plugin /e.so\n"
        "</ca>\n";
    std::string out;
    VERIFY(filters(config, out));
    VERIFY(!contains(out, "plugin"));
}

// Control bytes make the two parsers disagree on line boundaries; such configs are rejected outright.
void testControlBytesRejected()
{
    std::string config = "<ca>\n</ca>";
    config.push_back('\0');
    config += "\nplugin /e.so\n<ca>\ndata\n</ca>\n";
    std::string out = "unchanged";
    VERIFY(!filters(config, out));
    VERIFY(out == "unchanged");
}

// A close tag at the END of an overlong physical line is invisible to std::getline (one line, still
// inside the block) but the bundled OpenVPN's fixed-size inline reader (fgets, OPTION_LINE_SIZE ==
// 4096) splits the line and sees "</ca>" at the head of the next chunk, closing the block and
// parsing the following "plugin" at top level. Configs with a line long enough to be split are
// rejected outright so the two readers can never disagree on line boundaries.
void testOverlongInlineLineRejected()
{
    std::string payload = "<ca>\n";
    payload += std::string(4095, 'A');
    payload += "</ca>\n";
    payload += "plugin /attacker/owned/root-plugin.so\n";
    payload += "\"<ca>\"\n";
    payload += "</ca>\n";
    std::string out = "unchanged";
    VERIFY(!filters(payload, out));
    VERIFY(out == "unchanged");

    // A trailing '\r' (CRLF) counts toward the physical line length, matching fgets, so an otherwise
    // max-length line that carries a CR is still rejected.
    std::string crlf = "<ca>\n" + std::string(4094, 'A') + "\r\n</ca>\r\n";
    std::string crlfOut = "unchanged";
    VERIFY(!filters(crlf, crlfOut));
    VERIFY(crlfOut == "unchanged");
}

// The largest line the inline reader never splits (OPTION_LINE_SIZE - 2 == 4094 bytes, leaving room
// for the '\n') must still pass, so legitimate configs are not rejected.
void testMaxLengthLineAccepted()
{
    const std::string longData(4094, 'A');
    std::string config = "<ca>\n" + longData + "\n</ca>\n";
    std::string out;
    VERIFY(filters(config, out));
    VERIFY(contains(out, longData));
}

// A bare "--" (optionally with trailing arguments) has no directive name and maps to no real option,
// so it is stripped. A real directive with an explicit "--" prefix must still pass.
void testBareDoubleDashRejected()
{
    const char *bare[] = {"--", "-- ", "--\tfoo", "-- plugin /e.so"};
    for (const char *line : bare) {
        std::string config = std::string("client\n") + line + "\nremote example.com 1194\n";
        std::string out;
        VERIFY(filters(config, out));
        VERIFY(contains(out, "client"));
        VERIFY(contains(out, "remote example.com 1194"));
        VERIFY(!contains(out, "--"));
        VERIFY(!contains(out, "plugin"));
    }

    std::string out;
    VERIFY(filters("--client\n--remote example.com 1194\n", out));
    VERIFY(contains(out, "client"));
    VERIFY(contains(out, "remote example.com 1194"));
}

// Blocked directives at the top level are always stripped; allowed ones pass.
void testTopLevelDirectiveFiltering()
{
    std::string out;
    VERIFY(filters("plugin /e.so\nremote example.com 1194\n", out));
    VERIFY(!contains(out, "plugin"));
    VERIFY(contains(out, "remote example.com 1194"));
}

// dhcp-option is whitelisted by name, but its DNS value reaches the root DNS hook scripts. The
// filter must reject any DNS-family value (DNS, DNS6, and the alias spellings) that isn't a
// single bare IP.
void testDhcpOptionValueFiltering()
{
    std::string out;

    // Legitimate single-address forms (v4 or v6) pass through untouched.
    VERIFY(filters("dhcp-option DNS 10.255.255.2\n", out));
    VERIFY(contains(out, "dhcp-option DNS 10.255.255.2"));
    VERIFY(filters("dhcp-option DNS 2606:4700:4700::1111\n", out));
    VERIFY(contains(out, "2606:4700:4700::1111"));

    // The reported injection: a quoted DNS value hiding a busctl option must be stripped. Quoting is
    // never needed for a DNS value, so any quote (or backslash) rejects the line outright.
    VERIFY(filters("dhcp-option DNS \"1.2.3.4 --address=unixexec:path=/tmp/x\"\n", out));
    VERIFY(!contains(out, "--address"));
    VERIFY(!contains(out, "unixexec"));

    // Unquoted extra tokens after the address are stripped too.
    VERIFY(filters("dhcp-option DNS 1.2.3.4 --address=unixexec:path=/tmp/x\n", out));
    VERIFY(!contains(out, "--address"));

    // Quoting the whole "DNS <payload>" as one parameter can't smuggle it either (quote rejected).
    VERIFY(filters("dhcp-option \"DNS 1.2.3.4 --address=unixexec:path=/tmp/x\"\n", out));
    VERIFY(!contains(out, "--address"));

    // The subtype is matched case-insensitively (the hooks lowercase it): lowercase dns and dns6
    // are validated the same as their uppercase forms.
    VERIFY(filters("dhcp-option dns 10.255.255.2\n", out));
    VERIFY(contains(out, "dhcp-option dns 10.255.255.2"));
    VERIFY(filters("dhcp-option dns6 2606:4700:4700::1111\n", out));
    VERIFY(contains(out, "dhcp-option dns6 2606:4700:4700::1111"));

    // dhcp-option DNS takes exactly one address; a second one means the whole line is rejected.
    VERIFY(filters("dhcp-option DNS 1.2.3.4 5.6.7.8\n", out));
    VERIFY(!contains(out, "5.6.7.8"));

    // A non-IP DNS value is rejected.
    VERIFY(filters("dhcp-option DNS not-an-ip\n", out));
    VERIFY(!contains(out, "not-an-ip"));

    // OpenVPN 2.7 accepts DNS6 like DNS (one v4-or-v6 address), so it passes with a valid IP but is
    // still rejected when the value smuggles extra tokens.
    VERIFY(filters("dhcp-option DNS6 2606:4700:4700::1111\n", out));
    VERIFY(contains(out, "dhcp-option DNS6 2606:4700:4700::1111"));
    VERIFY(filters("dhcp-option DNS6 1.2.3.4 --address=unixexec:path=/tmp/x\n", out));
    VERIFY(!contains(out, "--address"));

    // update-systemd-resolved dispatches process_<subtype> with '-' mapped to '_', so the
    // DNS-IPV4/DNS-IPV6 spellings reach the same address sinks and get the same validation.
    VERIFY(filters("dhcp-option DNS-IPV4 1.2.3.4\n", out));
    VERIFY(contains(out, "dhcp-option DNS-IPV4 1.2.3.4"));
    VERIFY(filters("dhcp-option DNS-IPV4 unixexec:path=/tmp/x.1.2.3\n", out));
    VERIFY(!contains(out, "unixexec"));
    VERIFY(filters("dhcp-option DNS_IPV6 2606:4700:4700::1111\n", out));
    VERIFY(contains(out, "dhcp-option DNS_IPV6 2606:4700:4700::1111"));
    VERIFY(filters("dhcp-option DNS_IPV4 1.2.3.4 5.6.7.8\n", out));
    VERIFY(!contains(out, "5.6.7.8"));
    VERIFY(filters("dhcp-option dns_ipv6 2606:4700:4700::1111\n", out));
    VERIFY(contains(out, "dhcp-option dns_ipv6 2606:4700:4700::1111"));
    VERIFY(filters("dhcp-option Dns-IpV4 not-an-ip\n", out));
    VERIFY(!contains(out, "not-an-ip"));

    // A DNS-family subtype with no value at all is rejected.
    VERIFY(filters("dhcp-option DNS\n", out));
    VERIFY(!contains(out, "DNS"));

    // An IPV4 alias carrying a v6 literal (or IPV6 carrying a v4) is rejected: those spellings are
    // dispatched to the family-specific hook, so the address must match the family.
    VERIFY(filters("dhcp-option DNS-IPV4 2606:4700:4700::1111\n", out));
    VERIFY(!contains(out, "2606:4700:4700::1111"));
    VERIFY(filters("dhcp-option DNS-IPV6 1.2.3.4\n", out));
    VERIFY(!contains(out, "1.2.3.4"));

    // The "--"-prefixed spelling of the directive is validated the same way.
    VERIFY(filters("--dhcp-option DNS 1.2.3.4\n", out));
    VERIFY(contains(out, "dhcp-option DNS 1.2.3.4"));
    VERIFY(filters("--dhcp-option DNS 1.2.3.4 --address=unixexec:path=/tmp/x\n", out));
    VERIFY(!contains(out, "--address"));

    // A high-byte (non-ASCII) character in the address fails IP validation and drops the line.
    VERIFY(filters("dhcp-option DNS 1.2.3.4\xC3\xA9\n", out));
    VERIFY(!contains(out, "1.2.3.4"));

    // Other dhcp-option subtypes are unaffected.
    VERIFY(filters("dhcp-option DOMAIN-ROUTE .\n", out));
    VERIFY(contains(out, "dhcp-option DOMAIN-ROUTE ."));
    VERIFY(filters("dhcp-option DOMAIN example.com\n", out));
    VERIFY(contains(out, "dhcp-option DOMAIN example.com"));

    // A trailing inline comment is not part of the value; the DNS line still passes, even when the
    // comment itself contains quotes or backslashes.
    VERIFY(filters("dhcp-option DNS 8.8.8.8 # my dns\n", out));
    VERIFY(contains(out, "dhcp-option DNS 8.8.8.8"));
    VERIFY(filters("dhcp-option DNS 8.8.8.8 ;note\n", out));
    VERIFY(contains(out, "dhcp-option DNS 8.8.8.8"));
    VERIFY(filters("dhcp-option DNS 8.8.8.8 # o'brien's \"dns\"\n", out));
    VERIFY(contains(out, "dhcp-option DNS 8.8.8.8"));
}

// The dns (OpenVPN 2.6) directive: address-list IPs are validated (a non-IP token rejects the line),
// while the non-address forms carry no IP and pass through.
void testDnsDirectiveFiltering()
{
    std::string out;

    VERIFY(filters("dns server 0 address 1.1.1.1\n", out));
    VERIFY(contains(out, "dns server 0 address 1.1.1.1"));
    VERIFY(filters("dns server 0 address 1.1.1.1 8.8.8.8\n", out));
    VERIFY(contains(out, "8.8.8.8"));
    VERIFY(filters("dns server 0 address 1.1.1.1:53\n", out));
    VERIFY(contains(out, "1.1.1.1:53"));
    VERIFY(filters("dns server 0 address 2606:4700:4700::1111\n", out));
    VERIFY(contains(out, "2606:4700:4700::1111"));
    VERIFY(filters("dns server 0 address [2606:4700:4700::1111]:853\n", out));
    VERIFY(contains(out, "[2606:4700:4700::1111]:853"));

    // A malformed address entry rejects the whole directive (not just its port): a bracketed form
    // without a port, a zero/out-of-range/non-numeric port.
    VERIFY(filters("dns server 0 address [2606:4700:4700::1111]\n", out));
    VERIFY(!contains(out, "2606:4700:4700::1111"));
    VERIFY(filters("dns server 0 address 1.1.1.1:0\n", out));
    VERIFY(!contains(out, "1.1.1.1"));
    VERIFY(filters("dns server 0 address 1.1.1.1:99999\n", out));
    VERIFY(!contains(out, "1.1.1.1"));
    VERIFY(filters("dns server 0 address 1.1.1.1:53x\n", out));
    VERIFY(!contains(out, "1.1.1.1"));
    VERIFY(filters("dns server 0 address 1.1.1.1:\n", out));
    VERIFY(!contains(out, "1.1.1.1"));
    VERIFY(filters("dns server 0 address [1.2.3.4]:53\n", out));
    VERIFY(!contains(out, "1.2.3.4"));

    // "address" with no value is an explicit reject, not a silent accept.
    VERIFY(filters("dns server 0 address\n", out));
    VERIFY(!contains(out, "address"));

    // A non-IP token anywhere in the address list rejects the whole line.
    VERIFY(filters("dns server 0 address 1.1.1.1 --address=unixexec:path=/tmp/x\n", out));
    VERIFY(!contains(out, "--address"));
    VERIFY(filters("dns server 0 address not-an-ip\n", out));
    VERIFY(!contains(out, "not-an-ip"));

    // A quoted value is rejected outright.
    VERIFY(filters("dns server 0 address \"1.1.1.1 --foo\"\n", out));
    VERIFY(!contains(out, "--foo"));

    // Non-address forms carry no IP and pass through untouched.
    VERIFY(filters("dns search-domains example.com\n", out));
    VERIFY(contains(out, "dns search-domains example.com"));
    VERIFY(filters("dns server 0 dnssec yes\n", out));
    VERIFY(contains(out, "dns server 0 dnssec yes"));
}

// The anti-censorship i1-i5 directives carry a quoted value with heavy punctuation. The client
// generator emits them and the helper's filter writes the config, so the filter must pass such a
// line through byte-for-byte — a stripped i-directive would silently break the connection. (This
// pins the generator/filter contract on the helper side.)
void testAntiCensorshipIParamsPassThrough()
{
    const std::string iparam =
        "i1 \"<b 0xf0-9a_7c=3d.2e:1f/4a+5b,6c;7d(8e)9f[a0]b1{c2}d3|e4*f5&g6^h7%i8$j9#k0!l1?m2@n3~o4`p5>\"";
    const std::string config = iparam + "\nudp-stuffing\ntcp-split-reset\n";
    std::string out;
    VERIFY(filters(config, out));
    VERIFY(contains(out, iparam));
    VERIFY(contains(out, "udp-stuffing"));
    VERIFY(contains(out, "tcp-split-reset"));
}

} // namespace

int main()
{
    testCloseTagPrefixDifferential();
    testCloseTagTrailingVariants();
    testLegitimateConfigPassesThrough();
    testOpenerTrailingDataNotBlock();
    testControlBytesRejected();
    testOverlongInlineLineRejected();
    testMaxLengthLineAccepted();
    testBareDoubleDashRejected();
    testTopLevelDirectiveFiltering();
    testDhcpOptionValueFiltering();
    testDnsDirectiveFiltering();
    testAntiCensorshipIParamsPassThrough();

    if (g_failures == 0) {
        printf("All OvpnDirectiveWhitelist differential tests passed.\n");
    } else {
        printf("%d check(s) failed.\n", g_failures);
    }
    return g_failures;
}
