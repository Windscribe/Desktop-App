#pragma once

#import <NetworkExtension/NetworkExtension.h>
#import <Network/Network.h>
@class FlowTCP;
@class FlowUDP;
@class Settings;

NS_ASSUME_NONNULL_BEGIN

@interface TransparentProxyProvider : NETransparentProxyProvider {
    // settings_ is published by startProxy and read from flow callbacks on other queues, so it is only
    // touched under @synchronized(self).  stopped_ (same lock) makes handleNewFlow: decline instantly
    // once a stop has begun, before anything reads session state that is being torn down.
    Settings *settings_;
    FlowTCP *tcpHandler_;
    FlowUDP *udpHandler_;
    BOOL stopped_;
    // Teardown work that can block -- the hostname resolver owns a worker thread its stop() joins --
    // runs here rather than on the NetworkExtension queue that delivers stopProxy and starts new flows.
    dispatch_queue_t teardownQueue_;
}

- (void)startProxyWithOptions:(NSDictionary<NSString *,id> * _Nullable)options
            completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler;

- (void)stopProxyWithReason:(NEProviderStopReason)reason
          completionHandler:(void (^ _Nonnull)(void))completionHandler;

- (BOOL)handleNewFlow:(NEAppProxyFlow * _Nonnull)flow;

@end

NS_ASSUME_NONNULL_END
