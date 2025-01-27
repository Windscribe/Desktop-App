#pragma once

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>
#import <Network/Network.h>

NS_ASSUME_NONNULL_BEGIN

@interface FlowTCP : NSObject
{
    NSMutableDictionary<NSValue *, nw_connection_t> *activeConnections_;  // Maps NEAppProxyTCPFlow -> nw_connection_t
}

- (instancetype)init;
- (void)setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface;
- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
