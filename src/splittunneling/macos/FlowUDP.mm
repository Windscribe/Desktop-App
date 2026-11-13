#import "FlowUDP.h"

#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

#include "UdpSocket.h"
#import "Utils.h"

@interface FlowUDPSocket : NSObject {
@public
    std::unique_ptr<UdpSocket> socket;
}
@end

@implementation FlowUDPSocket
@end

@implementation FlowUDP

- (instancetype)init {
    self = [super init];
    if (self) {
        flowSockets_ = [NSMapTable mapTableWithKeyOptions:NSPointerFunctionsObjectPointerPersonality | NSPointerFunctionsStrongMemory
                                             valueOptions:NSPointerFunctionsObjectPersonality | NSPointerFunctionsStrongMemory];
        settings_ = nil;
    }
    return self;
}

- (void)setSettings:(id<SplitTunnelUDPRouting>)settings {
    // settings_ is published here (start queue) and read on the flow queues, so guard it with the same
    // lock.  Re-arm for a fresh session: a new proxy start clears the stop that disabled connections.
    @synchronized (self) {
        settings_ = settings;
        stopped_ = NO;
    }
}

- (void)cleanupFlow:(id<SplitTunnelUDPFlow>)flow withError:(NSError * _Nullable)error {
    // Take the flow's sockets out first, so nothing can be handed out while they are closed.
    NSArray *sockets = nil;
    @synchronized (self) {
        sockets = [[flowSockets_ objectForKey:flow] allValues];
        [flowSockets_ removeObjectForKey:flow];
    }
    sockets = nil;
    [flow closeReadWithError:error];
    [flow closeWriteWithError:error];
}

- (BOOL)setupUDPConnection:(id<SplitTunnelUDPFlow>)flow interface:(nw_interface_t _Nullable)interface onTunnel:(BOOL)onTunnel {
    id<SplitTunnelUDPRouting> settings;
    BOOL stopped;
    @synchronized (self) {
        settings = settings_;
        stopped = stopped_;
    }
    if (!settings) {
        spdlog::error("[UDP] Settings not initialized");
        return NO;
    }
    // Decline the flow once the proxy has stopped instead of claiming it and closing it on the first
    // datagram, so the system keeps routing it normally.  Mirrors the stopped_ check in FlowTCP, and
    // at the same level: the system keeps handing us flows all through a teardown, and error is the
    // release log level, so one disconnect under load would bury a log that keeps 2 MB plus a single
    // backup.
    if (stopped) {
        spdlog::debug("[UDP] Proxy stopped, declining flow");
        return NO;
    }

    [flow openWithLocalEndpoint:(NWHostEndpoint *)flow.localEndpoint completionHandler:^(NSError * _Nullable error) {
        if (error) {
            // Not an error level: a flow whose peer went away while it was being opened is routine --
            // every flow in flight when the tunnel drops ends up here.  At error level one teardown
            // under load writes megabytes, and the log keeps only 2 MB plus one backup, so the spam
            // erases the records of whatever actually went wrong.
            spdlog::debug("[UDP] flow open error: {}", [[error localizedDescription] UTF8String]);
            [self cleanupFlow:flow withError:error];
            return;
        }
        spdlog::debug("[UDP] Flow opened successfully");
        [self handleUDPOutboundFlow:flow interface:interface onTunnel:onTunnel];
    }];

    // For UDP, we don't know what the remote endpoint is until we process the datagrams, so we always accept the flow.
    const char *app = [flow.metaData.sourceAppSigningIdentifier UTF8String];
    const char *interfaceName = interface ? nw_interface_get_name(interface) : nullptr;
    spdlog::info("[UDP] Handling flow from {} => {}", app ? app : "<null>", interfaceName ? interfaceName : "<null>");
    return YES;
}

- (void)startReadingSocket:(FlowUDPSocket *)socket forFlow:(id<SplitTunnelUDPFlow>)flow {
    socket->socket->startReading(dispatch_get_main_queue(), [self, flow](std::vector<UdpSocket::Datagram> batch,
                                                                         UdpSocket::ResumeReading resume) {
        NSMutableArray<NSData *> *datagrams = [NSMutableArray arrayWithCapacity:batch.size()];
        NSMutableArray<NWEndpoint *> *endpoints = [NSMutableArray arrayWithCapacity:batch.size()];
        for (const auto &datagram : batch) {
            nw_endpoint_t sender = nw_endpoint_create_address(reinterpret_cast<const sockaddr *>(&datagram.from));
            NWEndpoint *endpoint = sender ? [Utils convertToOldEndpoint:sender] : nil;
            if (!endpoint) {
                spdlog::debug("[UDP] failed to convert sender address, dropping datagram");
                continue;
            }
            [datagrams addObject:[NSData dataWithBytes:datagram.data.data() length:datagram.data.size()]];
            [endpoints addObject:endpoint];
        }
        if (datagrams.count == 0) {
            resume();
            return;
        }

        [flow writeDatagrams:datagrams sentByEndpoints:endpoints completionHandler:^(NSError * _Nullable error) {
            if (error) {
                spdlog::error("[UDP] write error to flow: {}", [[error localizedDescription] UTF8String]);
                [self cleanupFlow:flow withError:error];
                return;
            }
            resume();
        }];
    });
}

- (void)handleUDPOutboundFlow:(id<SplitTunnelUDPFlow>)flow interface:(nw_interface_t _Nullable)interface onTunnel:(BOOL)onTunnel {
    [flow readDatagramsWithCompletionHandler:^(NSArray<NSData *> * _Nullable datagrams, NSArray<NWEndpoint *> * _Nullable endpoints,
                                               NSError * _Nullable error) {
        if (error) {
            spdlog::error("[UDP] Read error: {} ({})", [[error localizedDescription] UTF8String], error.code);
            [self cleanupFlow:flow withError:error];
            return;
        }

        if (!datagrams) {
            spdlog::debug("[UDP] No outbound data received, retrying");
            [self handleUDPOutboundFlow:flow interface:interface onTunnel:onTunnel];
            return;
        }

        if (datagrams.count == 0) {
            spdlog::info("[UDP] Empty outbound data received, cleaning up flow");
            [self cleanupFlow:flow withError:nil];
            return;
        }

        id<SplitTunnelUDPRouting> settings;
        @synchronized (self) {
            settings = settings_;
        }

        // Forward each datagram through the target interface
        for (NSUInteger i = 0; i < datagrams.count; i++) {
            NSData *datagram = datagrams[i];

            nw_endpoint_t endpoint = i < endpoints.count ? [Utils convertToNewEndpoint:endpoints[i]] : nil;
            if (!endpoint) {
                // One unusable destination is not a reason to close the socket the app is using for
                // every other destination: drop this datagram and carry on.
                spdlog::error("[UDP] failed to create endpoint, dropping datagram");
                continue;
            }

            nw_interface_t targetInterface = interface;

            const BOOL isLan = [Utils isLanRange:endpoint];
            const BOOL isSplit = !isLan && [settings isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:endpoint];

            // Anything we relay leaves under our group, which the firewall always passes.  So a non-split datagram
            // off the tunnel (the app bound this socket to a physical interface, or the system routed it there)
            // is dropped, as the firewall would drop it from the app.  LAN goes on: the firewall's
            // LAN rules apply to our group too.  The VPN server is let through only for the processes the firewall allows.
            if (!onTunnel && !isLan && !isSplit &&
                !([settings isVpnServer:endpoint] && [settings isFromVpnProcess:(NEAppProxyFlow *)flow])) {
                spdlog::debug("[UDP] dropping non-split datagram off the tunnel to {}", (nw_endpoint_get_hostname(endpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(endpoint));
                continue;
            }

            // DNS traffic should stay on the original (VPN) interface.  ROBERT can only be reached from the VPN interface.
            // If it's a LAN range (including the reserved 10.255.255.0/24 range), we leave the traffic on the original interface.
            // Note that the firewall may still block this later.
            if (nw_endpoint_get_port(endpoint) == 53 || !isSplit) {
                targetInterface = flow.networkInterface;
            }

            const struct sockaddr *address = nw_endpoint_get_address(endpoint);
            if (!address || (address->sa_family != AF_INET && address->sa_family != AF_INET6)) {
                spdlog::debug("[UDP] destination is not an address, dropping datagram");
                continue;
            }

            const uint32_t interfaceIndex = targetInterface ? nw_interface_get_index(targetInterface) : 0;
            NSString *key = [NSString stringWithFormat:@"%u|%d", interfaceIndex, address->sa_family];

            // Reuse the flow's socket for this interface and family, or make one.  A new socket is not
            // created once the proxy has stopped, so none outlives cleanup.
            FlowUDPSocket *socket = nil;
            BOOL stopped = NO;
            int createError = 0;
            @synchronized (self) {
                stopped = stopped_;
                if (!stopped) {
                    NSMutableDictionary<NSString *, id> *sockets = [flowSockets_ objectForKey:flow];
                    socket = sockets[key];
                    if (!socket) {
                        std::unique_ptr<UdpSocket> created = UdpSocket::create(address->sa_family, interfaceIndex, createError);
                        if (created) {
                            socket = [[FlowUDPSocket alloc] init];
                            socket->socket = std::move(created);
                            if (!sockets) {
                                sockets = [NSMutableDictionary dictionary];
                                [flowSockets_ setObject:sockets forKey:flow];
                            }
                            sockets[key] = socket;
                            [self startReadingSocket:socket forFlow:flow];
                        }
                    }
                }
            }
            if (stopped) {
                // Proxy stopped (checked under the lock): close the flow outside the lock so its read
                // loop is not re-armed.
                [self cleanupFlow:flow withError:nil];
                return;
            }
            if (!socket) {
                spdlog::error("[UDP] failed to create socket ({}), dropping datagram", createError);
                continue;
            }

            spdlog::debug("[UDP] sending {} bytes to {} on {}", datagram.length, (nw_endpoint_get_hostname(endpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(endpoint), targetInterface ? nw_interface_get_name(targetInterface) : "<null>");

            const int sendError = socket->socket->sendTo(datagram.bytes, datagram.length, address, address->sa_len);
            if (sendError != 0) {
                spdlog::debug("[UDP] send error ({}), dropping datagram", sendError);
            }
        }

        [self handleUDPOutboundFlow:flow interface:interface onTunnel:onTunnel];
    }];
}

- (void)cleanup {
    // Block new sockets, then close each flow once; cleanupFlow closes that flow's sockets.
    NSArray<id<SplitTunnelUDPFlow>> *flows = nil;
    @synchronized (self) {
        stopped_ = YES;
        flows = [[flowSockets_ keyEnumerator] allObjects];
    }
    for (id<SplitTunnelUDPFlow> flow in flows) {
        [self cleanupFlow:flow withError:nil];
    }
}

@end
