// Regression tests for the Windows helper address validators, the counterpart of the POSIX
// validation.test.cpp. These pin the guarantee that a client-supplied value carrying an embedded
// NUL cannot pass validation: the underlying parsers (InetPtonW, DnsValidateName_W, IIDFromString)
// read the c_str() and stop at the first NUL, so without an explicit guard a valid prefix would
// validate while the bytes after the NUL survive into text the service writes as SYSTEM. No
// external test framework; returns the number of failed checks (0 on success).

#include <Windows.h>

#include <cstdio>
#include <string>

#include "network_validation.h"

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

// Build a wide string with an embedded NUL, since a plain literal would terminate at the NUL. The
// suffix carries the smuggled payload (a newline plus an injected directive) that must never reach
// a sink.
std::wstring withNul(const std::wstring &prefix, const std::wstring &suffix)
{
    std::wstring s = prefix;
    s.push_back(L'\0');
    s += suffix;
    return s;
}

const std::wstring kPayload = L"\nplugin /tmp/root-plugin.so\n#";

void testIsValidIpAddress()
{
    VERIFY(NetworkValidation::isValidIpAddress(L"127.0.0.1"));
    VERIFY(NetworkValidation::isValidIpAddress(L"::1"));
    VERIFY(!NetworkValidation::isValidIpAddress(L"not-an-ip"));
    // The reported LPE payload shape: a valid prefix + NUL + injected directive.
    VERIFY(!NetworkValidation::isValidIpAddress(withNul(L"127.0.0.1", kPayload)));
    VERIFY(!NetworkValidation::isValidIpAddress(withNul(L"::1", kPayload)));
    // A bare trailing NUL (no payload) must also be rejected.
    VERIFY(!NetworkValidation::isValidIpAddress(withNul(L"127.0.0.1", L"")));
}

void testIsValidIpOrCidr()
{
    VERIFY(NetworkValidation::isValidIpOrCidr(L"10.0.0.0/24"));
    VERIFY(NetworkValidation::isValidIpOrCidr(L"fd00::/64"));
    VERIFY(NetworkValidation::isValidIpOrCidr(L"10.0.0.1"));
    VERIFY(!NetworkValidation::isValidIpOrCidr(L"10.0.0.0/33"));
    // NUL smuggled into the address part ahead of the slash.
    VERIFY(!NetworkValidation::isValidIpOrCidr(withNul(L"10.0.0.0", L"/24")));
    VERIFY(!NetworkValidation::isValidIpOrCidr(withNul(L"10.0.0.0/24", kPayload)));
}

void testIsValidHostname()
{
    VERIFY(NetworkValidation::isValidHostname(L"example.com"));
    VERIFY(!NetworkValidation::isValidHostname(L"bad host with spaces"));
    // Valid label prefix + NUL + injected content: DnsValidateName_W stops at the NUL, so the guard
    // is what rejects it.
    VERIFY(!NetworkValidation::isValidHostname(withNul(L"example.com", kPayload)));
    VERIFY(!NetworkValidation::isValidHostname(withNul(L"example.com", L"")));
}

void testIsValidPortList()
{
    VERIFY(NetworkValidation::isValidPortList(L"443"));
    VERIFY(NetworkValidation::isValidPortList(L"80,443,1000-2000"));
    VERIFY(!NetworkValidation::isValidPortList(L"70000"));
    // A NUL is not a digit, so the token parse rejects it.
    VERIFY(!NetworkValidation::isValidPortList(withNul(L"443", kPayload)));
}

void testIsValidGuid()
{
    VERIFY(NetworkValidation::isValidGuid(L"{4D36E972-E325-11CE-BFC1-08002BE10318}"));
    VERIFY(!NetworkValidation::isValidGuid(L"not-a-guid"));
    // Valid GUID prefix + NUL + trailing bytes: IIDFromString stops at the NUL, so the guard rejects it.
    VERIFY(!NetworkValidation::isValidGuid(withNul(L"{4D36E972-E325-11CE-BFC1-08002BE10318}", kPayload)));
    VERIFY(!NetworkValidation::isValidGuid(withNul(L"{4D36E972-E325-11CE-BFC1-08002BE10318}", L"")));
}

void testIsMacAddress()
{
    VERIFY(NetworkValidation::isMacAddress(L"AABBCCDDEEFF"));
    VERIFY(NetworkValidation::isMacAddress(L"AA-BB-CC-DD-EE-FF"));
    VERIFY(!NetworkValidation::isMacAddress(L"not-a-mac"));
    // Keep the total length at the accepted 12 so the NUL is rejected as a non-hex-digit rather
    // than by a length mismatch: "AABBCCDDE" (9) + NUL + "FF" (2) = 12.
    VERIFY(!NetworkValidation::isMacAddress(withNul(L"AABBCCDDE", L"FF")));
}

} // namespace

int main()
{
    testIsValidIpAddress();
    testIsValidIpOrCidr();
    testIsValidHostname();
    testIsValidPortList();
    testIsValidGuid();
    testIsMacAddress();

    if (g_failures == 0) {
        printf("All NetworkValidation NUL-smuggling tests passed.\n");
    } else {
        printf("%d check(s) failed.\n", g_failures);
    }
    return g_failures;
}
