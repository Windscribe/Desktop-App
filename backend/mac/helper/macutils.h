#pragma once

#include <string>

namespace MacUtils
{
    bool setDnsOfDynamicStoreEntry(std::string dnsIp, std::string dynEntry);
    std::string resourcePath();
    std::string bundleVersionFromPlist();
}
