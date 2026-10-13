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
        stopped_ = NO;
        teardownQueue_ = dispatch_queue_create(WS_PRODUCT_NAME_LOWER ".splittunnel.teardown", DISPATCH_QUEUE_SERIAL);
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
    Settings *settings = [[Settings alloc] initWithOptions:options error:&error];
    if (!settings) {
        completionHandler(error);
        return;
    }

    @synchronized (self) {
        settings_ = settings;
        stopped_ = NO;   // a fresh session clears the stop that was declining flows
    }

    [tcpHandler_ setSettings:settings];
    [udpHandler_ setSettings:settings];

    [Utils applyNetworkSettings:self completionHandler:completionHandler];
}

- (void)handleAppMessage:(NSData *)messageData completionHandler:(void (^ _Nullable)(NSData * _Nullable))completionHandler {
    // A live routing-settings update (apps/IPs/hostnames/mode) from the client.  Interface changes never
    // arrive this way -- they require a tunnel restart -- so the running proxy can apply this in place.
    Settings *settings;
    @synchronized (self) {
        settings = settings_;
    }
    if (!settings) {
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
    [settings updateRoutingSettings:(NSDictionary *)plist];

    if (completionHandler) {
        // Non-empty reply acks that the update was applied; the client restarts the tunnel on an empty
        // reply (the failure paths above) so the settings still reach us as start options.
        const uint8_t ok = 1;
        completionHandler([NSData dataWithBytes:&ok length:1]);
    }
}

- (void)stopProxyWithReason:(NEProviderStopReason)reason completionHandler:(void (^)(void))completionHandler {
    spdlog::info("Stopping " WS_PRODUCT_NAME " split tunnel extension");

    // Decline new flows and drop the session reference first, so anything the system delivers while the
    // teardown runs is rejected before it can reach state that is going away.
    Settings *settings;
    @synchronized (self) {
        stopped_ = YES;
        settings = settings_;
        settings_ = nil;
    }

    // Close the flows we hold before reporting the stop: they belong to the system, and each handler's
    // cleanup is bounded work under its own lock.
    [tcpHandler_ cleanup];
    [udpHandler_ cleanup];
    completionHandler();

    // Settings cleanup destroys the hostname resolver, whose stop() joins its worker thread.  Blocking
    // the NetworkExtension queue that delivered this stop -- the same queue that starts new flows on it
    // -- is what a crash report caught us doing, so the blocking part goes to our own queue.
    dispatch_async(teardownQueue_, ^{
        [settings cleanup];
    });
}

- (BOOL)handleNewFlow:(NEAppProxyFlow *)flow {
    // The system keeps delivering flows while a stop is in progress; decline them before reading any
    // session state.
    Settings *settings;
    @synchronized (self) {
        if (stopped_) {
            return NO;
        }
        settings = settings_;
    }
    if (!settings) {
        return NO;
    }

    nw_interface_t flowInterface = flow.networkInterface;
    if (!flowInterface) {
        spdlog::info("Flow interface not found");
        return NO;
    }

    const char *flowInterfaceName = nw_interface_get_name(flowInterface);
    const char *primaryInterfaceName = nw_interface_get_name(settings.primaryInterface);
    const char *vpnInterfaceName = nw_interface_get_name(settings.vpnInterface);

    // nw_interface_get_name yields NULL for a nil interface, and strcmp faults on it.
    if (!flowInterfaceName || !primaryInterfaceName || !vpnInterfaceName) {
        spdlog::info("Flow declined: interface name unavailable");
        return NO;
    }

    // Where a split flow egresses.  A flow routed into the tunnel leaves via the primary interface, the one
    // the tunnel itself runs over.  A flow the system routed to a physical interface -- the primary, or a
    // secondary network such as a second LAN or Wi-Fi + cellular -- stays on that interface: moving it to
    // the primary would send it to a network the destination may not be reachable from.  It has to be
    // proxied rather than declined either way, because the firewall only lets the extension's group out
    // around the tunnel; a declined flow leaves with the app's group and is dropped.
    const BOOL onTunnel = strcmp(flowInterfaceName, vpnInterfaceName) == 0;
    nw_interface_t egressInterface = nil;
    if (onTunnel) {
        egressInterface = settings.primaryInterface;
    } else if (strcmp(flowInterfaceName, primaryInterfaceName) == 0 || [Utils isSplitEligibleInterface:flowInterface]) {
        egressInterface = flowInterface;
    } else {
        spdlog::info("Flow interface ({}) is neither the VPN interface ({}) nor a physical interface", flowInterfaceName, vpnInterfaceName);
        return NO;
    }

    if ([flow isKindOfClass:[NEAppProxyTCPFlow class]]) {
        return [tcpHandler_ setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:egressInterface];
    } else if ([flow isKindOfClass:[NEAppProxyUDPFlow class]]) {
        return [udpHandler_ setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:egressInterface onTunnel:onTunnel];
    }
    return NO;
}

@end
