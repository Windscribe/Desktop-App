#pragma once

#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SplitTunnelUDPFlow <NSObject>
@property (readonly, nullable) NWEndpoint *localEndpoint;
@property (readonly) NEFlowMetaData *metaData;
@property (readonly, nullable) nw_interface_t networkInterface;
- (void)openWithLocalEndpoint:(nullable NWHostEndpoint *)localEndpoint
            completionHandler:(void (^)(NSError * _Nullable error))completionHandler;
- (void)readDatagramsWithCompletionHandler:(void (^)(NSArray<NSData *> * _Nullable datagrams,
                                                     NSArray<NWEndpoint *> * _Nullable remoteEndpoints,
                                                     NSError * _Nullable error))completionHandler;
- (void)writeDatagrams:(NSArray<NSData *> *)datagrams
       sentByEndpoints:(NSArray<NWEndpoint *> *)remoteEndpoints
     completionHandler:(void (^)(NSError * _Nullable error))completionHandler;
- (void)closeReadWithError:(nullable NSError *)error;
- (void)closeWriteWithError:(nullable NSError *)error;
@end

@protocol SplitTunnelUDPRouting <NSObject>
- (BOOL)isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:(nw_endpoint_t)remoteEndpoint;
- (BOOL)isVpnServer:(nw_endpoint_t)endpoint;
- (BOOL)isFromVpnProcess:(NEAppProxyFlow *)flow;
@end

@interface NEAppProxyUDPFlow (SplitTunnelUDPFlow) <SplitTunnelUDPFlow>
@end

@interface FlowUDP : NSObject {
    // A UDP flow is one app socket talking to many destinations, so each flow owns one unconnected socket
    // per interface and address family.  NSMapTable because flows are keyed by pointer identity and held
    // strongly.
    NSMapTable<id<SplitTunnelUDPFlow>, NSMutableDictionary<NSString *, id> *> *flowSockets_;
    id<SplitTunnelUDPRouting> settings_;
    // flowSockets_ is reached from the sockets' main-queue callbacks and from flow completions
    // on other queues, so every access is serialised under @synchronized(self).  stopped_ (same lock)
    // rejects new sockets once cleanup has run.
    BOOL stopped_;
}

- (instancetype)init;
- (void)setSettings:(id<SplitTunnelUDPRouting>)settings;
// onTunnel: the system routed the flow into the VPN interface (decided once in handleNewFlow).
- (BOOL)setupUDPConnection:(id<SplitTunnelUDPFlow>)flow interface:(nw_interface_t _Nullable)interface onTunnel:(BOOL)onTunnel;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
