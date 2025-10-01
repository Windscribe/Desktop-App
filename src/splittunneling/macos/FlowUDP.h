#pragma once

#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>
#import "Settings.h"

NS_ASSUME_NONNULL_BEGIN

@interface FlowUDP : NSObject {
    NSMutableDictionary<NSValue *, NEAppProxyUDPFlow *> *activeConnections_;  // Maps nw_connection_t -> NEAppProxyUDPFlow
    Settings *settings_;
}

- (instancetype)init;
- (void)setSettings:(Settings *)settings;
- (BOOL)setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
