#include "network_validation.h"

#include <ws2tcpip.h>
#include <windns.h>
#include <Mstcpip.h>
#include <netiodef.h>
#include <objbase.h>

#include <algorithm>
#include <cwctype>

namespace NetworkValidation
{

bool isValidIpAddress(const std::wstring &ip)
{
    IN_ADDR  addr4;
    IN6_ADDR addr6;
    return InetPtonW(AF_INET,  ip.c_str(), &addr4) == 1
        || InetPtonW(AF_INET6, ip.c_str(), &addr6) == 1;
}

bool isValidIpOrCidr(const std::wstring &value)
{
    const size_t slash = value.find(L'/');
    const std::wstring ipPart = (slash == std::wstring::npos) ? value : value.substr(0, slash);
    if (ipPart.empty()) {
        return false;
    }

    unsigned long maxPrefix;
    IN_ADDR  addr4;
    IN6_ADDR addr6;
    if (InetPtonW(AF_INET, ipPart.c_str(), &addr4) == 1) {
        maxPrefix = 32;
    } else if (InetPtonW(AF_INET6, ipPart.c_str(), &addr6) == 1) {
        maxPrefix = 128;
    } else {
        return false;
    }

    if (slash == std::wstring::npos) {
        return true;
    }

    const std::wstring prefixPart = value.substr(slash + 1);
    if (prefixPart.empty() || prefixPart.find_first_not_of(L"0123456789") != std::wstring::npos) {
        return false;
    }
    try {
        return std::stoul(prefixPart) <= maxPrefix;
    } catch (const std::exception &) {
        return false;
    }
}

bool isValidHostname(const std::wstring &hostname)
{
    return DnsValidateName_W(hostname.c_str(), DnsNameHostnameFull) == ERROR_SUCCESS;
}

bool isValidPortList(const std::wstring &ports)
{
    if (ports.empty()) {
        return false;
    }

    auto parsePort = [](const std::wstring &s, unsigned long &port) {
        if (s.empty() || s.find_first_not_of(L"0123456789") != std::wstring::npos) {
            return false;
        }
        try {
            port = std::stoul(s);
        } catch (const std::exception &) {
            return false;
        }
        return port >= 1 && port <= 65535;
    };

    size_t start = 0;
    while (true) {
        const size_t comma = ports.find(L',', start);
        const std::wstring token = ports.substr(start, comma - start);

        const size_t dash = token.find(L'-');
        unsigned long lo, hi;
        if (dash == std::wstring::npos) {
            if (!parsePort(token, lo)) return false;
        } else {
            if (!parsePort(token.substr(0, dash), lo)) return false;
            if (!parsePort(token.substr(dash + 1), hi)) return false;
            if (lo > hi) return false;
        }

        if (comma == std::wstring::npos) break;
        start = comma + 1;
    }

    return true;
}

bool isValidGuid(const std::wstring &guid)
{
    GUID parsed;
    return ::IIDFromString(guid.c_str(), &parsed) == S_OK;
}

bool isMacAddress(const std::wstring &value)
{
    bool valid = false;

    if (value.find_first_of(L":-") == std::wstring::npos) {
        if (value.size() == 12) {
            const auto result = std::count_if(value.begin(), value.end(), [](wint_t c){ return std::iswxdigit(c); });
            valid = (result == 12);
        }
    }
    else {
        // We expect the MAC address in AA-BB-CC-DD-EE-FF format, with the separator being a '-' or ':'.
        if (value.size() == 17) {
            // Convert to the non-DIX standard "-" notation required by the IP Helper API.
            std::wstring mac(value);
            std::replace(mac.begin(), mac.end(), L':', L'-');

            // Let the Windows API do the validation for us.
            PCWSTR terminator = NULL;
            DL_EUI48 addr;
            auto status = ::RtlEthernetStringToAddressW(mac.c_str(), &terminator, &addr);
            valid = (status == ERROR_SUCCESS);
        }
    }

    return valid;
}

}
