#include "getdnsconfig_mac.h"
#include <SystemConfiguration/SystemConfiguration.h>
#include <CoreFoundation/CoreFoundation.h>
#include <vector>
#include <unordered_set>
#include "utils/wsnet_logger.h"
#include "utils/utils.h"


namespace wsnet {

void removeDuplicates(std::vector<std::string> &vec)
{
    std::unordered_set<std::string> seen;
    auto it = vec.begin();
    while (it != vec.end()) {
        if (seen.find(*it) != seen.end()) {
            it = vec.erase(it);
        } else {
            seen.insert(*it);
            ++it;
        }
    }
}

std::string getDnsConfig_mac()
{
    std::vector<std::string> servers;

    SCDynamicStoreRef store = SCDynamicStoreCreate(nullptr, CFSTR("GetDNSInfo"), nullptr, nullptr);
    if (!store) {
        g_logger->error("getOsDnsServersFromPath(), SCDynamicStoreCreate failed");
        return std::string();
    }

    CFDictionaryRef dnsInfo = (CFDictionaryRef)SCDynamicStoreCopyValue(store, CFSTR("State:/Network/Global/DNS"));
    if (!dnsInfo) {
        g_logger->error("getOsDnsServersFromPath(), SCDynamicStoreCopyValue failed");
        CFRelease(store);
        return std::string();
    }

    CFArrayRef serverAddresses = (CFArrayRef)CFDictionaryGetValue(dnsInfo, CFSTR("ServerAddresses"));
    if (serverAddresses && CFGetTypeID(serverAddresses) == CFArrayGetTypeID()) {
        CFIndex count = CFArrayGetCount(serverAddresses);
        for (CFIndex i = 0; i < count; i++) {
            CFStringRef server = (CFStringRef)CFArrayGetValueAtIndex(serverAddresses, i);
            char buffer[256];
            if (CFStringGetCString(server, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                servers.push_back(buffer);
            }
        }
    }

    CFRelease(dnsInfo);
    CFRelease(store);

    removeDuplicates(servers);
    return utils::join(servers, ",");
}

} // namespace wsnet


