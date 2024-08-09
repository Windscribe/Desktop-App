#include "dnsservers.h"
#include <assert.h>
#include <spdlog/spdlog.h>

namespace wsnet {

DnsServers::DnsServers(const std::vector<std::string> &ips)
{
    servers_ = nullptr;

    if (ips.empty())
        return;

    struct ares_addr_node *prevNode = nullptr;
    for (const auto &ip : ips) {
        struct sockaddr_in sa;
        int res = ares_inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
        struct ares_addr_node *newNode = nullptr;
        if (res > 0) {
            newNode = new ares_addr_node();
            newNode->family = AF_INET;
            newNode->addr.addr4 = sa.sin_addr;
            newNode->next = nullptr;
        } else {    // if IPv4 failed, try IPv6
            struct sockaddr_in6 sa6;
            res = ares_inet_pton(AF_INET6, ip.c_str(), &(sa6.sin6_addr));
            if (res > 0) {
                newNode = new ares_addr_node();
                newNode->family = AF_INET6;
                memcpy(&newNode->addr.addr6, &sa6.sin6_addr, sizeof(newNode->addr.addr6));
                newNode->next = nullptr;
            }
        }

        if (newNode == nullptr) {
            spdlog::error("Incorrect DNS server specified: {}, skip it", ip);
            continue;
        }

        if (servers_ == nullptr) {
            servers_ = newNode;
        } else {
            prevNode->next = newNode;
        }
        prevNode = newNode;
    }
}

DnsServers::DnsServers(const struct ares_addr_node *servers)
{
    servers_ = copyList(servers);
}

DnsServers::DnsServers(const DnsServers &other)
{
    servers_ = copyList(other.servers_);
}

DnsServers::~DnsServers()
{
    cleanup();
}

DnsServers &DnsServers::operator=(const DnsServers &other)
{
    cleanup();
    servers_ = copyList(other.servers_);
    return *this;
}

bool DnsServers::operator==(const DnsServers &other) const
{
    struct ares_addr_node *a = servers_;
    struct ares_addr_node *b = other.servers_;

    // Returns true if linked lists a and b are identical, otherwise false
    while (a != nullptr && b != nullptr) {
        if (a->family != b->family) {
            return false;
        }

        if (a->family == AF_INET) {
            if (memcmp(&a->addr.addr4, &b->addr.addr4, sizeof(a->addr.addr4)) != 0) {
                return false;
            }
        } else if (a->family == AF_INET6) {
            if (memcmp(&a->addr.addr6, &b->addr.addr6, sizeof(a->addr.addr6)) != 0) {
                return false;
            }
        } else {
            assert(false);
        }
        a = a->next;
        b = b->next;
    }
    return (a == nullptr && b == nullptr);
}

bool DnsServers::operator!=(const DnsServers &other) const
{
    return !operator==(other);
}

std::string DnsServers::getAsSting() const
{
    std::string result;

    struct ares_addr_node *current = servers_;
    while (current) {

        if (!result.empty())
            result += "; ";

        char buf[INET6_ADDRSTRLEN];
        ares_inet_ntop(current->family, &current->addr, buf,  INET6_ADDRSTRLEN);
        result += buf;
        current = current->next;
    }

    return result;
}

void DnsServers::cleanup()
{
    struct ares_addr_node *current = servers_;
    while (current) {
        struct ares_addr_node *next = current->next;
        delete current;
        current = next;
    }
}

struct ares_addr_node *DnsServers::copyList(const struct ares_addr_node *head)
{
    if (head == nullptr)
        return nullptr;

    struct ares_addr_node *newNode = new ares_addr_node();
    *newNode = *head;
    newNode->next = copyList(head->next);
    return newNode;
}

} // namespace wsnet
