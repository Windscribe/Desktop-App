#pragma once

#include <string>
#include <vector>
#include <ares.h>

namespace wsnet {

// wrapper for struct ares_addr_node for convenient work
class DnsServers
{
public:
    DnsServers();
    DnsServers(const std::vector<std::string> &ips);
    DnsServers(const char *csv);
    DnsServers(const DnsServers &other);

    ~DnsServers();
    DnsServers& operator=(const DnsServers &other);
    bool operator==( const DnsServers &other ) const;
    bool operator!=( const DnsServers &other ) const;

    std::string get() const;
    bool isEmpty() const { return servers_.empty(); }

private:
    std::string servers_;

    bool parseAddress(const std::string &ip);
};


} // namespace wsnet
