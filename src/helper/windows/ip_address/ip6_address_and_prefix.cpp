#include <winsock2.h>
#include <ws2ipdef.h>
#include <Mstcpip.h>
#include <codecvt>

#include "ip6_address_and_prefix.h"

Ip6AddressAndPrefix::Ip6AddressAndPrefix(PCWSTR address) : ipAddress_{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, prefixLength_(0)
{
    // convert to std::string
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::string converted_str = converter.to_bytes(address);

    constructFromString(converted_str);
}

Ip6AddressAndPrefix::Ip6AddressAndPrefix(PCSTR address) : ipAddress_{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, prefixLength_(0)
{
    constructFromString(address);
}

bool Ip6AddressAndPrefix::isValid() const
{
    for (int i = 0; i < 16; i++) {
        if (ipAddress_[i] != 0) {
            return true;
        }
    }
    return false;
}

UINT8 *Ip6AddressAndPrefix::ip() const
{
    return (UINT8 *)&ipAddress_;
}

UINT8 Ip6AddressAndPrefix::prefixLength() const
{
    return prefixLength_;
}

void Ip6AddressAndPrefix::constructFromString(const std::string &str)
{
    std::string prefix;
    std::string address;
    std::string::size_type const p(str.find('/'));

    if (p != std::string::npos) {
        address = str.substr(0, p);
        prefix = str.substr(p + 1);
    } else {
        address = str;
    }

    if (!prefix.empty()) {
        try {
            prefixLength_ = (UINT8)std::stoul(prefix);
        } catch (...) {
            prefixLength_ = 128;
        }
    } else {
        prefixLength_ = 128;
    }

    IN6_ADDR v6Addr = { 0 };
    ULONG scopeId; // unused
    USHORT port; // unused
    if (RtlIpv6StringToAddressExA(address.c_str(), &v6Addr, &scopeId, &port) == 0) {
        CopyMemory(&ipAddress_, &v6Addr, 16);
    }
}

std::vector<Ip6AddressAndPrefix> Ip6AddressAndPrefix::fromVector(const std::vector<PCWSTR> &ips)
{
    std::vector<Ip6AddressAndPrefix> ret;

    for (PCWSTR ip : ips) {
        ret.push_back(Ip6AddressAndPrefix(ip));
    }
    return ret;
}
