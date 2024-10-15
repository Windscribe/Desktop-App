#pragma once

#include <string>
#include <vector>
#include <ares.h>

namespace wsnet {

// wrapper for struct ares_addr_node for convenient work
class DnsServers
{
public:
    DnsServers() {}
    DnsServers(const std::vector<std::string> &ips);
    DnsServers(const char *csv);

    bool operator==( const DnsServers &other ) const;
    bool operator!=( const DnsServers &other ) const;

    bool isEmpty() const { return servers_.empty(); }

    std::string getAsCsv() const;

private:
    std::vector<std::string> servers_;
};

} // namespace wsnet
