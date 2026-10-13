#pragma once

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

NS_ASSUME_NONNULL_BEGIN

@interface Utils : NSObject

+ (nw_interface_t _Nullable)findInterfaceByName:(NSString *)interface;
// Whether a flow on this interface may be split out of the tunnel onto it: a wired, Wi-Fi or cellular
// interface that is not one of the AWDL/peer-to-peer links.
+ (BOOL)isSplitEligibleInterface:(nw_interface_t)interface;

+ (nw_endpoint_t _Nullable)convertToNewEndpoint:(NWEndpoint *)endpoint;
+ (NWEndpoint * _Nullable)convertToOldEndpoint:(nw_endpoint_t)endpoint;

+ (void)applyNetworkSettings:(NETransparentProxyProvider * _Nonnull)provider
                             completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler;

+ (BOOL)isSameEndpoint:(nw_endpoint_t)endpoint1 endpoint:(nw_endpoint_t)endpoint2;

+ (NSError * _Nullable)errorFromNWError:(nw_error_t)nwError;

+ (BOOL)isLanRange:(nw_endpoint_t)endpoint;

@end

NS_ASSUME_NONNULL_END
