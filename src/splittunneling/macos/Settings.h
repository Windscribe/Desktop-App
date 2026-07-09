#pragma once

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>
#import <os/lock.h>

#include <memory>
#include <vector>
#include <string>
#include "ip_hostnames/ip_hostnames_manager.h"

NS_ASSUME_NONNULL_BEGIN

@interface Settings : NSObject {
    NSArray *appPaths_;
    NSArray *ips_;
    NSArray *ipsV6_;
    NSArray *hostnames_;
    nw_interface_t primaryInterface_;
    nw_interface_t vpnInterface_;
    bool isExclude_;
    bool isDebug_;
    std::shared_ptr<IpHostnamesManager> ipHostnamesManager_;
    // Guards the routing fields a live update can swap (appPaths_/isExclude_/ips_/ipsV6_/hostnames_) and
    // the ipHostnamesManager_ reference against the flow handlers and cleanup that touch them on other
    // queues.  Held only to swap fields or grab a strong manager reference; the manager's own (possibly
    // blocking) calls run outside this lock, so it must never be held across them.
    // primaryInterface_/vpnInterface_ are intentionally NOT guarded: they are start options only, set at
    // init and unchanged until a tunnel restart, so no live update ever swaps them.
    os_unfair_lock lock_;
}

@property (nonatomic, readonly) nw_interface_t primaryInterface;
@property (nonatomic, readonly) nw_interface_t vpnInterface;
@property (nonatomic, readonly) bool isExclude;
@property (nonatomic, readonly) bool isDebug;

- (instancetype)initWithOptions:(NSDictionary<NSString *, id> *)options error:(NSError **)error;
// Applies a live routing-settings update (apps/IPs/hostnames/mode) from a running provider, re-resolving
// the hostname list when it changed.  Routing settings are the ONLY thing a live update carries: the
// interfaces and the debug/log level are start options only and are intentionally not re-applied here
// (interfaces require a tunnel restart; debug is not a routing setting).
- (void)updateRoutingSettings:(NSDictionary<NSString *, id> *)options;
- (void)cleanup;
- (BOOL)isInAppList:(NEAppProxyFlow *)flow paths:(NSArray *)appPaths;
- (BOOL)isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:(nw_endpoint_t)remoteEndpoint;

@end

NS_ASSUME_NONNULL_END
