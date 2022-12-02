#include "../all_headers.h"
#include "ip4_address_and_mask.h"
#include <Mstcpip.h>
#include <codecvt>

Ip4AddressAndMask::Ip4AddressAndMask(PCWSTR address) : ipAddress_(0), mask_(0)
{
    // convert to std::string
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::string converted_str = converter.to_bytes(address);

    constructFromString(converted_str);
}

Ip4AddressAndMask::Ip4AddressAndMask(PCSTR address) : ipAddress_(0), mask_(0)
{
    constructFromString(address);
}

bool Ip4AddressAndMask::isValid() const
{
    return ipAddress_ != 0;
}

UINT32 Ip4AddressAndMask::ipNetworkOrder()  const
{
    return htonl(ipAddress_);
}

UINT32 Ip4AddressAndMask::maskNetworkOrder() const
{
    return htonl(mask_);
}

UINT32 Ip4AddressAndMask::ipHostOrder() const
{
    assert(isValid());
    return ipAddress_;
}

UINT32 Ip4AddressAndMask::maskHostOrder() const
{
    assert(isValid());
    return mask_;
}

std::wstring Ip4AddressAndMask::asString() const
{
    IN_ADDR v4Addr = { 0 };
    v4Addr.S_un.S_addr = ipAddress_;
    wchar_t str[128];
    ULONG len = 128;
    RtlIpv4AddressToStringExW(&v4Addr, 0, str, &len);
    return str;
}

void Ip4AddressAndMask::constructFromString(const std::string &str)
{
    // check whether there is a mask
    std::string mask;
    std::string address;
    std::string::size_type const p(str.find('/'));
    if (p != std::string::npos)
    {
        address = str.substr(0, p);
        mask = str.substr(p + 1);
    }
    else
    {
        address = str;
    }

    if (!mask.empty())
    {
        // convert mask string to unsigned long
        unsigned long maskNum;
        try {
            maskNum = std::stoul(mask);
        }
        catch (...)
        {
            return;
        }

        if (maskNum > 32)
        {
            return;
        }
        mask_ = maskFromInteger(maskNum);
    }
    else
    {
        mask_ = 0xFFFFFFFF;
    }

    IN_ADDR v4Addr = { 0 };
    ipAddress_ = 0;
    USHORT port;    // port unused
    if (RtlIpv4StringToAddressExA(address.c_str(), FALSE, &v4Addr, &port) == 0)        // success
    {
        CopyMemory(&ipAddress_, &v4Addr, 4);
        ipAddress_ = ntohl(ipAddress_);  // convert to host order
    }
    else
    {
        ipAddress_ = 0;
        return;
    }
}

UINT32 Ip4AddressAndMask::maskFromInteger(UINT32 num)
{
    UINT32 ret = 0;
    for (UINT32 i = 1; i <= num; ++i)
    {
        ret |= 1UL << (32 - i);
    }
    return ret;
}

std::vector<Ip4AddressAndMask> Ip4AddressAndMask::fromVector(const std::vector<PCWSTR> &ips)
{
    std::vector<Ip4AddressAndMask> ret;

    for (PCWSTR ip : ips) {
        ret.push_back(Ip4AddressAndMask(ip));
    }
    return ret;
}