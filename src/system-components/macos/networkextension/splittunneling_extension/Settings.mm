#import "Settings.h"
#import "Utils.h"
#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#import <Security/Security.h>
#include "../../../../client/common/utils/wsscopeguard.h"

@implementation Settings

@synthesize appPaths = appPaths_;
@synthesize ips = ips_;
@synthesize hostnames = hostnames_;
@synthesize primaryInterface = primaryInterface_;
@synthesize vpnInterface = vpnInterface_;
@synthesize isExclude = isExclude_;
@synthesize isDebug = isDebug_;
@synthesize ipHostnamesManager = ipHostnamesManager_;

- (instancetype)initWithOptions:(NSDictionary<NSString *, id> *)options error:(NSError **)error {
    self = [super init];
    if (self) {
        if (!options) {
            if (error) {
                *error = [NSError errorWithDomain:@"com.windscribe.client.splittunnelextension"
                                           code:1
                                       userInfo:@{NSLocalizedDescriptionKey: @"No options provided"}];
            }
            return nil;
        }

        isDebug_ = [options[@"debug"] boolValue];
        auto level = isDebug_ ? spdlog::level::trace : spdlog::level::err;
        spdlog::set_level(level);
        spdlog::get("splittunnelextension")->flush_on(level);
        spdlog::info("Debug mode: {}", isDebug_);

        appPaths_ = options[@"appPaths"];
        ips_ = options[@"ips"];
        hostnames_ = options[@"hostnames"];

        isExclude_ = [options[@"isExclude"] boolValue];
        spdlog::debug("Exclude: {}", isExclude_);

        primaryInterface_ = [Utils findInterfaceByName:options[@"primaryInterface"]];
        spdlog::debug("Primary interface: {}", [options[@"primaryInterface"] UTF8String]);
        if (!primaryInterface_) {
            if (error) {
                *error = [NSError errorWithDomain:@"com.windscribe.client.splittunnelextension"
                                           code:3
                                       userInfo:@{NSLocalizedDescriptionKey: @"Could not find primary interface"}];
            }
            return nil;
        }

        vpnInterface_ = [Utils findInterfaceByName:options[@"vpnInterface"]];
        spdlog::debug("VPN interface: {}", [options[@"vpnInterface"] UTF8String]);
        if (!vpnInterface_) {
            if (error) {
                *error = [NSError errorWithDomain:@"com.windscribe.client.splittunnelextension"
                                           code:4
                                       userInfo:@{NSLocalizedDescriptionKey: @"Could not find VPN interface"}];
            }
            return nil;
        }

        ipHostnamesManager_ = new IpHostnamesManager();
        ipHostnamesManager_->setSettings([self getIpsVector], [self getHostnamesVector]);
        ipHostnamesManager_->enable();
    }
    return self;
}

- (void)cleanup {
    appPaths_ = nil;
    ips_ = nil;
    hostnames_ = nil;
    primaryInterface_ = nil;
    vpnInterface_ = nil;
    isExclude_ = false;
    isDebug_ = false;

    if (ipHostnamesManager_) {
        ipHostnamesManager_->disable();
        delete ipHostnamesManager_;
        ipHostnamesManager_ = nullptr;
    }
}

- (BOOL)isInAppList:(NEAppProxyFlow *)flow {
    BOOL result = NO;

    // Create mutable attributes dictionary
    CFMutableDictionaryRef mutableAttributes = CFDictionaryCreateMutable(NULL, 1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    if (!mutableAttributes) {
        spdlog::error("Failed to create mutable attributes dictionary");
        return NO;
    }

    // Add audit token to dictionary
    CFDictionaryAddValue(mutableAttributes, kSecGuestAttributeAudit, (CFDataRef)(flow.metaData.sourceAppAuditToken));

    // Create immutable copy
    CFDictionaryRef attributes = CFDictionaryCreateCopy(NULL, mutableAttributes);
    CFRelease(mutableAttributes);

    if (!attributes) {
        spdlog::error("Failed to create immutable attributes dictionary");
        return NO;
    }
    auto attributesGuard = wsl::wsScopeGuard([attributes]{ CFRelease(attributes); });

    // Get dynamic code reference
    SecCodeRef dynamicCodeRef = NULL;
    OSStatus status = SecCodeCopyGuestWithAttributes(nil, attributes, kSecCSDefaultFlags, &dynamicCodeRef);
    if (status != errSecSuccess) {
        spdlog::error("Failed to create code reference from audit token: {}", status);
        return NO;
    }
    auto dynamicCodeGuard = wsl::wsScopeGuard([dynamicCodeRef]{ CFRelease(dynamicCodeRef); });

    // Get static code reference
    SecStaticCodeRef codeRef = NULL;
    status = SecCodeCopyStaticCode(dynamicCodeRef, kSecCSDefaultFlags, &codeRef);
    if (status != errSecSuccess) {
        spdlog::error("Failed to get static code reference: {}", status);
        return NO;
    }
    auto codeGuard = wsl::wsScopeGuard([codeRef]{ CFRelease(codeRef); });

    // Get path URL
    CFURLRef pathURL = NULL;
    status = SecCodeCopyPath(codeRef, kSecCSDefaultFlags, &pathURL);
    if (status != errSecSuccess) {
        spdlog::error("Failed to get path URL: {}", status);
        return NO;
    }
    auto pathGuard = wsl::wsScopeGuard([pathURL]{ CFRelease(pathURL); });

    // Convert URL to path string
    NSString *appPath = [(__bridge NSURL *)pathURL path];
    if (!appPath) {
        spdlog::error("Failed to get path string");
        return NO;
    }

    // Check if the app path starts with any of the paths in appPaths_
    for (NSString *path in appPaths_) {
        if ([appPath hasPrefix:path]) {
            result = YES;
            break;
        }
    }

    return result;
}

- (std::vector<std::string>)getIpsVector {
    std::vector<std::string> ips;
    for (NSString *ip in ips_) {
        ips.push_back([ip UTF8String]);
    }
    return ips;
}

- (std::vector<std::string>)getHostnamesVector {
    std::vector<std::string> hostnames;
    for (NSString *hostname in hostnames_) {
        hostnames.push_back([hostname UTF8String]);
    }
    return hostnames;
}

- (BOOL)isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:(nw_endpoint_t)remoteEndpoint {
    bool isInAppList = [self isInAppList:flow];

    // If excluding and app is in the app list, split tunnel this flow.
    if (isExclude_ && isInAppList) {
        return YES;
    }

    // If including and app is in the app list, do not split tunnel this flow.
    if (!isExclude_ && isInAppList) {
        return NO;
    }

    // Otherwise, check if the remote endpoint is in the ip or hostname list.
    if (!ipHostnamesManager_) {
        return NO;
    }

    const struct sockaddr *addr = nw_endpoint_get_address(remoteEndpoint);
    if (!addr) {
        return NO;
    }

    char ipStr[INET6_ADDRSTRLEN];
    const void *addrPtr;

    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in *addr_in = (const struct sockaddr_in *)addr;
        addrPtr = &(addr_in->sin_addr);
    } else if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6 *addr_in6 = (const struct sockaddr_in6 *)addr;
        addrPtr = &(addr_in6->sin6_addr);
    } else {
        return NO;
    }

    if (inet_ntop(addr->sa_family, addrPtr, ipStr, sizeof(ipStr))) {
        // If excluding and IP is in the list, or if including and IP is not in the list, split tunnel this flow.
        return isExclude_ == ipHostnamesManager_->isIpInList(ipStr);
    }

    return NO;
}

@end
