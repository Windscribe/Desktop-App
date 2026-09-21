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

// Result of matching a flow's process against the configured app list.  Unknown is deliberately distinct
// from No: the process could not be attributed, so the caller must fail closed instead of treating the
// flow as one from an app that is simply not in the list.
typedef NS_ENUM(NSInteger, SplitTunnelAppMatch) {
    SplitTunnelAppMatchNo = 0,
    SplitTunnelAppMatchYes,
    SplitTunnelAppMatchUnknown,
};

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
    // Token → on-disk app path.  NSCache is thread-safe on its own, so this is not under lock_:
    // the mapping is independent of a live routing update (only appPaths_ is compared against it).
    NSCache<NSData *, NSString *> *appPathCache_;
    // Token → monotonic timestamp of the last failed resolution.  A token whose code lookup fails would
    // otherwise redo three Security calls (and log) for every datagram of that flow, so a failure is
    // remembered for a short window.  It is a throttle, not a negative cache: a throttled lookup answers
    // Unknown, never No, so the window costs a retry and never routes a flow on a guess.
    NSCache<NSData *, NSNumber *> *appPathFailureCache_;
    // Guards the routing fields a live update can swap (appPaths_/isExclude_/ips_/ipsV6_/hostnames_) and
    // the ipHostnamesManager_ reference against the flow handlers and cleanup that touch them on other
    // queues.  Held only to swap fields or grab a strong manager reference; the manager's own (possibly
    // blocking) calls run outside this lock, so it must never be held across them.
    // primaryInterface_/vpnInterface_ are intentionally NOT guarded: they are start options only, set at
    // init and never written again -- not by a live update, and not by cleanup (clearing them there
    // raced the flow callbacks that read them).  A stopped session drops the whole Settings object.
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
// Matches the flow's process against appPaths.  Answers Unknown when the process cannot be attributed
// (no audit token, or a code-signing lookup that failed or is throttled); see SplitTunnelAppMatch.
- (SplitTunnelAppMatch)appMatchForFlow:(NEAppProxyFlow *)flow paths:(NSArray *)appPaths;
- (BOOL)isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:(nw_endpoint_t)remoteEndpoint;

@end

NS_ASSUME_NONNULL_END
