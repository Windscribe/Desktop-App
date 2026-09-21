#pragma once

#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>
#import "Settings.h"

NS_ASSUME_NONNULL_BEGIN

@interface FlowUDP : NSObject {
    // A UDP flow is one app socket talking to many destinations, so each flow owns its own map of
    // connections, keyed by destination.  Reusing a connection is then a hash lookup in that flow's
    // map instead of a scan over every connection of every flow.  NSMapTable because
    // NEAppProxyUDPFlow is not NSCopying: flows are keyed by pointer identity and held strongly, as
    // they were when they were the values of the old dictionary.
    NSMapTable<NEAppProxyUDPFlow *, NSMutableDictionary<NSString *, id> *> *flowConnections_;
    Settings *settings_;
    // flowConnections_ is reached from the connection's main-queue callbacks and from flow completions
    // on other queues, so every access is serialised under @synchronized(self).  stopped_ (same lock)
    // rejects new connections once cleanup has run.
    BOOL stopped_;
}

- (instancetype)init;
- (void)setSettings:(Settings *)settings;
- (BOOL)setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
