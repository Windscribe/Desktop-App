#import "Settings.h"
#import "Utils.h"
#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#import <Security/Security.h>
#include "../../client/client-common/utils/wsscopeguard.h"

// Build the resolver's IP set from the family-split lists.  The IpHostnamesManager keys its lookup on
// inet_ntop output, so v6 literals must be canonicalised (e.g. "2001:0db8::0001" -> "2001:db8::1") to
// match; v4 dotted-decimal already matches and is passed through unchanged.
static std::vector<std::string> ipsVectorFromLists(NSArray *ipsV4, NSArray *ipsV6)
{
    std::vector<std::string> ips;
    for (NSString *ip in ipsV4) {
        ips.push_back([ip UTF8String]);
    }
    for (NSString *ip in ipsV6) {
        const char *raw = [ip UTF8String];
        struct in6_addr addr6;
        char canonical[INET6_ADDRSTRLEN];
        if (inet_pton(AF_INET6, raw, &addr6) == 1
            && inet_ntop(AF_INET6, &addr6, canonical, sizeof(canonical)) != nullptr) {
            ips.push_back(canonical);
        } else {
            spdlog::warn("Skipping invalid IPv6 literal in split tunnel IP list: {}", raw);
        }
    }
    return ips;
}

static std::vector<std::string> hostnamesVectorFromList(NSArray *hostnames)
{
    std::vector<std::string> result;
    for (NSString *hostname in hostnames) {
        result.push_back([hostname UTF8String]);
    }
    return result;
}

@implementation Settings

@synthesize primaryInterface = primaryInterface_;
@synthesize vpnInterface = vpnInterface_;
@synthesize isDebug = isDebug_;

- (instancetype)initWithOptions:(NSDictionary<NSString *, id> *)options error:(NSError **)error {
    self = [super init];
    if (self) {
        lock_ = OS_UNFAIR_LOCK_INIT;
        if (!options) {
            if (error) {
                *error = [NSError errorWithDomain:@WS_MAC_SPLIT_TUNNEL_BUNDLE_ID
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

        primaryInterface_ = [Utils findInterfaceByName:options[@"primaryInterface"]];
        spdlog::debug("Primary interface: {}", [options[@"primaryInterface"] UTF8String]);
        if (!primaryInterface_) {
            if (error) {
                *error = [NSError errorWithDomain:@WS_MAC_SPLIT_TUNNEL_BUNDLE_ID
                                           code:3
                                       userInfo:@{NSLocalizedDescriptionKey: @"Could not find primary interface"}];
            }
            return nil;
        }

        vpnInterface_ = [Utils findInterfaceByName:options[@"vpnInterface"]];
        spdlog::debug("VPN interface: {}", [options[@"vpnInterface"] UTF8String]);
        if (!vpnInterface_) {
            if (error) {
                *error = [NSError errorWithDomain:@WS_MAC_SPLIT_TUNNEL_BUNDLE_ID
                                           code:4
                                       userInfo:@{NSLocalizedDescriptionKey: @"Could not find VPN interface"}];
            }
            return nil;
        }

        ipHostnamesManager_ = std::make_shared<IpHostnamesManager>();
        // Routing fields (apps/IPs/hostnames/mode) and the resolver share the live-update path.
        [self updateRoutingSettings:options];
    }
    return self;
}

- (bool)isExclude {
    os_unfair_lock_lock(&lock_);
    bool value = isExclude_;
    os_unfair_lock_unlock(&lock_);
    return value;
}

- (void)updateRoutingSettings:(NSDictionary<NSString *, id> *)options {
    // A live update carries only routing settings (apps/IPs/hostnames/mode).  The interfaces and the
    // debug/log level arrive as start options and are intentionally not read here: interfaces require a
    // tunnel restart, and debug is not a routing setting.
    NSArray *appPaths = options[@"appPaths"];
    NSArray *ips = options[@"ips"];
    NSArray *ipsV6 = options[@"ipsV6"];
    NSArray *hostnames = options[@"hostnames"];
    bool isExclude = [options[@"isExclude"] boolValue];

    // Swap the fields under the lock, then build the resolver inputs and re-resolve outside it: the
    // conversion canonicalises every v6 literal and the resolver call can block on its worker thread,
    // and lock_ is a spinlock the flow handlers also take, so neither may run under it.
    std::shared_ptr<IpHostnamesManager> manager;
    os_unfair_lock_lock(&lock_);
    // Only the IP/hostname lists drive a re-resolve; an apps-only or mode-only edit must not wipe and
    // re-query the resolved address set.
    bool listsChanged = ![ips isEqualToArray:ips_] || ![ipsV6 isEqualToArray:ipsV6_] ||
                        ![hostnames isEqualToArray:hostnames_];
    appPaths_ = appPaths;
    ips_ = ips;
    ipsV6_ = ipsV6;
    hostnames_ = hostnames;
    isExclude_ = isExclude;
    manager = ipHostnamesManager_;
    os_unfair_lock_unlock(&lock_);

    // A hostname change re-resolves asynchronously, so the resolved set is briefly empty until lookups
    // land -- inherent to any hostname change and the deliberate cost of applying routing live, not a leak.
    if (manager && listsChanged) {
        manager->setSettings(ipsVectorFromLists(ips, ipsV6), hostnamesVectorFromList(hostnames));
        manager->enable();
    }

    spdlog::debug("Exclude: {}", isExclude);
}

- (void)cleanup {
    // These are read by flow handlers on other queues, so clear them under the lock that guards them.
    // Move the manager reference out under the lock, then disable and drop it outside the lock so the
    // spinlock is never held across the resolver teardown.
    std::shared_ptr<IpHostnamesManager> manager;
    os_unfair_lock_lock(&lock_);
    appPaths_ = nil;
    isExclude_ = false;
    ips_ = nil;
    ipsV6_ = nil;
    hostnames_ = nil;
    manager = std::move(ipHostnamesManager_);
    os_unfair_lock_unlock(&lock_);

    if (manager) {
        manager->disable();
    }
    // manager (and the resolver thread it owns) is destroyed when the last reference drops.

    primaryInterface_ = nil;
    vpnInterface_ = nil;
    isDebug_ = false;
}

- (BOOL)isInAppList:(NEAppProxyFlow *)flow paths:(NSArray *)appPaths {
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

    // Check if the app path starts with any of the paths in appPaths
    for (NSString *path in appPaths) {
        if ([appPath hasPrefix:path]) {
            result = YES;
            break;
        }
    }

    return result;
}

- (BOOL)isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:(nw_endpoint_t)remoteEndpoint {
    // Snapshot the mode, app list, and resolver reference together so a live update can't swap one
    // against the others mid-call.  The reference is held strong so cleanup can't drop the manager from
    // under us, and its lookup (which takes the manager's own mutex) runs outside our spinlock.
    NSArray *appPaths;
    bool isExclude;
    std::shared_ptr<IpHostnamesManager> manager;
    os_unfair_lock_lock(&lock_);
    isExclude = isExclude_;
    appPaths = appPaths_;
    manager = ipHostnamesManager_;
    os_unfair_lock_unlock(&lock_);

    bool isInAppList = [self isInAppList:flow paths:appPaths];

    // If excluding and app is in the app list, split tunnel this flow.
    if (isExclude && isInAppList) {
        return YES;
    }

    // If including and app is in the app list, do not split tunnel this flow.
    if (!isExclude && isInAppList) {
        return NO;
    }

    // Otherwise, check if the remote endpoint is in the ip or hostname list.
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

    if (!inet_ntop(addr->sa_family, addrPtr, ipStr, sizeof(ipStr))) {
        return NO;
    }

    // If excluding and IP is in the list, or if including and IP is not in the list, split tunnel this flow.
    return manager && (isExclude == manager->isIpInList(ipStr)) ? YES : NO;
}

@end
