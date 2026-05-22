#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cwchar>
#else
#include <arpa/inet.h>
#endif

#include "ipaddress.h"

namespace types {

#ifdef _WIN32
namespace {
// IP literals (dotted-decimal v4, hex v6, optional /prefix) are pure ASCII. We just narrow
// each wchar_t to char with static_cast — explicit cast silences MSVC C4244 under /WX.
std::string narrowAscii(const wchar_t *src, std::size_t len)
{
    std::string out;
    if (!src) {
        return out;
    }
    out.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(static_cast<char>(src[i]));
    }
    return out;
}
} // namespace
#endif

// ============================================================================
//  IpAddress
// ============================================================================

IpAddress::IpAddress() = default;

IpAddress::IpAddress(const std::string &addr) : family_(IPv4), bytes_{}, valid_(false)
{
    if (addr.empty())
        return;

    family_ = detectFamily(addr);

    int af = (family_ == IPv6) ? AF_INET6 : AF_INET;
    if (inet_pton(af, addr.c_str(), bytes_.data()) == 1) {
        valid_ = true;
    } else {
        bytes_ = {};
    }
}

IpAddress::IpAddress(Family family, const uint8_t *data, std::size_t size)
    : family_(family), bytes_{}, valid_(false)
{
    std::size_t expected = (family == IPv6) ? 16 : 4;
    if (data && size >= expected) {
        std::memcpy(bytes_.data(), data, expected);
        valid_ = true;
    }
}

#ifdef _WIN32
IpAddress::IpAddress(const std::wstring &addr) : IpAddress(narrowAscii(addr.data(), addr.size())) {}
IpAddress::IpAddress(const wchar_t *addr) : IpAddress(narrowAscii(addr, addr ? std::wcslen(addr) : 0)) {}
#endif

bool IpAddress::isValid() const { return valid_; }
bool IpAddress::isV4() const { return valid_ && family_ == IPv4; }
bool IpAddress::isV6() const { return valid_ && family_ == IPv6; }
IpAddress::Family IpAddress::family() const { return family_; }

const uint8_t *IpAddress::bytes() const { return bytes_.data(); }

std::size_t IpAddress::bytesSize() const
{
    return (family_ == IPv6) ? 16 : 4;
}

uint32_t IpAddress::ipv4NetworkOrder() const
{
    if (!isV4())
        return 0;
    uint32_t result = 0;
    std::memcpy(&result, bytes_.data(), 4);
    return result;
}

uint32_t IpAddress::ipv4HostOrder() const
{
    return ntohl(ipv4NetworkOrder());
}

std::string IpAddress::toString() const
{
    if (!valid_)
        return {};

    char buf[INET6_ADDRSTRLEN] = {};
    int af = (family_ == IPv6) ? AF_INET6 : AF_INET;
    if (inet_ntop(af, bytes_.data(), buf, sizeof(buf)) != nullptr)
        return buf;
    return {};
}

bool IpAddress::operator==(const IpAddress &other) const
{
    if (valid_ != other.valid_)
        return false;
    if (!valid_)
        return true;
    if (family_ != other.family_)
        return false;
    std::size_t len = (family_ == IPv6) ? 16 : 4;
    return std::memcmp(bytes_.data(), other.bytes_.data(), len) == 0;
}

bool IpAddress::operator!=(const IpAddress &other) const
{
    return !(*this == other);
}

bool IpAddress::operator<(const IpAddress &other) const
{
    if (family_ != other.family_)
        return family_ < other.family_;
    std::size_t len = (family_ == IPv6) ? 16 : 4;
    return std::memcmp(bytes_.data(), other.bytes_.data(), len) < 0;
}

IpAddress::Family IpAddress::detectFamily(const std::string &str)
{
    return (str.find(':') != std::string::npos) ? IPv6 : IPv4;
}


// ============================================================================
//  IpAddressRange
// ============================================================================

IpAddressRange::IpAddressRange() = default;

IpAddressRange::IpAddressRange(const std::string &cidr) : prefixLen_(0)
{
    if (cidr.empty())
        return;

    std::string addrPart;
    auto slashPos = cidr.find('/');
    if (slashPos != std::string::npos) {
        addrPart = cidr.substr(0, slashPos);
        std::string prefixStr = cidr.substr(slashPos + 1);
        // Strict numeric parse: reject empty prefix, signs, non-digit chars and trailing
        // garbage. Without this, `std::stoul` would accept "32abc"/"+10", and values > 255
        // would silently truncate via the `static_cast<uint8_t>` below (e.g. "/256" -> /0,
        // bypassing the maxPrefix check and producing a spurious default-route entry).
        if (prefixStr.empty())
            return;
        for (char c : prefixStr) {
            if (c < '0' || c > '9')
                return;
        }
        try {
            std::size_t consumed = 0;
            unsigned long val = std::stoul(prefixStr, &consumed);
            if (consumed != prefixStr.size() || val > 128)
                return;
            prefixLen_ = static_cast<uint8_t>(val);
        } catch (...) {
            return;
        }
    } else {
        addrPart = cidr;
    }

    address_ = IpAddress(addrPart);
    if (!address_.isValid()) {
        prefixLen_ = 0;
        return;
    }

    uint8_t maxPrefix = address_.isV4() ? 32 : 128;
    if (slashPos == std::string::npos) {
        prefixLen_ = maxPrefix;
    } else if (prefixLen_ > maxPrefix) {
        address_ = IpAddress();
        prefixLen_ = 0;
    }
}

IpAddressRange::IpAddressRange(const IpAddress &addr, uint8_t prefixLength)
    : address_(addr), prefixLen_(prefixLength)
{
    if (addr.isValid()) {
        uint8_t maxPrefix = addr.isV6() ? 128 : 32;
        if (prefixLen_ > maxPrefix)
            prefixLen_ = maxPrefix;
    }
}

#ifdef _WIN32
IpAddressRange::IpAddressRange(const std::wstring &cidr) : IpAddressRange(narrowAscii(cidr.data(), cidr.size())) {}
IpAddressRange::IpAddressRange(const wchar_t *cidr) : IpAddressRange(narrowAscii(cidr, cidr ? std::wcslen(cidr) : 0)) {}
#endif

bool IpAddressRange::isValid() const { return address_.isValid(); }
bool IpAddressRange::isV4() const { return address_.isV4(); }
bool IpAddressRange::isV6() const { return address_.isV6(); }
IpAddress::Family IpAddressRange::family() const { return address_.family(); }
const IpAddress &IpAddressRange::address() const { return address_; }
uint8_t IpAddressRange::prefixLength() const { return prefixLen_; }

uint32_t IpAddressRange::ipv4MaskNetworkOrder() const
{
    if (!isV4())
        return 0;
    return htonl(prefixToMask(prefixLen_));
}

uint32_t IpAddressRange::ipv4MaskHostOrder() const
{
    if (!isV4())
        return 0;
    return prefixToMask(prefixLen_);
}

std::string IpAddressRange::toString() const
{
    if (!isValid())
        return {};
    return address_.toString() + "/" + std::to_string(prefixLen_);
}

bool IpAddressRange::operator==(const IpAddressRange &other) const
{
    return address_ == other.address_ && prefixLen_ == other.prefixLen_;
}

bool IpAddressRange::operator!=(const IpAddressRange &other) const
{
    return !(*this == other);
}

bool IpAddressRange::operator<(const IpAddressRange &other) const
{
    if (address_ != other.address_)
        return address_ < other.address_;
    return prefixLen_ < other.prefixLen_;
}

std::vector<IpAddressRange> IpAddressRange::fromStrings(const std::vector<std::string> &strs)
{
    std::vector<IpAddressRange> result;
    result.reserve(strs.size());
    for (const auto &s : strs)
        result.emplace_back(s);
    return result;
}

#ifdef _WIN32
std::vector<IpAddressRange> IpAddressRange::fromStrings(const std::vector<std::wstring> &strs)
{
    std::vector<IpAddressRange> result;
    result.reserve(strs.size());
    for (const auto &s : strs)
        result.emplace_back(s);
    return result;
}
#endif

IpAddressRange::SplitResult IpAddressRange::splitByFamily(const std::vector<IpAddressRange> &ranges)
{
    SplitResult result;
    for (const auto &r : ranges) {
        if (!r.isValid())
            continue;
        if (r.isV4())
            result.v4.push_back(r);
        else
            result.v6.push_back(r);
    }
    return result;
}

uint32_t IpAddressRange::prefixToMask(uint8_t prefix)
{
    if (prefix == 0)
        return 0;
    if (prefix >= 32)
        return 0xFFFFFFFF;
    return ~((1u << (32 - prefix)) - 1);
}

} // namespace types
