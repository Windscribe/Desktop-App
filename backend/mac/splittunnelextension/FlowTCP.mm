#import "FlowTCP.h"

#include <spdlog/spdlog.h>
#import "Utils.h"

@implementation FlowTCP

- (instancetype)init {
    self = [super init];
    if (self) {
        activeConnections_ = [[NSMutableDictionary alloc] init];
    }
    return self;
}

- (BOOL)setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface {
    nw_endpoint_t endpoint = [Utils convertToNewEndpoint:flow.remoteEndpoint];
    if (!endpoint) {
        spdlog::error("[TCP] No endpoint found, cleaning up flow");
        [self cleanupFlow:flow withError:nil andConnection:nil];
        return NO;
    }

    // If it's a LAN range (except the reserved 10.255.255.0/24 range), we leave the traffic on the original interface.
    // Note that the firewall may still block this later.
    if ([Utils isLanRange:endpoint]) {
        spdlog::info("[TCP] Ignoring LAN traffic");
        return NO;
    }

    nw_parameters_t parameters = nw_parameters_create_secure_tcp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    nw_parameters_require_interface(parameters, interface);
    nw_connection_t connection = nw_connection_create(endpoint, parameters);
    nw_connection_set_queue(connection, dispatch_get_main_queue());

    // Set up connection state handler before storing or starting
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t  _Nullable error) {
        if (error) {
            spdlog::error("[TCP] Connection error: code={}", nw_error_get_error_code(error));
            [self cleanupFlow:flow withError:[Utils errorFromNWError:error] andConnection:connection];
        } else if (state == nw_connection_state_cancelled || state == nw_connection_state_failed) {
            spdlog::info("[TCP] Connection state changed to {}", state == nw_connection_state_cancelled ? "cancelled" : "failed");
            [self cleanupFlow:flow withError:nil andConnection:connection];
        } else if (state == nw_connection_state_ready) {
            spdlog::debug("[TCP] Connection established successfully");
            nw_path_t path = nw_connection_copy_current_path(connection);
            NWEndpoint *localEndpoint = [Utils convertToOldEndpoint:nw_path_copy_effective_local_endpoint(path)];

            [flow openWithLocalEndpoint:(NWHostEndpoint *)localEndpoint completionHandler:^(NSError * _Nullable error) {
                if (error) {
                    spdlog::error("[TCP] Failed to open flow: {}", [[error localizedDescription] UTF8String]);
                    [self cleanupFlow:flow withError:error andConnection:nil];
                    return;
                }
                spdlog::debug("[TCP] Flow opened successfully");
                [self handleTCPOutboundFlow:flow interface:interface];
                [self handleTCPInboundFlow:flow interface:interface];
            }];
        }
    });

    // Store and start the connection
    [activeConnections_ setObject:(id)connection forKey:[NSValue valueWithPointer:(__bridge void *)flow]];
    nw_connection_start(connection);
    return YES;
}

- (void)handleTCPOutboundFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface {
    [flow readDataWithCompletionHandler:^(NSData * _Nullable data, NSError * _Nullable error) {
        if (error) {
            spdlog::error("[TCP] Read error: {} ({})", [[error localizedDescription] UTF8String], error.code);
            [self cleanupFlow:flow withError:error andConnection:[activeConnections_ objectForKey:[NSValue valueWithPointer:(__bridge void *)flow]]];
            return;
        }

        if (!data) {
            spdlog::debug("[TCP] No outbound data received, retrying");
            [self handleTCPOutboundFlow:flow interface:interface];
            return;
        }

        if (data.length == 0) {
            spdlog::info("[TCP] Empty outbound data received, cleaning up flow");
            [self cleanupFlow:flow withError:nil andConnection:nil];
            return;
        }

        nw_connection_t connection = [activeConnections_ objectForKey:[NSValue valueWithPointer:(__bridge void *)flow]];
        if (!connection) {
            spdlog::error("[TCP] No connection found for flow");
            [self cleanupFlow:flow withError:nil andConnection:nil];
            return;
        }

        nw_endpoint_t endpoint = nw_connection_copy_endpoint(connection);
        spdlog::info("[TCP] sending {} bytes to {}", data.length, (nw_endpoint_get_hostname(endpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(endpoint));

        nw_connection_send(connection,
                            dispatch_data_create(data.bytes, data.length, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT),
                            NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT,
                            true,
                            ^(nw_error_t  _Nullable sendError) {
            if (sendError) {
                spdlog::error("[TCP] Send error: code={}", nw_error_get_error_code(sendError));
                [self cleanupFlow:flow withError:[Utils errorFromNWError:sendError] andConnection:connection];
            }
            [self handleTCPOutboundFlow:flow interface:interface];
        });
    }];
}

- (void)handleTCPInboundFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface {
    nw_connection_t connection = [activeConnections_ objectForKey:[NSValue valueWithPointer:(__bridge void *)flow]];
    if (!connection) {
        spdlog::error("[TCP] No connection found for flow");
        [self cleanupFlow:flow withError:nil andConnection:nil];
        return;
    }

    nw_connection_receive(connection, 1, UINT16_MAX,
        ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t receive_error) {
            if (receive_error) {
                spdlog::error("[TCP] Receive error: code={}", nw_error_get_error_code(receive_error));
                [self cleanupFlow:flow withError:[Utils errorFromNWError:receive_error] andConnection:connection];
                return;
            }

            if (!content) {
                spdlog::info("[TCP] No inbound data received, cleaning up flow");
                [self cleanupFlow:flow withError:nil andConnection:connection];
                return;
            }

            const void *buffer;
            size_t buffer_length;
            dispatch_data_t contiguous = dispatch_data_create_map(content, &buffer, &buffer_length);
            NSData *data = [NSData dataWithBytes:buffer length:buffer_length];

            nw_endpoint_t endpoint = nw_connection_copy_endpoint(connection);
            spdlog::debug("[TCP] received {} bytes from {}", data.length, (nw_endpoint_get_hostname(endpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(endpoint));

            [flow writeData:data withCompletionHandler:^(NSError * _Nullable error) {
                if (error) {
                    spdlog::error("[TCP] Write error: {}", [[error localizedDescription] UTF8String]);
                    [self cleanupFlow:flow withError:error andConnection:connection];
                    return;
                }
                if (is_complete) {
                    [self cleanupFlow:flow withError:nil andConnection:connection];
                } else {
                    // Continue receiving data
                    [self handleTCPInboundFlow:flow interface:interface];
                }
            }];
    });
}

- (void)cleanupFlow:(NEAppProxyTCPFlow *)flow withError:(nullable NSError *)error andConnection:(nullable nw_connection_t)connection {
    spdlog::debug("[TCP] Cleaning up flow and connection");
    if (connection) {
        nw_connection_cancel(connection);
        [activeConnections_ removeObjectForKey:[NSValue valueWithPointer:(__bridge void *)flow]];
    }
    [flow closeReadWithError:error];
    [flow closeWriteWithError:error];
}

- (void)cleanup {
    for (NSValue *key in [activeConnections_ allKeys]) {
        NEAppProxyTCPFlow *flow = (__bridge NEAppProxyTCPFlow *)[key pointerValue];
        nw_connection_t connection = [activeConnections_ objectForKey:key];
        [self cleanupFlow:flow withError:nil andConnection:connection];
    }
    [activeConnections_ removeAllObjects];
}

@end
