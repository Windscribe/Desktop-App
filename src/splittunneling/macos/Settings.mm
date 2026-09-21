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

// Path-boundary prefix: "/Applications/Safari.app" matches itself and
// "/Applications/Safari.app/Contents/...", but not "/Applications/Safari.app.evil".
// A configured entry can carry a trailing separator ("/Applications/Safari.app/") -- it comes straight
// from the preferences file, which only checks that the path exists -- and names the same bundle, so the
// separator is stripped before comparing.  An entry that is nothing but separators names no app, and
// matches nothing rather than every process on the system.
static BOOL appPathMatchesPrefix(NSString *appPath, NSString *prefix)
{
    NSUInteger prefixLength = prefix.length;
    while (prefixLength > 0 && [prefix characterAtIndex:prefixLength - 1] == '/') {
        prefixLength--;
    }
    if (prefixLength == 0 || ![appPath hasPrefix:[prefix substringToIndex:prefixLength]]) {
        return NO;
    }
    return appPath.length == prefixLength || [appPath characterAtIndex:prefixLength] == '/';
}

// How long a failed code-signing lookup suppresses a retry for the same audit token.  Long enough to
// keep a UDP flow's per-datagram checks off the Security calls, short enough that a transient failure
// costs one window rather than the life of the process.
static const NSTimeInterval kAppPathFailureRetryInterval = 5.0;

@interface Settings ()
// Resolves the on-disk path of the process behind an audit token, or nil if it cannot be determined.
- (NSString * _Nullable)resolveAppPathForAuditToken:(NSData *)auditToken;
@end

@implementation Settings

@synthesize primaryInterface = primaryInterface_;
@synthesize vpnInterface = vpnInterface_;
@synthesize isDebug = isDebug_;

- (instancetype)initWithOptions:(NSDictionary<NSString *, id> *)options error:(NSError **)error {
    self = [super init];
    if (self) {
        lock_ = OS_UNFAIR_LOCK_INIT;
        appPathCache_ = [[NSCache alloc] init];
        appPathCache_.countLimit = 256;
        appPathFailureCache_ = [[NSCache alloc] init];
        appPathFailureCache_.countLimit = 256;
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

    // primaryInterface_/vpnInterface_ are deliberately left alone: they are unguarded start options (see
    // the header) that flow callbacks still running on other queues read.  Clearing them here was a write
    // racing those reads, and handleNewFlow: crashed in strcmp on the resulting NULL name.  They are
    // released along with this object, which a stopped session drops anyway.
    isDebug_ = false;
}

- (NSString * _Nullable)resolveAppPathForAuditToken:(NSData *)auditToken {
    CFMutableDictionaryRef mutableAttributes = CFDictionaryCreateMutable(NULL, 1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    if (!mutableAttributes) {
        spdlog::error("Failed to create mutable attributes dictionary");
        return nil;
    }

    CFDictionaryAddValue(mutableAttributes, kSecGuestAttributeAudit, (__bridge CFDataRef)auditToken);

    CFDictionaryRef attributes = CFDictionaryCreateCopy(NULL, mutableAttributes);
    CFRelease(mutableAttributes);

    if (!attributes) {
        spdlog::error("Failed to create immutable attributes dictionary");
        return nil;
    }
    auto attributesGuard = wsl::wsScopeGuard([attributes]{ CFRelease(attributes); });

    SecCodeRef dynamicCodeRef = NULL;
    OSStatus status = SecCodeCopyGuestWithAttributes(NULL, attributes, kSecCSDefaultFlags, &dynamicCodeRef);
    if (status != errSecSuccess) {
        spdlog::error("Failed to create code reference from audit token: {}", status);
        return nil;
    }
    auto dynamicCodeGuard = wsl::wsScopeGuard([dynamicCodeRef]{ CFRelease(dynamicCodeRef); });

    SecStaticCodeRef codeRef = NULL;
    status = SecCodeCopyStaticCode(dynamicCodeRef, kSecCSDefaultFlags, &codeRef);
    if (status != errSecSuccess) {
        spdlog::error("Failed to get static code reference: {}", status);
        return nil;
    }
    auto codeGuard = wsl::wsScopeGuard([codeRef]{ CFRelease(codeRef); });

    CFURLRef pathURL = NULL;
    status = SecCodeCopyPath(codeRef, kSecCSDefaultFlags, &pathURL);
    if (status != errSecSuccess) {
        spdlog::error("Failed to get path URL: {}", status);
        return nil;
    }
    auto pathGuard = wsl::wsScopeGuard([pathURL]{ CFRelease(pathURL); });

    NSString *appPath = [(__bridge NSURL *)pathURL path];
    if (!appPath) {
        spdlog::error("Failed to get path string");
        return nil;
    }
    return appPath;
}

- (SplitTunnelAppMatch)appMatchForFlow:(NEAppProxyFlow *)flow paths:(NSArray *)appPaths {
    if (appPaths.count == 0) {
        // Nothing to match against, so attribution cannot change the answer: No, not Unknown.
        return SplitTunnelAppMatchNo;
    }

    // The token is nullable (a flow from a process the system cannot attribute has none), and handing a
    // nil value to CFDictionaryAddValue below throws NSInvalidArgumentException, which would take the
    // whole extension -- and with it every split flow -- down.  Logged at debug: UDP runs this per
    // datagram, so an unattributable flow must not flood the log at the default error level.
    NSData *auditToken = flow.metaData.sourceAppAuditToken;
    if (!auditToken) {
        spdlog::debug("Flow has no source app audit token");
        return SplitTunnelAppMatchUnknown;
    }

    // Cache the resolved path, not membership in appPaths: a live update can swap the list, and
    // SecCodeCopyGuestWithAttributes on every UDP datagram is the expensive part.
    NSString *appPath = [appPathCache_ objectForKey:auditToken];
    if (!appPath) {
        // A token whose lookup just failed is not retried (nor logged again) until the window passes:
        // failures are not cached, so without this every datagram of that flow repeats the three
        // Security calls.  systemUptime is monotonic, so a wall-clock change cannot stretch the window.
        const NSTimeInterval now = NSProcessInfo.processInfo.systemUptime;
        NSNumber *lastFailure = [appPathFailureCache_ objectForKey:auditToken];
        if (lastFailure && now - lastFailure.doubleValue < kAppPathFailureRetryInterval) {
            return SplitTunnelAppMatchUnknown;
        }

        appPath = [self resolveAppPathForAuditToken:auditToken];
        if (!appPath) {
            [appPathFailureCache_ setObject:@(now) forKey:auditToken];
            return SplitTunnelAppMatchUnknown;
        }
        [appPathCache_ setObject:appPath forKey:auditToken];
    }

    for (NSString *path in appPaths) {
        if (appPathMatchesPrefix(appPath, path)) {
            return SplitTunnelAppMatchYes;
        }
    }
    return SplitTunnelAppMatchNo;
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

    const SplitTunnelAppMatch match = [self appMatchForFlow:flow paths:appPaths];

    // The process could not be attributed, so fail closed: leave the flow on the default route, which is
    // the tunnel.  Inclusive mode has to say so explicitly -- an unattributable flow would otherwise fall
    // through to the IP check below and be split out of the tunnel on the strength of a lookup that never
    // answered.  Exclusive mode already falls back to the tunnel, so Unknown and No lead to the same
    // place there and the attribution-independent IP/hostname rules still get their say.
    if (match == SplitTunnelAppMatchUnknown && !isExclude) {
        return NO;
    }
    const bool isInAppList = (match == SplitTunnelAppMatchYes);

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
