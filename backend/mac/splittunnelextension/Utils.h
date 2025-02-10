#pragma once

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

NS_ASSUME_NONNULL_BEGIN

@interface Utils : NSObject

+ (nw_interface_t)findInterfaceByName:(NSString *)interface;

+ (nw_endpoint_t)convertToNewEndpoint:(NWEndpoint *)endpoint;
+ (NWEndpoint *)convertToOldEndpoint:(nw_endpoint_t)endpoint;

+ (void)applyNetworkSettings:(NETransparentProxyProvider * _Nonnull)provider
                             completionHandler:(void (^ _Nonnull)(NSError * _Nullable))completionHandler;

+ (BOOL)isSameEndpoint:(nw_endpoint_t)endpoint1 endpoint:(nw_endpoint_t)endpoint2;

+ (NSError * _Nullable)errorFromNWError:(nw_error_t)nwError;

+ (BOOL)isLanRange:(nw_endpoint_t)endpoint;

@end

NS_ASSUME_NONNULL_END
