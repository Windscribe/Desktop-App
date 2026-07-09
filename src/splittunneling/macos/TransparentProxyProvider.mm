#import "TransparentProxyProvider.h"

#include <spdlog/spdlog.h>
#import <Security/Security.h>
#include "../../client/client-common/utils/wsscopeguard.h"

#import "FlowTCP.h"
#import "FlowUDP.h"
#import "Utils.h"
#import "Settings.h"

@implementation TransparentProxyProvider

- (instancetype)init {
    spdlog::info("Initializing " WS_PRODUCT_NAME " split tunnel extension");
    self = [super init];
    if (self) {
        tcpHandler_ = [[FlowTCP alloc] init];
        udpHandler_ = [[FlowUDP alloc] init];
    }
    return self;
}

- (void)startProxyWithOptions:(NSDictionary<NSString *,id> * _Nullable)options completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler {
    spdlog::info("Starting " WS_PRODUCT_NAME " split tunnel extension");

    if (!options) {
        NSError *error = [NSError errorWithDomain:@WS_MAC_SPLIT_TUNNEL_BUNDLE_ID code:1 userInfo:@{NSLocalizedDescriptionKey: @"No options provided"}];
        completionHandler(error);
        return;
    }

    NSError *error = nil;
    settings_ = [[Settings alloc] initWithOptions:options error:&error];
    if (!settings_) {
        completionHandler(error);
        return;
    }

    [tcpHandler_ setSettings:settings_];
    [udpHandler_ setSettings:settings_];

    [Utils applyNetworkSettings:self completionHandler:completionHandler];
}

- (void)handleAppMessage:(NSData *)messageData completionHandler:(void (^ _Nullable)(NSData * _Nullable))completionHandler {
    // A live routing-settings update (apps/IPs/hostnames/mode) from the client.  Interface changes never
    // arrive this way -- they require a tunnel restart -- so the running proxy can apply this in place.
    if (!settings_) {
        spdlog::error("Received settings update before the proxy was started");
        if (completionHandler) {
            completionHandler(nil);
        }
        return;
    }

    NSError *error = nil;
    id plist = [NSPropertyListSerialization propertyListWithData:messageData
                                                         options:NSPropertyListImmutable
                                                          format:nil
                                                           error:&error];
    if (![plist isKindOfClass:[NSDictionary class]]) {
        spdlog::error("Failed to parse split tunnel settings update");
        if (completionHandler) {
            completionHandler(nil);
        }
        return;
    }

    spdlog::info("Applying live " WS_PRODUCT_NAME " split tunnel settings update");
    [settings_ updateRoutingSettings:(NSDictionary *)plist];

    if (completionHandler) {
        // Non-empty reply acks that the update was applied; the client restarts the tunnel on an empty
        // reply (the failure paths above) so the settings still reach us as start options.
        const uint8_t ok = 1;
        completionHandler([NSData dataWithBytes:&ok length:1]);
    }
}

- (void)stopProxyWithReason:(NEProviderStopReason)reason completionHandler:(void (^)(void))completionHandler {
    spdlog::info("Stopping " WS_PRODUCT_NAME " split tunnel extension");
    [settings_ cleanup];
    [tcpHandler_ cleanup];
    [udpHandler_ cleanup];
    completionHandler();
}

- (BOOL)handleNewFlow:(NEAppProxyFlow *)flow {
    nw_interface_t flowInterface = flow.networkInterface;
    if (!flowInterface) {
        spdlog::info("Flow interface not found");
        return NO;
    }

    const char *flowInterfaceName = nw_interface_get_name(flowInterface);
    const char *primaryInterfaceName = nw_interface_get_name(settings_.primaryInterface);
    const char *vpnInterfaceName = nw_interface_get_name(settings_.vpnInterface);

    if (strcmp(flowInterfaceName, primaryInterfaceName) != 0 && strcmp(flowInterfaceName, vpnInterfaceName) != 0) {
        spdlog::info("Flow interface ({}) not in primary ({}) or VPN interface ({})", flowInterfaceName, primaryInterfaceName, vpnInterfaceName);
        return NO;
    }

    if ([flow isKindOfClass:[NEAppProxyTCPFlow class]]) {
        return [tcpHandler_ setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:settings_.primaryInterface];
    } else if ([flow isKindOfClass:[NEAppProxyUDPFlow class]]) {
        return [udpHandler_ setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:settings_.primaryInterface];
    }
    return NO;
}

@end
