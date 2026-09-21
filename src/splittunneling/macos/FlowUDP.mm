#import "FlowUDP.h"

#include <spdlog/spdlog.h>
#import "Utils.h"

@implementation FlowUDP

- (instancetype)init {
    self = [super init];
    if (self) {
        flowConnections_ = [NSMapTable mapTableWithKeyOptions:NSPointerFunctionsObjectPointerPersonality | NSPointerFunctionsStrongMemory
                                                 valueOptions:NSPointerFunctionsObjectPersonality | NSPointerFunctionsStrongMemory];
        settings_ = nil;
    }
    return self;
}

- (void)setSettings:(Settings *)settings {
    // settings_ is published here (start queue) and read on the flow queues, so guard it with the same
    // lock.  Re-arm for a fresh session: a new proxy start clears the stop that disabled connections.
    @synchronized (self) {
        settings_ = settings;
        stopped_ = NO;
    }
}

// What identifies a connection within a flow: the destination it talks to -- the same hostname and
// port Utils.isSameEndpoint compared -- plus the interface it is bound to, because the same
// destination can need a different interface after a live settings update, and a connection bound to
// the wrong one must never be reused.
- (NSString * _Nullable)keyForEndpoint:(nw_endpoint_t)endpoint interface:(nw_interface_t _Nullable)interface {
    if (!endpoint) {
        return nil;
    }
    const char *host = nw_endpoint_get_hostname(endpoint);
    const char *ifname = interface ? nw_interface_get_name(interface) : NULL;
    // Enough for the longest hostname the resolver can produce, plus ":65535|" and an interface name.
    char buf[1088];
    snprintf(buf, sizeof(buf), "%s:%u|%s", host ? host : "", nw_endpoint_get_port(endpoint), ifname ? ifname : "");
    return [[NSString alloc] initWithUTF8String:buf];
}

- (void)removeConnection:(nw_connection_t)connection key:(NSString *)key forFlow:(NEAppProxyUDPFlow *)flow {
    if (!connection || !key || !flow) {
        return;
    }
    // Drop the connection only; the flow is closed once by cleanupFlow, its sole owner.  Idempotent:
    // cancelling completes the pending receive, whose handler lands here again.
    @synchronized (self) {
        NSMutableDictionary<NSString *, id> *connections = [flowConnections_ objectForKey:flow];
        if (connections[key] != connection) {
            // Already gone, or a newer connection to the same destination has taken its place.
            return;
        }
        [connections removeObjectForKey:key];
        if (connections.count == 0) {
            // A flow with no connections is not tracked, exactly as before.
            [flowConnections_ removeObjectForKey:flow];
        }
    }
    // Cancelling outside the lock: it completes the pending receive, and that handler takes this same
    // lock on its way back in here.
    nw_connection_cancel(connection);
    spdlog::debug("[UDP] removed connection to {}", [key UTF8String]);
}

- (void)cleanupFlow:(NEAppProxyUDPFlow *)flow withError:(NSError * _Nullable)error {
    // Take the flow's connections out first, so nothing can be handed out while they are cancelled.
    NSArray *connections = nil;
    @synchronized (self) {
        connections = [[flowConnections_ objectForKey:flow] allValues];
        [flowConnections_ removeObjectForKey:flow];
    }
    for (id connection in connections) {
        nw_connection_cancel((nw_connection_t)connection);
    }
    [flow closeReadWithError:error];
    [flow closeWriteWithError:error];
}

- (BOOL)setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface {
    Settings *settings;
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
        // Outbound flows that create connections will start the inbound handler
        [self handleUDPOutboundFlow:flow interface:interface];
    }];

    // For UDP, we don't know what the remote endpoint is until we process the datagrams, so we always accept the flow.
    spdlog::info("[UDP] Handling flow from {} => {}", [flow.metaData.sourceAppSigningIdentifier UTF8String], nw_interface_get_name(interface));
    return YES;
}

- (void)handleUDPOutboundFlow:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface {
    [flow readDatagramsWithCompletionHandler:^(NSArray<NSData *> * _Nullable datagrams, NSArray<NWEndpoint *> * _Nullable endpoints, NSError * _Nullable error) {
        if (error) {
            spdlog::error("[UDP] Read error: {} ({})", [[error localizedDescription] UTF8String], error.code);
            [self cleanupFlow:flow withError:error];
            return;
        }

        if (!datagrams) {
            spdlog::debug("[UDP] No outbound data received, retrying");
            [self handleUDPOutboundFlow:flow interface:interface];
            return;
        }

        if (datagrams.count == 0) {
            spdlog::info("[UDP] Empty outbound data received, cleaning up flow");
            [self cleanupFlow:flow withError:nil];
            return;
        }

        Settings *settings;
        @synchronized (self) {
            settings = settings_;
        }

        // Forward each datagram through the target interface
        for (NSUInteger i = 0; i < datagrams.count; i++) {
            NSData *datagram = datagrams[i];

            // Create and configure parameters for secure UDP connection
            nw_parameters_t parameters = nw_parameters_create_secure_udp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
            nw_endpoint_t endpoint = [Utils convertToNewEndpoint:endpoints[i]];
            if (!endpoint) {
                // One unusable destination is not a reason to close the socket the app is using for
                // every other destination: drop this datagram and carry on.
                spdlog::error("[UDP] failed to create endpoint, dropping datagram");
                continue;
            }

            nw_interface_t targetInterface = interface;

            // DNS traffic should stay on the original (VPN) interface.  ROBERT can only be reached from the VPN interface.
            // If it's a LAN range (including the reserved 10.255.255.0/24 range), we leave the traffic on the original interface.
            // Note that the firewall may still block this later.
            if (nw_endpoint_get_port(endpoint) == 53 || [Utils isLanRange:endpoint] || ![settings isSplitTunnelApplicable:flow remoteEndpoint:endpoint]) {
                targetInterface = flow.networkInterface;
            }

            nw_parameters_require_interface(parameters, targetInterface);

            NSString *key = [self keyForEndpoint:endpoint interface:targetInterface];
            if (!key) {
                spdlog::error("[UDP] failed to build a connection key, dropping datagram");
                continue;
            }

            // Reuse the connection to this destination, or make one.  A new connection is not created
            // once the proxy has stopped, so none outlives cleanup.
            nw_connection_t connection = nil;
            @synchronized (self) {
                NSMutableDictionary<NSString *, id> *connections = [flowConnections_ objectForKey:flow];
                connection = connections[key];
                if (!connection && !stopped_) {
                    connection = nw_connection_create(endpoint, parameters);
                    nw_connection_set_queue(connection, dispatch_get_main_queue());
                    if (!connections) {
                        connections = [NSMutableDictionary dictionary];
                        [flowConnections_ setObject:connections forKey:flow];
                    }
                    connections[key] = connection;
                    nw_connection_start(connection); // UDP is connectionless but this is needed for the inbound handler
                    [self handleUDPInboundFlow:connection key:key flow:flow interface:interface];
                }
            }
            if (!connection) {
                // Proxy stopped (checked under the lock): close the flow outside the lock so its read
                // loop is not re-armed.
                [self cleanupFlow:flow withError:nil];
                return;
            }

            spdlog::debug("[UDP] sending {} bytes to {} on {}", datagram.length, (nw_endpoint_get_hostname(endpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(endpoint), targetInterface ? nw_interface_get_name(targetInterface) : "<null>");

            nw_connection_send(
                connection,
                dispatch_data_create(datagram.bytes, datagram.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT),
                NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT,
                true,
                ^(nw_error_t  _Nullable sendError) {
                    if (sendError) {
                        // The send failed for this destination only.  Drop its connection so the next
                        // datagram to it starts a fresh one, and leave the flow's other destinations
                        // (and the flow itself) alone.
                        spdlog::error("[UDP] write error, dropping connection");
                        [self removeConnection:connection key:key forFlow:flow];
                        return;
                    }
                });
        }

        [self handleUDPOutboundFlow:flow interface:interface];
    }];
}

- (void)handleUDPInboundFlow:(nw_connection_t)connection
                         key:(NSString *)key
                        flow:(NEAppProxyUDPFlow *)flow
                   interface:(nw_interface_t)interface {
    nw_connection_receive_message(connection,
        ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t receive_error) {
            // Everything below is per connection: a UDP flow is one socket talking to many
            // destinations, so a destination that errors, goes quiet or is cancelled must cost the app
            // that destination and nothing else.  Closing the flow here used to take every other
            // destination with it, and it also made cancelling a single connection impossible.
            if (receive_error) {
                spdlog::error("[UDP] Receive error: code={}, dropping connection", nw_error_get_error_code(receive_error));
                [self removeConnection:connection key:key forFlow:flow];
                return;
            }

            if (!content) {
                spdlog::debug("[UDP] connection ended, removing it");
                [self removeConnection:connection key:key forFlow:flow];
                return;
            }

            const void *buffer;
            size_t buffer_length;
            dispatch_data_t __unused contiguous = dispatch_data_create_map(content, &buffer, &buffer_length);
            NSData *data = [NSData dataWithBytes:buffer length:buffer_length];

            // Get the remote endpoint from the connection for writing back
            nw_endpoint_t remoteEndpoint = nw_connection_copy_endpoint(connection);
            NWEndpoint *endpoint = [Utils convertToOldEndpoint:remoteEndpoint];

            if (!endpoint) {
                spdlog::error("[UDP] failed to create endpoint, dropping connection");
                [self removeConnection:connection key:key forFlow:flow];
                return;
            }

            spdlog::debug("[UDP] received {} bytes from {}", data.length, (nw_endpoint_get_hostname(remoteEndpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(remoteEndpoint));

            [flow writeDatagrams:@[data] sentByEndpoints:@[endpoint] completionHandler:^(NSError * _Nullable error) {
                if (error) {
                    spdlog::error("[UDP] write error to flow: {}", [[error localizedDescription] UTF8String]);
                    [self cleanupFlow:flow withError:error];
                    return;
                }

                // Continue receiving on this connection
                [self handleUDPInboundFlow:connection key:key flow:flow interface:interface];
            }];
        });
}

- (void)cleanup {
    // Block new connections, then close each flow once; cleanupFlow cancels that flow's connections.
    NSArray<NEAppProxyUDPFlow *> *flows = nil;
    @synchronized (self) {
        stopped_ = YES;
        flows = [[flowConnections_ keyEnumerator] allObjects];
    }
    for (NEAppProxyUDPFlow *flow in flows) {
        [self cleanupFlow:flow withError:nil];
    }
}

@end
