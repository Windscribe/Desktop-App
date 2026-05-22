#import "Utils.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <spdlog/spdlog.h>

@implementation Utils

+ (nw_endpoint_t)convertToNewEndpoint:(NWEndpoint *)endpoint {
    if ([endpoint isKindOfClass:[NWHostEndpoint class]]) {
        NWHostEndpoint *hostEndpoint = (NWHostEndpoint *)endpoint;
        const char *hostname = [hostEndpoint.hostname UTF8String];
        const char *port = [[hostEndpoint port] UTF8String];
        if (hostname && port) {
            return nw_endpoint_create_host(hostname, port);
        }
    }
    return NULL;
}

+ (NWEndpoint *)convertToOldEndpoint:(nw_endpoint_t)endpoint {
    if (!endpoint) return nil;
    const char *hostname = nw_endpoint_get_hostname(endpoint);
    uint16_t port = nw_endpoint_get_port(endpoint);
    if (!hostname) return nil;
    NSString *hostnameString = [NSString stringWithUTF8String:hostname];
    if (!hostnameString) return nil;
    NSString *portString = [NSString stringWithFormat:@"%d", port];

    return [NWHostEndpoint endpointWithHostname:hostnameString port:portString];
}

+ (void)applyNetworkSettings:(NETransparentProxyProvider * _Nonnull)provider
                             completionHandler:(void (^ _Nonnull)(NSError * _Nullable))handler
{
    NETransparentProxyNetworkSettings *settings = [[NETransparentProxyNetworkSettings alloc] initWithTunnelRemoteAddress:@"127.0.0.1"];
    // All-nil host + prefix 0 is Apple's documented "match everything" form for NENetworkRule,
    // and empirically matches both AF_INET and AF_INET6 flows on macOS 13/14/15. An earlier
    // attempt to add a second explicit `::/0` rule via `NWHostEndpoint host=:: port=0
    // remotePrefix=0` was rejected by `setTunnelNetworkSettings:` with
    // "Either a non-wildcard port or a non-wildcard address must be specified", which silently
    // dropped the entire ruleset — Apple disallows wildcard host + wildcard port + prefix 0
    // when an `NWHostEndpoint` is provided explicitly, but accepts the same shape when both
    // endpoints are nil.
    NENetworkRule *includeRule =
        [[NENetworkRule alloc] initWithRemoteNetwork:nil
                                        remotePrefix:0
                                        localNetwork:nil
                                         localPrefix:0
                                            protocol:NENetworkRuleProtocolAny
                                           direction:NETrafficDirectionOutbound];

    NWHostEndpoint *localhostEndpoint = [NWHostEndpoint endpointWithHostname:@"127.0.0.0" port:@"0"];
    NENetworkRule *localhostRule =
        [[NENetworkRule alloc] initWithRemoteNetwork:localhostEndpoint
                                        remotePrefix:8
                                        localNetwork:nil
                                         localPrefix:0
                                            protocol:NENetworkRuleProtocolAny
                                           direction:NETrafficDirectionOutbound];

    // v6 sibling of the localhost exclude rule — without this, ::1 traffic would be hijacked
    // into the proxy once the all-nil include rule starts matching v6 flows. 127.0.0.0/8
    // doesn't cover ::1. /128 is non-wildcard so this rule is accepted by NENetworkRule.
    NWHostEndpoint *localhostV6Endpoint = [NWHostEndpoint endpointWithHostname:@"::1" port:@"0"];
    NENetworkRule *localhostV6Rule =
        [[NENetworkRule alloc] initWithRemoteNetwork:localhostV6Endpoint
                                        remotePrefix:128
                                        localNetwork:nil
                                         localPrefix:0
                                            protocol:NENetworkRuleProtocolAny
                                           direction:NETrafficDirectionOutbound];

    settings.includedNetworkRules = @[includeRule];
    settings.excludedNetworkRules = @[localhostRule, localhostV6Rule];

    [provider setTunnelNetworkSettings:settings completionHandler:^(NSError * _Nullable error) {
        if (error) {
            spdlog::error("Failed to set tunnel network settings: {}", [[error localizedDescription] UTF8String]);
            handler(error);
        } else {
            spdlog::debug("Successfully set tunnel network settings, proxy started");
            handler(nil);
        }
    }];
}

+ (nw_interface_t)findInterfaceByName:(NSString *)interface {
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    nw_path_monitor_t monitor = nw_path_monitor_create();
    dispatch_queue_t queue = dispatch_queue_create(WS_PRODUCT_NAME_LOWER ".interfacefinder", DISPATCH_QUEUE_SERIAL);
    nw_path_monitor_set_queue(monitor, queue);

    __block nw_interface_t foundInterface = nil;
    nw_path_monitor_set_update_handler(monitor, ^(nw_path_t path) {
        nw_path_enumerate_interfaces(path, ^bool(nw_interface_t nwInterface) {
            if (strcmp(nw_interface_get_name(nwInterface), [interface UTF8String]) == 0) {
                foundInterface = nwInterface;
                dispatch_semaphore_signal(semaphore);
                return false; // Stop enumeration
            }
            return true; // Continue enumeration
        });
    });

    nw_path_monitor_start(monitor);

    // Wait up to 1 second for the interface to be found
    dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC)); // NOLINT: Semaphore is intentionally used here to synchronously wait for async network path monitor callback
    nw_path_monitor_cancel(monitor);

    return foundInterface;
}

+ (BOOL)isSameEndpoint:(nw_endpoint_t)endpoint1 endpoint:(nw_endpoint_t)endpoint2 {
    if (!endpoint1 || !endpoint2) return NO;

    const char *host1 = nw_endpoint_get_hostname(endpoint1);
    const char *host2 = nw_endpoint_get_hostname(endpoint2);
    if ((host1 != NULL && host2 == NULL) || (host1 == NULL && host2 != NULL)) {
        return NO;
    }
    if (host1 != NULL && host2 != NULL && strcmp(host1, host2) != 0) {
        return NO;
    }
    uint16_t port1 = nw_endpoint_get_port(endpoint1);
    uint16_t port2 = nw_endpoint_get_port(endpoint2);
    return port1 == port2;
}

+ (NSError *)errorFromNWError:(nw_error_t)nwError {
    if (!nwError) return nil;

    NSString *description = [NSString stringWithFormat:@"Network error: domain=%d code=%d",
                           nw_error_get_error_domain(nwError),
                           nw_error_get_error_code(nwError)];

    return [NSError errorWithDomain:@WS_MAC_SPLIT_TUNNEL_BUNDLE_ID
                             code:nw_error_get_error_code(nwError)
                         userInfo:@{NSLocalizedDescriptionKey: description}];
}

+ (BOOL)isLanRange:(nw_endpoint_t)endpoint {
    if (!endpoint) {
        return NO;
    }

    const struct sockaddr *addr = nw_endpoint_get_address(endpoint);
    if (!addr) {
        return NO;
    }

    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in *addr_in = (const struct sockaddr_in *)addr;
        uint32_t ip = ntohl(addr_in->sin_addr.s_addr);

        return ((ip & 0xFF000000) == 0x0A000000) ||    // 10.0.0.0/8
               ((ip & 0xFFFF0000) == 0xA9FE0000) ||    // 169.254.0.0/16
               ((ip & 0xFFF00000) == 0xAC100000) ||    // 172.16.0.0/12
               ((ip & 0xFFFF0000) == 0xC0A80000);      // 192.168.0.0/16
    }

    if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6 *addr_in6 = (const struct sockaddr_in6 *)addr;
        const struct in6_addr *ip6 = &addr_in6->sin6_addr;

        // Loopback (::1) — also excluded via the network-rule path in applyNetworkSettings, but
        // checked here as defence in depth for the inclusive-mode flow gate.
        if (IN6_IS_ADDR_LOOPBACK(ip6)) return YES;
        // Link-local (fe80::/10) — never routable beyond the segment, must stay on original iface.
        if (IN6_IS_ADDR_LINKLOCAL(ip6)) return YES;
        // Multicast (ff00::/8) — covers mDNS (ff02::fb), SSDP (ff05::c), MLD reports, etc.
        if (IN6_IS_ADDR_MULTICAST(ip6)) return YES;
        // Unique Local Addresses (ULA, fc00::/7) — RFC 4193 private addressing for the user's LAN.
        // High 7 bits == 1111110, i.e. first byte masked with 0xfe equals 0xfc.
        if ((ip6->s6_addr[0] & 0xfe) == 0xfc) return YES;
        // IPv4-mapped IPv6 (::ffff:0:0/96) — apply the existing v4 LAN check on the embedded
        // octets (last 4 bytes of sin6_addr).
        if (IN6_IS_ADDR_V4MAPPED(ip6)) {
            uint32_t v4;
            std::memcpy(&v4, &ip6->s6_addr[12], sizeof(v4));
            v4 = ntohl(v4);
            return ((v4 & 0xFF000000) == 0x0A000000) ||
                   ((v4 & 0xFFFF0000) == 0xA9FE0000) ||
                   ((v4 & 0xFFF00000) == 0xAC100000) ||
                   ((v4 & 0xFFFF0000) == 0xC0A80000);
        }
        // Global unicast (2000::/3) and everything else — not LAN, route via split tunnel.
        return NO;
    }

    return NO;
}

@end
