// Pins the helper-authored WireGuard config: the service .conf is built from validated fields,
// catch-all routes are expanded, and a smuggled directive never becomes a line in the file.

#include <cstdio>
#include <string>

#include "../../common/helper_commands.h"
#include "../wireguard/wireguardconfigwriter.h"

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

const std::wstring kPrivateKey = L"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
const std::wstring kPublicKey = L"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB=";

std::wstring generate(const std::wstring &allowedIps = L"0.0.0.0/0",
                      const std::wstring &keepalive = L"25",
                      const AmneziawgConfig &amneziawg = AmneziawgConfig(),
                      const std::wstring &endpoint = L"203.0.113.5:51820",
                      const std::wstring &address = L"10.0.0.2/32",
                      const std::wstring &dns = L"10.255.255.1")
{
    return WireGuardConfigWriter::generateConfigFile(
        kPrivateKey, address, dns, L"", kPublicKey, L"", endpoint, allowedIps, keepalive, amneziawg);
}

void testVanillaConfig()
{
    const std::wstring file = generate();
    VERIFY(file.find(L"PrivateKey = " + kPrivateKey) != std::wstring::npos);
    VERIFY(file.find(L"Address = 10.0.0.2/32\n") != std::wstring::npos);
    VERIFY(file.find(L"DNS = 10.255.255.1\n") != std::wstring::npos);
    VERIFY(file.find(L"Endpoint = 203.0.113.5:51820\n") != std::wstring::npos);
    VERIFY(file.find(L"AllowedIPs = 0.0.0.0/1, 128.0.0.0/1\n") != std::wstring::npos);
    VERIFY(file.find(L"PersistentKeepalive = 25\n") != std::wstring::npos);
    VERIFY(file.find(L"ListenPort") == std::wstring::npos);
    VERIFY(file.find(L"PresharedKey") == std::wstring::npos);
}

void testCatchAllExpansion()
{
    const std::wstring file = generate(L"0.0.0.0/0, ::/0, 192.168.1.0/24");
    VERIFY(file.find(L"AllowedIPs = 0.0.0.0/1, 128.0.0.0/1, ::/1, 8000::/1, 192.168.1.0/24\n") != std::wstring::npos);
}

void testEmptyKeepaliveDefaults()
{
    const std::wstring file = generate(L"10.0.0.0/8", L"");
    VERIFY(file.find(L"PersistentKeepalive = 25\n") != std::wstring::npos);
}

void testRejectsInjectedDirective()
{
    VERIFY(generate(L"10.0.0.0/8\nPostUp = calc.exe").empty());
    VERIFY(generate(L"10.0.0.0/8", L"25\nTable = off").empty());
    VERIFY(generate(L"10.0.0.0/8", L"25", AmneziawgConfig(), L"203.0.113.5:51820\nDNS = 1.2.3.4").empty());
    // A NUL after a valid prefix must not validate on the prefix alone.
    std::wstring address = L"10.0.0.2/32";
    address.push_back(L'\0');
    address += L"\nPostUp = calc.exe";
    VERIFY(generate(L"10.0.0.0/8", L"25", AmneziawgConfig(), L"203.0.113.5:51820", address).empty());
}

void testRejectsBadKeysAndEndpoint()
{
    VERIFY(WireGuardConfigWriter::generateConfigFile(
        L"not-a-key", L"10.0.0.2/32", L"10.255.255.1", L"", kPublicKey, L"",
        L"203.0.113.5:51820", L"0.0.0.0/0", L"25", AmneziawgConfig()).empty());
    VERIFY(generate(L"10.0.0.0/8", L"25", AmneziawgConfig(), L"vpn.example.com:51820").empty());
    VERIFY(generate(L"10.0.0.0/8", L"25", AmneziawgConfig(), L"203.0.113.5:0").empty());
    VERIFY(generate(L"not-a-cidr").empty());
}

void testAmneziawgFields()
{
    AmneziawgConfig config;
    config.jc = 4;
    config.h1 = "12345-67890";
    config.iValues.push_back("<b 0x01020304>");
    config.headerProtectionKey = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    config.s1 = config.s2 = config.s3 = config.s4 = 12;
    config.contentPaddingAddition = "0-16";
    config.randomTrailers = "true";

    const std::wstring file = generate(L"0.0.0.0/0", L"10-20", config);
    VERIFY(file.find(L"Jc = 4\n") != std::wstring::npos);
    VERIFY(file.find(L"H1 = 12345-67890\n") != std::wstring::npos);
    VERIFY(file.find(L"I1 = <b 0x01020304>\n") != std::wstring::npos);
    VERIFY(file.find(L"HeaderProtectionKey = AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=\n") != std::wstring::npos);
    VERIFY(file.find(L"ContentPaddingAddition = 0-16\n") != std::wstring::npos);
    VERIFY(file.find(L"RandomTrailers = true\n") != std::wstring::npos);
    VERIFY(file.find(L"PersistentKeepalive = 10-20\n") != std::wstring::npos);
    // An unset v3 key must not appear. Emitting an empty "HeaderProtectionKey =" would change the handshake.
    VERIFY(file.find(L"RekeyTimeout") == std::wstring::npos);
    VERIFY(file.find(L"DisableCookies") == std::wstring::npos);
}

void testAmneziawgV3FieldsEmitted()
{
    // Pins every v3 key name; a misspelled key would be silently ignored by the tunnel service.
    AmneziawgConfig config;
    config.headerProtectionKey = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    config.s1 = config.s2 = config.s3 = config.s4 = 12;
    config.contentPaddingAddition = "0-16";
    config.rekeyAfterTime = "100-140";
    config.rekeyTimeout = "5";
    config.rejectAfterTime = "180-200";
    config.keepaliveTimeout = "10-12";
    config.maxHandshakeAttempts = "20";
    config.randomTrailers = "true";
    config.disableCookies = "false";

    const std::wstring file = generate(L"0.0.0.0/0", L"25", config);
    VERIFY(file.find(L"HeaderProtectionKey = AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=\n") != std::wstring::npos);
    VERIFY(file.find(L"ContentPaddingAddition = 0-16\n") != std::wstring::npos);
    VERIFY(file.find(L"RekeyAfterTime = 100-140\n") != std::wstring::npos);
    VERIFY(file.find(L"RekeyTimeout = 5\n") != std::wstring::npos);
    VERIFY(file.find(L"RejectAfterTime = 180-200\n") != std::wstring::npos);
    VERIFY(file.find(L"KeepaliveTimeout = 10-12\n") != std::wstring::npos);
    VERIFY(file.find(L"MaxHandshakeAttempts = 20\n") != std::wstring::npos);
    VERIFY(file.find(L"RandomTrailers = true\n") != std::wstring::npos);
    VERIFY(file.find(L"DisableCookies = false\n") != std::wstring::npos);
}

void testLongInitParameterAccepted()
{
    // Server presets have no length limit, so the helper must not impose the custom-config import cap.
    AmneziawgConfig config;
    config.iValues.push_back(std::string(5000, 'A'));
    const std::wstring file = generate(L"0.0.0.0/0", L"25", config);
    VERIFY(file.find(L"I1 = " + std::wstring(5000, L'A')) != std::wstring::npos);
}

void testRejectsAmneziawgRangeOnVanilla()
{
    VERIFY(generate(L"0.0.0.0/0", L"10-20").empty());
}

void testRejectsShortHeaderProtectionPadding()
{
    AmneziawgConfig config;
    config.headerProtectionKey = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    config.s1 = 11;
    config.s2 = config.s3 = config.s4 = 12;
    VERIFY(generate(L"0.0.0.0/0", L"25", config).empty());
}

} // namespace

int main()
{
    testVanillaConfig();
    testCatchAllExpansion();
    testEmptyKeepaliveDefaults();
    testRejectsInjectedDirective();
    testRejectsBadKeysAndEndpoint();
    testAmneziawgFields();
    testAmneziawgV3FieldsEmitted();
    testLongInitParameterAccepted();
    testRejectsAmneziawgRangeOnVanilla();
    testRejectsShortHeaderProtectionPadding();

    if (g_failures != 0) {
        printf("%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("wireguardconfigwriter.test passed\n");
    return 0;
}
