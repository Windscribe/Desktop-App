#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>

#include <set>
#include <spdlog/spdlog.h>
#include "macutils.h"
#include "utils/wsscopeguard.h"

namespace MacUtils
{

bool setDnsOfDynamicStoreEntry(std::string dnsIp, std::string dynEntry)
{
    // get current dns entry
    SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("" WS_APP_IDENTIFIER "DnsSetter"), NULL, NULL);
    if (dynRef == NULL) {
        spdlog::error("setDnsOfDynamicStoreEntry - SCDynamicStoreCreate failed");
        return false;
    }

    CFDictionaryRef dnskey = NULL;
    CFMutableDictionaryRef newdnskey = NULL;
    CFMutableArrayRef dnsserveraddresses = NULL;
    auto exitGuard = wsl::wsScopeGuard([&] {
        if (dnsserveraddresses != NULL) {
            CFRelease(dnsserveraddresses);
        }
        if (newdnskey != NULL) {
            CFRelease(newdnskey);
        }
        if (dnskey != NULL) {
            CFRelease(dnskey);
        }
        CFRelease(dynRef);
    });

    NSString *entryNsString = [NSString stringWithCString:dynEntry.c_str() encoding:[NSString defaultCStringEncoding]];
    dnskey = (CFDictionaryRef) SCDynamicStoreCopyValue(dynRef, (CFStringRef) entryNsString);
    if (dnskey == NULL) {
        spdlog::error("setDnsOfDynamicStoreEntry - SCDynamicStoreCopyValue failed");
        return false;
    }

    // prep dns server entry
    newdnskey = CFDictionaryCreateMutableCopy(NULL, 0, dnskey);
    if (newdnskey == NULL) {
        spdlog::error("setDnsOfDynamicStoreEntry - CFDictionaryCreateMutableCopy failed");
        return false;
    }

    NSString *dnsIpsNsString = [NSString stringWithCString:dnsIp.c_str() encoding:[NSString defaultCStringEncoding]];
    dnsserveraddresses = CFArrayCreateMutable(NULL, 0, NULL);
    if (dnsserveraddresses == NULL) {
        spdlog::error("setDnsOfDynamicStoreEntry - CFArrayCreateMutable failed");
        return false;
    }

    CFArrayAppendValue(dnsserveraddresses, (CFStringRef) dnsIpsNsString);
    CFDictionarySetValue(newdnskey, CFSTR("ServerAddresses"), dnsserveraddresses);

    // apply
    bool success = SCDynamicStoreSetValue(dynRef, (CFStringRef) entryNsString, newdnskey);

    if (!success)
    {
        spdlog::error("Failed to apply dns to store entry");
    }

    return success;
}

std::string resourcePath()
{
    return WS_MAC_APP_DIR "/Contents/Resources/";
}

std::string bundleVersionFromPlist()
{
    NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
    NSString *buildNumber = [infoDict objectForKey:@"CFBundleVersion"];
    return [buildNumber UTF8String];
}

std::vector<std::string> getOsDnsServers()
{
    std::vector<std::string> result;
    std::set<std::string> seen;

    // One dynamic store for the whole call; the lambda just queries different paths against it.
    SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("DNSSETTING"), NULL, NULL);
    if (dynRef == NULL) {
        spdlog::error("getOsDnsServers - SCDynamicStoreCreate failed");
        return result;
    }

    auto addFromPath = [&](CFStringRef path) {
        CFPropertyListRef propList = SCDynamicStoreCopyValue(dynRef, path);
        if (propList == NULL) {
            return;
        }
        // The dynamic store holds arbitrary plist values; verify each level's type before casting so
        // a malformed entry (non-dictionary value, non-array ServerAddresses, or a non-string
        // element) can't crash the helper via a bad cast or a -UTF8String sent to a non-string.
        if (CFGetTypeID(propList) == CFDictionaryGetTypeID()) {
            CFArrayRef addresses = (CFArrayRef)CFDictionaryGetValue((CFDictionaryRef)propList, CFSTR("ServerAddresses"));
            if (addresses != NULL && CFGetTypeID(addresses) == CFArrayGetTypeID()) {
                for (CFIndex j = 0; j < CFArrayGetCount(addresses); j++) {
                    CFStringRef addr = (CFStringRef)CFArrayGetValueAtIndex(addresses, j);
                    if (addr != NULL && CFGetTypeID(addr) == CFStringGetTypeID()) {
                        const char *c = [(__bridge NSString *)addr UTF8String];
                        if (c != NULL) {
                            std::string s = c;
                            if (!s.empty() && seen.insert(s).second) {
                                result.push_back(s);
                            }
                        }
                    }
                }
            }
        }
        CFRelease(propList);
    };

    SCPreferencesRef prefsDNS = SCPreferencesCreate(NULL, CFSTR("DNSSETTING"), NULL);
    if (prefsDNS == NULL) {
        spdlog::error("getOsDnsServers - SCPreferencesCreate failed");
        CFRelease(dynRef);
        return result;
    }

    CFArrayRef services = SCNetworkServiceCopyAll(prefsDNS);
    if (services != NULL) {
        for (CFIndex i = 0; i < CFArrayGetCount(services); i++) {
            SCNetworkServiceRef service = (SCNetworkServiceRef)CFArrayGetValueAtIndex(services, i);
            CFStringRef serviceId = SCNetworkServiceGetServiceID(service);
            CFStringRef path = CFStringCreateWithFormat(NULL, NULL, CFSTR("State:/Network/Service/%@/DNS"), serviceId);
            if (path != NULL) {
                addFromPath(path);
                CFRelease(path);
            }
        }
        CFRelease(services);
    }

    CFStringRef globalPath = CFStringCreateWithFormat(NULL, NULL, CFSTR("State:/Network/Global/DNS"));
    if (globalPath != NULL) {
        addFromPath(globalPath);
        CFRelease(globalPath);
    }

    CFRelease(prefsDNS);
    CFRelease(dynRef);
    return result;
}

} // namespace MacUtils
