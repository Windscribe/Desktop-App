#include "dnsservers.h"
#include <assert.h>
#include "utils/utils.h"

namespace wsnet {

DnsServers::DnsServers(const std::vector<std::string> &ips)
{
    servers_ = ips;
    for (auto &it : servers_) {
        it += ":53";
    }
}

DnsServers::DnsServers(const char *csv)
{
    servers_ = utils::split(csv, ',');
}

bool DnsServers::operator==(const DnsServers &other) const
{
    return servers_ == other.servers_;
}

bool DnsServers::operator!=(const DnsServers &other) const
{
    return !operator==(other);
}

std::string DnsServers::getAsCsv() const
{
    return utils::join(servers_, ",");
}

} // namespace wsnet
