#pragma once

#import <NetworkExtension/NetworkExtension.h>
#import <Network/Network.h>
@class FlowTCP;
@class FlowUDP;
@class Settings;

NS_ASSUME_NONNULL_BEGIN

@interface TransparentProxyProvider : NETransparentProxyProvider {
    Settings *settings_;
    FlowTCP *tcpHandler_;
    FlowUDP *udpHandler_;
}

- (void)startProxyWithOptions:(NSDictionary<NSString *,id> * _Nullable)options
            completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler;

- (void)stopProxyWithReason:(NEProviderStopReason)reason
          completionHandler:(void (^ _Nonnull)(void))completionHandler;

- (BOOL)handleNewFlow:(NEAppProxyFlow * _Nonnull)flow;

@end

NS_ASSUME_NONNULL_END
