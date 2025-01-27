#import "TransparentProxyProvider.h"

#include <spdlog/spdlog.h>

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

    bundleIds_ = options[@"bundleIds"];
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
    bundleIds_ = nil;
    primaryInterface_ = nil;
    vpnInterface_ = nil;
    isExclude_ = false;
    [tcpHandler_ cleanup];
    [udpHandler_ cleanup];
    completionHandler();
}

- (BOOL)handleNewFlow:(NEAppProxyFlow *)flow {
    NSString *appId = flow.metaData.sourceAppSigningIdentifier;

    // exclusive, and app is not in the list, or inclusive, and app is in the list
    if (isExclude_ != [bundleIds_ containsObject:appId]) {
        return NO; // Let the flow go out normally
    }

    if (strcmp(nw_interface_get_name(flow.networkInterface), nw_interface_get_name(vpnInterface_)) != 0) {
        // Not on VPN interface, do nothing
        return NO;
    }

    // Handle flow based on type
    if ([flow isKindOfClass:[NEAppProxyTCPFlow class]]) {
        spdlog::info("Handling new TCP flow for app: {} => {}", [appId UTF8String], nw_interface_get_name(primaryInterface_));
        [tcpHandler_ setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:primaryInterface_];
    } else if ([flow isKindOfClass:[NEAppProxyUDPFlow class]]) {
        spdlog::info("Handling new UDP flow for app: {} => {}", [appId UTF8String], nw_interface_get_name(primaryInterface_));
        [udpHandler_ setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:primaryInterface_];
    }

    return YES;
}

@end
