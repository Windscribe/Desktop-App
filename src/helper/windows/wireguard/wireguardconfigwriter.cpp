#include "wireguardconfigwriter.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <sstream>

#include "../../common/helper_commands.h"
#include "../network_validation.h"

namespace WireGuardConfigWriter {
namespace {

constexpr int kMinHeaderProtectionPadding = 12;
constexpr unsigned long kMaxVanillaPersistentKeepalive = 65535;

// 32 bytes as lowercase hex, the form amneziawgParamToHelperConfig() sends for HeaderProtectionKey.
bool isHexKey(const std::wstring &key)
{
    return key.size() == 64 && std::all_of(key.begin(), key.end(), [](wchar_t c) {
        return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f');
    });
}

// Encodes 32 raw bytes (64 hex chars, already validated) as the 44-char base64 wireguard-windows
// writes for a key. Input length is fixed, so the trailing remainder is always one '='.
std::wstring hexKeyToBase64(const std::wstring &hex)
{
    unsigned char raw[32];
    auto nibble = [](wchar_t c) { return static_cast<unsigned char>(c <= L'9' ? c - L'0' : c - L'a' + 10); };
    for (std::size_t i = 0; i < 32; ++i) {
        raw[i] = static_cast<unsigned char>((nibble(hex[i * 2]) << 4) | nibble(hex[i * 2 + 1]));
    }

    static const wchar_t kBase64[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::wstring encoded;
    encoded.reserve(44);
    for (std::size_t i = 0; i < 30; i += 3) {
        const unsigned int n = (raw[i] << 16) | (raw[i + 1] << 8) | raw[i + 2];
        encoded.push_back(kBase64[(n >> 18) & 63]);
        encoded.push_back(kBase64[(n >> 12) & 63]);
        encoded.push_back(kBase64[(n >> 6) & 63]);
        encoded.push_back(kBase64[n & 63]);
    }
    const unsigned int tail = (raw[30] << 16) | (raw[31] << 8);
    encoded.push_back(kBase64[(tail >> 18) & 63]);
    encoded.push_back(kBase64[(tail >> 12) & 63]);
    encoded.push_back(kBase64[(tail >> 6) & 63]);
    encoded.push_back(L'=');
    return encoded;
}

// WireGuard keys are 32 bytes, base64-encoded to 44 characters including the trailing '='.
bool isWireGuardKey(const std::wstring &key)
{
    if (key.size() != 44 || key.back() != L'=') {
        return false;
    }
    return std::all_of(key.begin(), key.end() - 1, [](wchar_t c) {
        return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') ||
               (c >= L'0' && c <= L'9') || c == L'+' || c == L'/';
    });
}

// "N" or "N-M" of up-to-10-digit decimals, with lo <= hi <= max.
bool isUintRange(const std::wstring &value, unsigned long long maxValue)
{
    const size_t dash = value.find(L'-');
    if (dash == 0 || value.find(L'-', dash == std::wstring::npos ? 0 : dash + 1) != std::wstring::npos) {
        return false;
    }
    const std::wstring loText = value.substr(0, dash);
    const std::wstring hiText = dash == std::wstring::npos ? loText : value.substr(dash + 1);
    auto parse = [](const std::wstring &text, unsigned long long &out) {
        if (text.empty() || text.size() > 10 ||
            text.find_first_not_of(L"0123456789") != std::wstring::npos) {
            return false;
        }
        try {
            out = std::stoull(text);
        } catch (const std::exception &) {
            return false;
        }
        return true;
    };
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    return parse(loText, lo) && parse(hiText, hi) && lo <= hi && hi <= maxValue;
}

// H1-H4 are a uint32 or a "x-y" pair of uint32s (amneziawg-go message paddings).
bool isHeaderValue(const std::wstring &value)
{
    return isUintRange(value, 0xFFFFFFFFull);
}

bool isIpCidrList(const std::wstring &list)
{
    if (list.empty()) {
        return false;
    }
    size_t start = 0;
    bool any = false;
    while (start <= list.size()) {
        const size_t comma = list.find(L',', start);
        const std::wstring token = list.substr(start, comma - start);
        const size_t first = token.find_first_not_of(L" \t");
        const size_t last = token.find_last_not_of(L" \t");
        if (first != std::wstring::npos) {
            if (!NetworkValidation::isValidIpOrCidr(token.substr(first, last - first + 1))) {
                return false;
            }
            any = true;
        }
        if (comma == std::wstring::npos) {
            break;
        }
        start = comma + 1;
    }
    return any;
}

bool isIpList(const std::wstring &list)
{
    if (list.empty()) {
        return true;
    }
    size_t start = 0;
    while (start <= list.size()) {
        const size_t comma = list.find(L',', start);
        const std::wstring token = list.substr(start, comma - start);
        const size_t first = token.find_first_not_of(L" \t");
        const size_t last = token.find_last_not_of(L" \t");
        if (first != std::wstring::npos &&
            !NetworkValidation::isValidIpAddress(token.substr(first, last - first + 1))) {
            return false;
        }
        if (comma == std::wstring::npos) {
            break;
        }
        start = comma + 1;
    }
    return true;
}

// host:port, host a literal IPv4 or IPv6 (bracketed). The engine resolves custom-config
// hostnames before this call, so a hostname here is a caller that skipped that step.
bool isPeerEndpoint(const std::wstring &endpoint)
{
    const size_t colon = endpoint.rfind(L':');
    if (colon == std::wstring::npos || colon == 0 || colon + 1 >= endpoint.size()) {
        return false;
    }
    const std::wstring portText = endpoint.substr(colon + 1);
    if (portText.find_first_not_of(L"0123456789") != std::wstring::npos) {
        return false;
    }
    unsigned long port = 0;
    try {
        port = std::stoul(portText);
    } catch (const std::exception &) {
        return false;
    }
    if (port < 1 || port > 65535) {
        return false;
    }

    std::wstring host = endpoint.substr(0, colon);
    if (!host.empty() && host.front() == L'[' && host.back() == L']') {
        host = host.substr(1, host.size() - 2);
    }
    return NetworkValidation::isValidIpAddress(host);
}

// "N" or "N-M" seconds. A range is an AmneziaWG extension; vanilla wireguard-nt takes a uint16.
bool isPersistentKeepalive(const std::wstring &value, bool isAmneziaWG)
{
    if (value.find(L'-') != std::wstring::npos && !isAmneziaWG) {
        return false;
    }
    const unsigned long long maxValue = isAmneziaWG ? 0xFFFFFFFFull : kMaxVanillaPersistentKeepalive;
    return isUintRange(value, maxValue);
}

std::wstring trim(const std::wstring &value)
{
    const size_t first = value.find_first_not_of(L" \t");
    if (first == std::wstring::npos) {
        return std::wstring();
    }
    const size_t last = value.find_last_not_of(L" \t");
    return value.substr(first, last - first + 1);
}

// wireguard-windows (tunnel.dll) activates its own WFP firewall when AllowedIPs contains
// any /0 entry. Replace catch-all /0 with equivalent /1 pairs so that firewall stays off.
std::wstring expandCatchAllRoutes(const std::wstring &allowedIps)
{
    std::wstring result;
    size_t start = 0;
    while (start <= allowedIps.size()) {
        const size_t comma = allowedIps.find(L',', start);
        const std::wstring token = trim(allowedIps.substr(start, comma - start));
        if (!token.empty()) {
            if (!result.empty()) {
                result += L", ";
            }
            if (token == L"0.0.0.0/0") {
                result += L"0.0.0.0/1, 128.0.0.0/1";
            } else if (token == L"::/0") {
                result += L"::/1, 8000::/1";
            } else {
                result += token;
            }
        }
        if (comma == std::wstring::npos) {
            break;
        }
        start = comma + 1;
    }
    return result;
}

bool validateAmneziawg(const AmneziawgConfig &config)
{
    for (const std::string &hValue : {config.h1, config.h2, config.h3, config.h4}) {
        const std::wstring value(hValue.begin(), hValue.end());
        if (!value.empty() && !isHeaderValue(value)) {
            spdlog::error("WireGuard: invalid AmneziaWG header parameter");
            return false;
        }
    }
    if (config.iValues.size() > 5) {
        spdlog::error("WireGuard: too many AmneziaWG init parameters");
        return false;
    }
    // Length is intentionally not capped. Server presets have no limit, and macOS/Linux write the
    // value as-is; the printable-ASCII check is what stops a smuggled directive.
    for (const std::string &iValue : config.iValues) {
        if (!std::all_of(iValue.begin(), iValue.end(), [](unsigned char c) { return c >= 0x20 && c <= 0x7E; })) {
            spdlog::error("WireGuard: invalid AmneziaWG init parameter");
            return false;
        }
    }

    // headerProtectionKey arrives as hex, the shared IPC form. The service config takes base64.
    const std::wstring headerKey(config.headerProtectionKey.begin(), config.headerProtectionKey.end());
    if (!headerKey.empty() && !isHexKey(headerKey)) {
        spdlog::error("WireGuard: invalid AmneziaWG HeaderProtectionKey");
        return false;
    }
    if (!headerKey.empty()) {
        for (const int padding : {config.s1, config.s2, config.s3, config.s4}) {
            if (padding < kMinHeaderProtectionPadding) {
                spdlog::error("WireGuard: AmneziaWG header-protection padding is shorter than the cipher nonce");
                return false;
            }
        }
    }

    const std::string *ranges[] = {
        &config.contentPaddingAddition, &config.rekeyAfterTime, &config.rekeyTimeout,
        &config.rejectAfterTime, &config.keepaliveTimeout, &config.maxHandshakeAttempts,
    };
    for (const std::string *range : ranges) {
        if (range->empty()) {
            continue;
        }
        if (!isUintRange(std::wstring(range->begin(), range->end()), 0xFFFFFFFFull)) {
            spdlog::error("WireGuard: invalid AmneziaWG range parameter");
            return false;
        }
    }
    for (const std::string *flag : {&config.randomTrailers, &config.disableCookies}) {
        if (!flag->empty() && *flag != "true" && *flag != "false") {
            spdlog::error("WireGuard: invalid AmneziaWG boolean parameter");
            return false;
        }
    }
    return true;
}

void appendLine(std::wostringstream &out, const wchar_t *key, const std::wstring &value)
{
    if (!value.empty()) {
        out << key << L" = " << value << L'\n';
    }
}

void appendLine(std::wostringstream &out, const wchar_t *key, const std::string &value)
{
    if (!value.empty()) {
        out << key << L" = " << std::wstring(value.begin(), value.end()) << L'\n';
    }
}

} // namespace

std::wstring generateConfigFile(const std::wstring &clientPrivateKey,
                                const std::wstring &clientIpAddress,
                                const std::wstring &clientDnsAddress,
                                const std::wstring &clientListenPort,
                                const std::wstring &peerPublicKey,
                                const std::wstring &peerPresharedKey,
                                const std::wstring &peerEndpoint,
                                const std::wstring &allowedIps,
                                const std::wstring &persistentKeepalive,
                                const AmneziawgConfig &amneziawgConfig)
{
    if (!isWireGuardKey(clientPrivateKey) || !isWireGuardKey(peerPublicKey) ||
        (!peerPresharedKey.empty() && !isWireGuardKey(peerPresharedKey))) {
        spdlog::error("WireGuard: rejecting key with invalid format");
        return std::wstring();
    }
    // Log the field name only. The rejected value is client-supplied, unbounded, and may contain
    // newlines that would forge lines in the SYSTEM service log.
    if (!isIpCidrList(clientIpAddress)) {
        spdlog::error("WireGuard: invalid client IP address");
        return std::wstring();
    }
    if (!isIpList(clientDnsAddress)) {
        spdlog::error("WireGuard: invalid DNS address list");
        return std::wstring();
    }
    if (!isIpCidrList(allowedIps)) {
        spdlog::error("WireGuard: invalid AllowedIPs");
        return std::wstring();
    }
    if (!isPeerEndpoint(peerEndpoint)) {
        spdlog::error("WireGuard: invalid peer endpoint");
        return std::wstring();
    }
    if (!clientListenPort.empty() && !isUintRange(clientListenPort, 65535)) {
        spdlog::error("WireGuard: invalid ListenPort");
        return std::wstring();
    }
    if (clientListenPort.find(L'-') != std::wstring::npos) {
        spdlog::error("WireGuard: ListenPort must be a single port");
        return std::wstring();
    }

    const bool isAmneziaWG = amneziawgConfig.isValid();
    const std::wstring keepalive = persistentKeepalive.empty() ? L"25" : persistentKeepalive;
    if (!isPersistentKeepalive(keepalive, isAmneziaWG)) {
        spdlog::error("WireGuard: invalid PersistentKeepalive");
        return std::wstring();
    }
    if (isAmneziaWG && !validateAmneziawg(amneziawgConfig)) {
        return std::wstring();
    }

    // QSettings double-quotes base64, which wireguard-windows cannot parse, so the file is
    // written by hand. Every interpolated value has already been restricted to a single line.
    std::wostringstream out;
    out << L"[Interface]\n";
    appendLine(out, L"PrivateKey", clientPrivateKey);
    appendLine(out, L"Address", clientIpAddress);
    appendLine(out, L"DNS", clientDnsAddress);
    appendLine(out, L"ListenPort", clientListenPort);

    if (isAmneziaWG) {
        if (amneziawgConfig.jc > 0) {
            out << L"Jc = " << amneziawgConfig.jc << L'\n';
        }
        if (amneziawgConfig.jmin > 0) {
            out << L"Jmin = " << amneziawgConfig.jmin << L'\n';
        }
        if (amneziawgConfig.jmax > 0) {
            out << L"Jmax = " << amneziawgConfig.jmax << L'\n';
        }
        if (amneziawgConfig.s1 != 0) {
            out << L"S1 = " << amneziawgConfig.s1 << L'\n';
        }
        if (amneziawgConfig.s2 != 0) {
            out << L"S2 = " << amneziawgConfig.s2 << L'\n';
        }
        if (amneziawgConfig.s3 != 0) {
            out << L"S3 = " << amneziawgConfig.s3 << L'\n';
        }
        if (amneziawgConfig.s4 != 0) {
            out << L"S4 = " << amneziawgConfig.s4 << L'\n';
        }
        appendLine(out, L"H1", amneziawgConfig.h1);
        appendLine(out, L"H2", amneziawgConfig.h2);
        appendLine(out, L"H3", amneziawgConfig.h3);
        appendLine(out, L"H4", amneziawgConfig.h4);
        for (std::size_t i = 0; i < amneziawgConfig.iValues.size() && i < 5; ++i) {
            if (amneziawgConfig.iValues[i].empty()) {
                continue;
            }
            out << L"I" << (i + 1) << L" = " << std::wstring(amneziawgConfig.iValues[i].begin(),
                                                             amneziawgConfig.iValues[i].end()) << L'\n';
        }
        // The IPC form is hex (shared with the POSIX UAPI). wireguard-windows wants base64.
        if (!amneziawgConfig.headerProtectionKey.empty()) {
            const std::wstring headerKey(amneziawgConfig.headerProtectionKey.begin(),
                                         amneziawgConfig.headerProtectionKey.end());
            appendLine(out, L"HeaderProtectionKey", hexKeyToBase64(headerKey));
        }
        appendLine(out, L"ContentPaddingAddition", amneziawgConfig.contentPaddingAddition);
        appendLine(out, L"RekeyAfterTime", amneziawgConfig.rekeyAfterTime);
        appendLine(out, L"RekeyTimeout", amneziawgConfig.rekeyTimeout);
        appendLine(out, L"RejectAfterTime", amneziawgConfig.rejectAfterTime);
        appendLine(out, L"KeepaliveTimeout", amneziawgConfig.keepaliveTimeout);
        appendLine(out, L"MaxHandshakeAttempts", amneziawgConfig.maxHandshakeAttempts);
        appendLine(out, L"RandomTrailers", amneziawgConfig.randomTrailers);
        appendLine(out, L"DisableCookies", amneziawgConfig.disableCookies);
    }

    out << L'\n';
    out << L"[Peer]\n";
    appendLine(out, L"PublicKey", peerPublicKey);
    appendLine(out, L"Endpoint", peerEndpoint);
    appendLine(out, L"PresharedKey", peerPresharedKey);
    out << L"AllowedIPs = " << expandCatchAllRoutes(allowedIps) << L'\n';
    out << L"PersistentKeepalive = " << keepalive << L'\n';
    return out.str();
}

} // namespace WireGuardConfigWriter
