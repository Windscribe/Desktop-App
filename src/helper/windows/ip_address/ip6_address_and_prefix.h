#pragma once

#include <string>
#include <vector>

// specifies IPv6 prefix and prefix length
class Ip6AddressAndPrefix
{
public:
    explicit Ip6AddressAndPrefix(PCWSTR address);
    explicit Ip6AddressAndPrefix(PCSTR address);

    bool isValid() const;

    UINT8 *ip() const;
    UINT8 prefixLength() const;

    bool operator==(const Ip6AddressAndPrefix &other) const {
        return (memcmp(ipAddress_, other.ipAddress_, 16) == 0) && prefixLength_ == other.prefixLength_;
    }

    static std::vector<Ip6AddressAndPrefix> fromVector(const std::vector<PCWSTR> &ips);

private:
    UINT8 ipAddress_[16];
    UINT8 prefixLength_;

    void constructFromString(const std::string &str);
};
