#import "FlowUDP.h"

#include <spdlog/spdlog.h>
#import "Utils.h"

@implementation FlowUDP

- (instancetype)init {
    self = [super init];
    if (self) {
        activeConnections_ = [[NSMutableDictionary alloc] init];
        settings_ = nil;
    }
    return self;
}

- (void)setSettings:(Settings *)settings {
    settings_ = settings;
}

- (void)removeConnection:(nw_connection_t)connection {
    NSValue *key = [NSValue valueWithPointer:(__bridge void *)connection];
    NEAppProxyUDPFlow *flow = activeConnections_[key];
    if (![activeConnections_ objectForKey:key]) {
        return;
    }
    nw_connection_cancel(connection);
    [activeConnections_ removeObjectForKey:key];
    spdlog::debug("[UDP] removed connection from active connections list");

    // If there are no more connections for this flow, close the flow
    bool found = false;
    for (NSValue *key in [activeConnections_ allKeys]) {
        NEAppProxyUDPFlow *connFlow = activeConnections_[key];
        if (connFlow == flow) {
            found = true;
            break;
        }
    }
    if (!found) {
        [flow closeReadWithError:nil];
        [flow closeWriteWithError:nil];
    }
}

- (void)cleanupFlow:(NEAppProxyUDPFlow *)flow withError:(NSError * _Nullable)error {
    // Remove all connections associated with this flow
    NSArray *keys = [activeConnections_ allKeys];
    for (NSValue *key in keys) {
        NEAppProxyUDPFlow *connFlow = activeConnections_[key];
        if (connFlow == flow) {
            nw_connection_t conn = (nw_connection_t)key.pointerValue;
            [self removeConnection:conn];
        }
    }
    [flow closeReadWithError:error];
    [flow closeWriteWithError:error];
}

- (BOOL)setupUDPConnection:(NEAppProxyUDPFlow *)flow interface:(nw_interface_t)interface {
    if (!settings_) {
        spdlog::error("[UDP] Settings not initialized");
        return NO;
    }

    [flow openWithLocalEndpoint:(NWHostEndpoint *)flow.localEndpoint completionHandler:^(NSError * _Nullable error) {
        if (error) {
            spdlog::error("[UDP] flow open error: {}", [[error localizedDescription] UTF8String]);
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

        // Forward each datagram through the target interface
        for (NSUInteger i = 0; i < datagrams.count; i++) {
            NSData *datagram = datagrams[i];

            // Create and configure parameters for secure UDP connection
            nw_parameters_t parameters = nw_parameters_create_secure_udp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
            nw_endpoint_t endpoint = [Utils convertToNewEndpoint:endpoints[i]];
            if (!endpoint) {
                spdlog::error("[UDP] failed to create endpoint");
                [self cleanupFlow:flow withError:nil];
                return;
            }

            nw_interface_t targetInterface = interface;

            // DNS traffic should stay on the original (VPN) interface.  ROBERT can only be reached from the VPN interface.
            // If it's a LAN range (including the reserved 10.255.255.0/24 range), we leave the traffic on the original interface.
            // Note that the firewall may still block this later.
            if (nw_endpoint_get_port(endpoint) == 53 || [Utils isLanRange:endpoint] || ![settings_ isSplitTunnelApplicable:flow remoteEndpoint:endpoint]) {
                targetInterface = flow.networkInterface;
            }

            nw_parameters_require_interface(parameters, targetInterface);

            // Check for existing connection to the same endpoint and flow
            nw_connection_t existingConnection = nil;
            for (NSValue *key in activeConnections_) {
                nw_connection_t conn = (nw_connection_t)key.pointerValue;
                NEAppProxyUDPFlow *connFlow = activeConnections_[key];

                if (connFlow == flow) {
                    nw_endpoint_t connEndpoint = nw_connection_copy_endpoint(conn);
                    if ([Utils isSameEndpoint:connEndpoint endpoint:endpoint]) {
                        existingConnection = conn;
                        break;
                    }
                }
            }

            nw_connection_t connection;
            if (existingConnection) {
                connection = existingConnection;
            } else {
                connection = nw_connection_create(endpoint, parameters);
                nw_connection_set_queue(connection, dispatch_get_main_queue());
                [activeConnections_ setObject:flow forKey:[NSValue valueWithPointer:(__bridge void *)connection]];
                nw_connection_start(connection); // UDP is connectionless but this is needed for the inbound handler
                [self handleUDPInboundFlow:connection flow:flow interface:interface];
            }

            spdlog::debug("[UDP] sending {} bytes to {} on {}", datagram.length, (nw_endpoint_get_hostname(endpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(endpoint), nw_interface_get_name(targetInterface));

            nw_connection_send(
                connection,
                dispatch_data_create(datagram.bytes, datagram.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT),
                NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT,
                true,
                ^(nw_error_t  _Nullable sendError) {
                    if (sendError) {
                        spdlog::error("[UDP] write error");
                        [self cleanupFlow:flow withError:[Utils errorFromNWError:sendError]];
                        return;
                    }
                });
        }

        [self handleUDPOutboundFlow:flow interface:interface];
    }];
}

- (void)handleUDPInboundFlow:(nw_connection_t)connection
                     flow:(NEAppProxyUDPFlow *)flow
                interface:(nw_interface_t)interface {
    nw_connection_receive_message(connection,
        ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t receive_error) {
            if (receive_error) {
                spdlog::error("[UDP] Receive error: code={}", nw_error_get_error_code(receive_error));
                [self cleanupFlow:flow withError:[Utils errorFromNWError:receive_error]];
                return;
            }

            if (!content) {
                spdlog::info("[UDP] No inbound data received, cleaning up flow");
                [self cleanupFlow:flow withError:nil];
                return;
            }

            const void *buffer;
            size_t buffer_length;
            dispatch_data_t contiguous = dispatch_data_create_map(content, &buffer, &buffer_length);
            NSData *data = [NSData dataWithBytes:buffer length:buffer_length];

            // Get the remote endpoint from the connection for writing back
            nw_endpoint_t remoteEndpoint = nw_connection_copy_endpoint(connection);
            NWEndpoint *endpoint = [Utils convertToOldEndpoint:remoteEndpoint];

            if (!endpoint) {
                spdlog::error("[UDP] failed to create endpoint");
                [self cleanupFlow:flow withError:nil];
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
                [self handleUDPInboundFlow:connection flow:flow interface:interface];
            }];
        });
}

- (void)cleanup {
    for (NSValue *key in [activeConnections_ allKeys]) {
        nw_connection_t connection = (nw_connection_t)key.pointerValue;
        NEAppProxyUDPFlow *flow = activeConnections_[key];
        [self removeConnection:connection];
    }
    [activeConnections_ removeAllObjects];
}

@end
