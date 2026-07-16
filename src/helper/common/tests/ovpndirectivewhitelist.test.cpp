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

// Blocked directives at the top level are always stripped; allowed ones pass.
void testTopLevelDirectiveFiltering()
{
    std::string out;
    VERIFY(filters("plugin /e.so\nremote example.com 1194\n", out));
    VERIFY(!contains(out, "plugin"));
    VERIFY(contains(out, "remote example.com 1194"));
}

} // namespace

int main()
{
    testCloseTagPrefixDifferential();
    testCloseTagTrailingVariants();
    testLegitimateConfigPassesThrough();
    testOpenerTrailingDataNotBlock();
    testControlBytesRejected();
    testTopLevelDirectiveFiltering();

    if (g_failures == 0) {
        printf("All OvpnDirectiveWhitelist differential tests passed.\n");
    } else {
        printf("%d check(s) failed.\n", g_failures);
    }
    return g_failures;
}
