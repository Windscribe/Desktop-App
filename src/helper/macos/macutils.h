#pragma once

#include <string>
#include <vector>

namespace MacUtils
{
    bool setDnsOfDynamicStoreEntry(std::string dnsIp, std::string dynEntry);
    std::string resourcePath();
    std::string bundleVersionFromPlist();

    // The OS-configured system resolvers (dual-stack), read from SystemConfiguration. Used by the
    // kill-switch <disallowed_dns> pf table to block the host's own DNS servers around the VPN.
    // Performed in the privileged helper so it is not trusted from the client.
    std::vector<std::string> getOsDnsServers();
}
