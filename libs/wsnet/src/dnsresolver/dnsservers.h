#pragma once

#include <string>
#include <vector>
#include <ares.h>

namespace wsnet {

// wrapper for struct ares_addr_node for convenient work
class DnsServers
{
public:
    DnsServers() : servers_(nullptr) {}
    DnsServers(const std::vector<std::string> &ips);
    DnsServers(const struct ares_addr_node *servers);
    DnsServers(const DnsServers &other);

    ~DnsServers();
    DnsServers& operator=(const DnsServers &other);
    bool operator==( const DnsServers &other ) const;
    bool operator!=( const DnsServers &other ) const;

    struct ares_addr_node *getForCares() { return servers_; }
    std::string getAsSting() const;
    bool isEmpty() const { return servers_ == nullptr; }

private:
    struct ares_addr_node *servers_;

    void cleanup();
    struct ares_addr_node *copyList(const ares_addr_node *head);
};


} // namespace wsnet
