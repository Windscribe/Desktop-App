#include "dnsservers.h"
#include <assert.h>
#include <sstream>

namespace wsnet {

DnsServers::DnsServers()
{
    servers_ = "";
}

DnsServers::DnsServers(const std::vector<std::string> &ips)
{
    std::string result;

    for (const std::string &ip : ips) {
        // Currently only support raw addresses set this way, and not the full address:port%interface syntax supported by ares
        if (!parseAddress(ip)) {
            continue;
        }
 
        if (!result.empty()) {
            result += ",";
        }
      
        result += ip;
    }

    servers_ = result;
}

DnsServers::DnsServers(const char *csv)
{
    servers_ = csv;
}

DnsServers::DnsServers(const DnsServers &other)
{
    servers_ = other.get();
}

DnsServers::~DnsServers()
{
}

DnsServers &DnsServers::operator=(const DnsServers &other)
{
    servers_ = other.get();
    return *this;
}

bool DnsServers::operator==(const DnsServers &other) const
{
    return servers_ == other.get();
}

bool DnsServers::operator!=(const DnsServers &other) const
{
    return !operator==(other);
}

std::string DnsServers::get() const
{
    return servers_;
}

bool DnsServers::parseAddress(const std::string &addr)
{
    std::string ip = addr;

    if (addr.empty()) {
        return false;
    }

    sockaddr_in sa;
    if (ares_inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) > 0) {
        return true;
    }

    // IPv6 addresses may be enclosed in square brackets
    if (ip[0] == '[' && ip[ip.size() - 1] == ']') {
        ip = ip.substr(1, ip.size() - 2);
    }

    sockaddr_in6 sa6;
    if (ares_inet_pton(AF_INET6, ip.c_str(), &(sa6.sin6_addr)) > 0) {
        return true;
    }

    return false;
}

} // namespace wsnet
