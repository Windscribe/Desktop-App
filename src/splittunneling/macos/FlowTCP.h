#pragma once

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>
#import <Network/Network.h>
#import "Settings.h"

NS_ASSUME_NONNULL_BEGIN

@interface FlowTCP : NSObject
{
    NSMutableDictionary<NSValue *, nw_connection_t> *activeConnections_;  // Maps NEAppProxyTCPFlow -> nw_connection_t
    Settings *settings_;
    // activeConnections_ is reached from the connection's main-queue callbacks and from flow completions
    // on other queues, so every access is serialised under @synchronized(self).  stopped_ (same lock)
    // rejects new connections once cleanup has run.
    BOOL stopped_;
}

- (instancetype)init;
- (void)setSettings:(Settings *)settings;
- (BOOL)setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
