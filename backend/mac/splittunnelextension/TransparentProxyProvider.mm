#import "TransparentProxyProvider.h"

#include <spdlog/spdlog.h>
#import <Security/Security.h>
#include "../../../client/common/utils/wsscopeguard.h"

#import "FlowTCP.h"
#import "FlowUDP.h"
#import "Utils.h"

@implementation TransparentProxyProvider

- (instancetype)init {
    spdlog::info("Initializing Windscribe split tunnel extension");
    self = [super init];
    if (self) {
        tcpHandler_ = [[FlowTCP alloc] init];
        udpHandler_ = [[FlowUDP alloc] init];
    }
    return self;
}

- (void)dealloc {
}

- (void)startProxyWithOptions:(NSDictionary<NSString *,id> * _Nullable)options completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler {
    spdlog::info("Starting Windscribe split tunnel extension");
    if (!options) {
        spdlog::error("No options provided");
        NSError *error = [NSError errorWithDomain:@"com.windscribe.client.splittunnelextension"
                                             code:1
                                         userInfo:@{NSLocalizedDescriptionKey: @"No options provided"}];
        completionHandler(error);
        return;
    }

    isDebug_ = [options[@"debug"] boolValue];
    auto level = isDebug_ ? spdlog::level::trace : spdlog::level::err;
    spdlog::set_level(level);
    spdlog::get("splittunnelextension")->flush_on(level);
    spdlog::info("Debug mode: {}", isDebug_);

    appPaths_ = options[@"appPaths"];
    isExclude_ = [options[@"isExclude"] boolValue];
    spdlog::debug("Exclude: {}", isExclude_);
    primaryInterface_ = [Utils findInterfaceByName:options[@"primaryInterface"]];
    spdlog::debug("Primary interface: {}", [options[@"primaryInterface"] UTF8String]);
    vpnInterface_ = [Utils findInterfaceByName:options[@"vpnInterface"]];
    spdlog::debug("VPN interface: {}", [options[@"vpnInterface"] UTF8String]);

    if (!primaryInterface_ || !vpnInterface_) {
        spdlog::error("Could not find primary/VPN interface");
        NSError *error = [NSError errorWithDomain:@"com.windscribe.client.splittunnelextension"
                                           code:3
                                       userInfo:@{NSLocalizedDescriptionKey: @"Could not find primary/VPN interface"}];
        completionHandler(error);
        return;

    }

    [Utils applyNetworkSettings:self completionHandler:completionHandler];
}

- (void)stopProxyWithReason:(NEProviderStopReason)reason completionHandler:(void (^)(void))completionHandler {
    spdlog::info("Stopping Windscribe split tunnel extension");
    appPaths_ = nil;
    primaryInterface_ = nil;
    vpnInterface_ = nil;
    isExclude_ = false;
    [tcpHandler_ cleanup];
    [udpHandler_ cleanup];
    completionHandler();
}

- (BOOL)handleNewFlow:(NEAppProxyFlow *)flow {
    NSString *appId = flow.metaData.sourceAppSigningIdentifier;

    if (![self isInAppList:flow]) {
        return NO; // Let the flow go out normally
    }

    nw_interface_t interface = isExclude_ ? primaryInterface_ : vpnInterface_;

    if ([flow isKindOfClass:[NEAppProxyTCPFlow class]]) {
        spdlog::info("Handling new TCP flow to interface: {} => {}", [appId UTF8String], nw_interface_get_name(interface));
        return [tcpHandler_ setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:interface];
    } else if ([flow isKindOfClass:[NEAppProxyUDPFlow class]]) {
        spdlog::info("Handling new UDP flow to interface: {} => {}", [appId UTF8String], nw_interface_get_name(interface));
        return [udpHandler_ setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:interface];
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


@end
