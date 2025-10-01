#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>

#include <spdlog/spdlog.h>
#include "macutils.h"
#include "utils/wsscopeguard.h"

namespace MacUtils
{

bool setDnsOfDynamicStoreEntry(std::string dnsIp, std::string dynEntry)
{
    // get current dns entry
    SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("WindscribeDnsSetter"), NULL, NULL);
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
    return "/Applications/Windscribe.app/Contents/Resources/";
}

std::string bundleVersionFromPlist()
{
    NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
    NSString *buildNumber = [infoDict objectForKey:@"CFBundleVersion"];
    return [buildNumber UTF8String];
}

} // namespace MacUtils
