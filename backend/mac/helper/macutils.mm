#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>

#include "logger.h"
#include "macutils.h"

namespace MacUtils
{

bool setDnsOfDynamicStoreEntry(std::string dnsIp, std::string dynEntry)
{
    // get current dns entry
    SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("WindscribeDnsSetter"), NULL, NULL);
    NSString *entryNsString = [NSString stringWithCString:dynEntry.c_str() encoding:[NSString defaultCStringEncoding]];
    CFDictionaryRef dnskey = (CFDictionaryRef) SCDynamicStoreCopyValue(dynRef, (CFStringRef) entryNsString);

    // prep dns server entry
    CFMutableDictionaryRef newdnskey = CFDictionaryCreateMutableCopy(NULL,0,dnskey);
    NSString *dnsIpsNsString = [NSString stringWithCString:dnsIp.c_str() encoding:[NSString defaultCStringEncoding]];
    CFMutableArrayRef dnsserveraddresses = CFArrayCreateMutable(NULL,0,NULL);
    CFArrayAppendValue(dnsserveraddresses, (CFStringRef) dnsIpsNsString);
    CFDictionarySetValue(newdnskey, CFSTR("ServerAddresses"), dnsserveraddresses);

    // apply
    bool success = SCDynamicStoreSetValue(dynRef, (CFStringRef) entryNsString, newdnskey);

    if (!success)
    {
        LOG("Failed to apply dns to store entry");
    }

    CFRelease(dnskey);
    CFRelease(newdnskey);
    CFRelease(dnsserveraddresses);
    CFRelease(dynRef);
    return success;
}

std::string resourcePath()
{
    return "/Applications/Windscribe.app/Contents/Resources/";
}

} // namespace MacUtils
