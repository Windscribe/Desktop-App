#pragma once

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

#include <vector>
#include <string>
#include "ip_hostnames/ip_hostnames_manager.h"

NS_ASSUME_NONNULL_BEGIN

@interface Settings : NSObject {
    NSArray *appPaths_;
    NSArray *ips_;
    NSArray *hostnames_;
    nw_interface_t primaryInterface_;
    nw_interface_t vpnInterface_;
    bool isExclude_;
    bool isDebug_;
    IpHostnamesManager *ipHostnamesManager_;
}

@property (nonatomic, readonly) NSArray *appPaths;
@property (nonatomic, readonly) NSArray *ips;
@property (nonatomic, readonly) NSArray *hostnames;
@property (nonatomic, readonly) nw_interface_t primaryInterface;
@property (nonatomic, readonly) nw_interface_t vpnInterface;
@property (nonatomic, readonly) bool isExclude;
@property (nonatomic, readonly) bool isDebug;
@property (nonatomic, readonly) IpHostnamesManager *ipHostnamesManager;

- (instancetype)initWithOptions:(NSDictionary<NSString *, id> *)options error:(NSError **)error;
- (void)cleanup;
- (BOOL)isInAppList:(NEAppProxyFlow *)flow;
- (BOOL)isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:(nw_endpoint_t)remoteEndpoint;
- (std::vector<std::string>)getIpsVector;
- (std::vector<std::string>)getHostnamesVector;

@end

NS_ASSUME_NONNULL_END
