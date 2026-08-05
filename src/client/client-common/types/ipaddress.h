#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace types {

class IpAddress
{
public:
    enum Family { IPv4, IPv6 };

    IpAddress();
    explicit IpAddress(const std::string &addr);
    IpAddress(Family family, const uint8_t *data, std::size_t size);
#ifdef _WIN32
    // Convenience for Windows-only callers (helper IPC, WinAPI). IP literals are ASCII,
    // so we narrow each wchar_t with a static_cast<char> — no UTF conversion needed.
    explicit IpAddress(const std::wstring &addr);
    explicit IpAddress(const wchar_t *addr);
#endif

    bool isValid() const;
    bool isV4() const;
    bool isV6() const;
    Family family() const;

    // 127/8, 0.0.0.0, ::1, :: — and the same embedded in an IPv4-mapped literal, which toString()
    // renders as "::ffff:127.0.0.1" and no string test on either family would catch. False when invalid.
    bool isLoopbackOrUnspecified() const;

    // Raw address bytes in network byte order (big-endian).
    // IPv4: 4 meaningful bytes, IPv6: 16 bytes.
    const uint8_t *bytes() const;
    std::size_t bytesSize() const;

    // IPv4-only convenience accessors.
    uint32_t ipv4NetworkOrder() const;
    uint32_t ipv4HostOrder() const;

    std::string toString() const;

    bool operator==(const IpAddress &other) const;
    bool operator!=(const IpAddress &other) const;
    bool operator<(const IpAddress &other) const;

    static Family detectFamily(const std::string &str);

private:
    Family family_ = IPv4;
    std::array<uint8_t, 16> bytes_ = {};
    bool valid_ = false;
};


class IpAddressRange
{
public:
    IpAddressRange();
    explicit IpAddressRange(const std::string &cidr);
    IpAddressRange(const IpAddress &addr, uint8_t prefixLength);
#ifdef _WIN32
    // Same rationale as IpAddress(const std::wstring &) — Windows-only ASCII narrow.
    explicit IpAddressRange(const std::wstring &cidr);
    explicit IpAddressRange(const wchar_t *cidr);
#endif

    bool isValid() const;
    bool isV4() const;
    bool isV6() const;
    IpAddress::Family family() const;

    const IpAddress &address() const;
    uint8_t prefixLength() const;

    // IPv4-only: prefix length as a bitmask (e.g. /24 -> 0xFFFFFF00).
    uint32_t ipv4MaskNetworkOrder() const;
    uint32_t ipv4MaskHostOrder() const;

    // Returns CIDR string. Single-host addresses include the prefix (/32 or /128).
    std::string toString() const;

    bool operator==(const IpAddressRange &other) const;
    bool operator!=(const IpAddressRange &other) const;
    bool operator<(const IpAddressRange &other) const;

    static std::vector<IpAddressRange> fromStrings(const std::vector<std::string> &strs);
#ifdef _WIN32
    static std::vector<IpAddressRange> fromStrings(const std::vector<std::wstring> &strs);
#endif

    struct SplitResult {
        std::vector<IpAddressRange> v4;
        std::vector<IpAddressRange> v6;
    };
    static SplitResult splitByFamily(const std::vector<IpAddressRange> &ranges);

private:
    IpAddress address_;
    uint8_t prefixLen_ = 0;

    static uint32_t prefixToMask(uint8_t prefix);
};

// The two halves of the "address[:port]" packed form, kept together so only this file knows the encoding.
// Bracket-aware by necessity: a bare IPv6 literal already contains colons, so a port is unambiguous only
// when the address is bracketed — which is also what systemd-resolved emits.

// Port 0 yields the bare address; a port on IPv6 yields "[addr]:port".
std::string joinEndpoint(const std::string &address, uint16_t port);

// Sets port to 0 when the entry carries none. False if a port is present but not a decimal 1-65535, or
// the brackets are unterminated — either means the entry did not come from joinEndpoint.
bool splitEndpoint(const std::string &entry, std::string &address, uint16_t &port);

} // namespace types
