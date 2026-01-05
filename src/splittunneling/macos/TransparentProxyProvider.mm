#import "TransparentProxyProvider.h"

#include <spdlog/spdlog.h>
#import <Security/Security.h>
#include "../../client/common/utils/wsscopeguard.h"

#import "FlowTCP.h"
#import "FlowUDP.h"
#import "Utils.h"
#import "Settings.h"

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

- (void)startProxyWithOptions:(NSDictionary<NSString *,id> * _Nullable)options completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler {
    spdlog::info("Starting Windscribe split tunnel extension");

    if (!options) {
        NSError *error = [NSError errorWithDomain:@"com.windscribe.client.splittunnelextension" code:1 userInfo:@{NSLocalizedDescriptionKey: @"No options provided"}];
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

- (void)stopProxyWithReason:(NEProviderStopReason)reason completionHandler:(void (^)(void))completionHandler {
    spdlog::info("Stopping Windscribe split tunnel extension");
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
