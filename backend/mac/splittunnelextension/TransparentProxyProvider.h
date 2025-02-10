#pragma once

#import <NetworkExtension/NetworkExtension.h>
#import <Network/Network.h>
@class FlowTCP;
@class FlowUDP;

NS_ASSUME_NONNULL_BEGIN

@interface TransparentProxyProvider : NETransparentProxyProvider {
    NSArray *appPaths_;
    nw_interface_t primaryInterface_;
    nw_interface_t vpnInterface_;
    bool isExclude_;
    bool isDebug_;
    FlowTCP *tcpHandler_;
    FlowUDP *udpHandler_;
}

- (void)startProxyWithOptions:(NSDictionary<NSString *,id> * _Nullable)options
            completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler;

- (void)stopProxyWithReason:(NEProviderStopReason)reason
          completionHandler:(void (^ _Nonnull)(void))completionHandler;

- (BOOL)handleNewFlow:(NEAppProxyFlow * _Nonnull)flow;

- (BOOL)isInAppList:(NEAppProxyFlow * _Nonnull)flow;

@end

NS_ASSUME_NONNULL_END
