#ifndef macutils_h
#define macutils_h

#include <string>

namespace MacUtils
{
    bool setDnsOfDynamicStoreEntry(std::string dnsIp, std::string dynEntry);
    std::string resourcePath();
}

#endif /* macutils_h */
