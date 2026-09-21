#import "FlowTCP.h"

#include <spdlog/spdlog.h>
#import "Utils.h"
#import "ip_hostnames/ip_hostnames_manager.h"

@implementation FlowTCP

- (instancetype)init {
    self = [super init];
    if (self) {
        activeConnections_ = [[NSMutableDictionary alloc] init];
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

- (BOOL)setupTCPConnection:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface {
    Settings *settings;
    BOOL stopped;
    @synchronized (self) {
        settings = settings_;
        stopped = stopped_;
    }
    if (!settings) {
        spdlog::error("[TCP] Settings not initialized");
        return NO;
    }
    // Decline before creating an nw_connection, same as UDP: once the proxy has stopped the system
    // should keep routing the flow normally instead of claiming it and tearing it down.  Not an error
    // level: the system keeps handing us flows all through a teardown, and error is the release log
    // level, so one disconnect under load would bury a log that keeps 2 MB plus a single backup.
    if (stopped) {
        spdlog::debug("[TCP] Proxy stopped, declining flow");
        return NO;
    }

    nw_endpoint_t endpoint = [Utils convertToNewEndpoint:flow.remoteEndpoint];
    if (!endpoint) {
        spdlog::error("[TCP] No endpoint found, cleaning up flow");
        [self cleanupFlow:flow withError:nil];
        return NO;
    }

    // If it's inclusive mode and the endpoint is in a LAN range (including the reserved 10.255.255.0/24 range),
    // we leave the traffic on the original interface.  The firewall may block this depending on the Allow LAN traffic setting.
    if (![settings isExclude] && [Utils isLanRange:endpoint]) {
        spdlog::debug("[TCP] Ignoring LAN traffic");
        return NO;
    }

    // Check if split tunnel applies to this flow and endpoint
    if (![settings isSplitTunnelApplicable:flow remoteEndpoint:endpoint]) {
        spdlog::debug("[TCP] Flow not in app list or hostname list: {}", nw_endpoint_get_hostname(endpoint));
        return NO;
    }

    spdlog::info("[TCP] Handling flow from {} => {}", [flow.metaData.sourceAppSigningIdentifier UTF8String], nw_interface_get_name(interface));

    nw_parameters_t parameters = nw_parameters_create_secure_tcp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    nw_parameters_require_interface(parameters, interface);
    nw_connection_t connection = nw_connection_create(endpoint, parameters);
    nw_connection_set_queue(connection, dispatch_get_main_queue());

    // Set up connection state handler before storing or starting
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t  _Nullable error) {
        if (error) {
            spdlog::error("[TCP] Connection error: code={}", nw_error_get_error_code(error));
            [self cleanupFlow:flow withError:[Utils errorFromNWError:error]];
        } else if (state == nw_connection_state_cancelled || state == nw_connection_state_failed) {
            spdlog::info("[TCP] Connection state changed to {}", state == nw_connection_state_cancelled ? "cancelled" : "failed");
            [self cleanupFlow:flow withError:nil];
        } else if (state == nw_connection_state_ready) {
            spdlog::debug("[TCP] Connection established successfully");
            nw_path_t path = nw_connection_copy_current_path(connection);
            nw_endpoint_t localNwEndpoint = path ? nw_path_copy_effective_local_endpoint(path) : NULL;
            NWEndpoint *localEndpoint = localNwEndpoint ? [Utils convertToOldEndpoint:localNwEndpoint] : nil;

            [flow openWithLocalEndpoint:(NWHostEndpoint *)localEndpoint completionHandler:^(NSError * _Nullable error) {
                if (error) {
                    spdlog::error("[TCP] Failed to open flow: {}", [[error localizedDescription] UTF8String]);
                    [self cleanupFlow:flow withError:error];
                    return;
                }
                spdlog::debug("[TCP] Flow opened successfully");
                [self handleTCPOutboundFlow:flow interface:interface];
                [self handleTCPInboundFlow:flow interface:interface];
            }];
        }
    });

    // Store and start the connection.  Drop it if the proxy stopped while we were setting up, so no
    // connection outlives cleanup.
    @synchronized (self) {
        if (stopped_) {
            // Drop the state handler before cancelling.  It closes the flow, and this flow is being
            // declined (return NO), so it goes back to a system that has already begun its startup --
            // closing it in that state is what crashed inside NetworkExtension's own flow-startup block.
            // The connection has not been started yet, so the handler can still be replaced here.
            nw_connection_set_state_changed_handler(connection, nil);
            nw_connection_cancel(connection);
            return NO;
        }
        [activeConnections_ setObject:(id)connection forKey:[NSValue valueWithPointer:(__bridge void *)flow]];
        nw_connection_start(connection);
    }
    return YES;
}

- (void)handleTCPOutboundFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface {
    [flow readDataWithCompletionHandler:^(NSData * _Nullable data, NSError * _Nullable error) {
        if (error) {
            spdlog::error("[TCP] Read error: {} ({})", [[error localizedDescription] UTF8String], error.code);
            [self cleanupFlow:flow withError:error];
            return;
        }

        if (!data) {
            spdlog::debug("[TCP] No outbound data received, retrying");
            [self handleTCPOutboundFlow:flow interface:interface];
            return;
        }

        if (data.length == 0) {
            spdlog::info("[TCP] Empty outbound data received, cleaning up flow");
            [self cleanupFlow:flow withError:nil];
            return;
        }

        nw_connection_t connection = nil;
        @synchronized (self) {
            connection = [activeConnections_ objectForKey:[NSValue valueWithPointer:(__bridge void *)flow]];
        }
        if (!connection) {
            spdlog::error("[TCP] No connection found for flow");
            [self cleanupFlow:flow withError:nil];
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
                [self cleanupFlow:flow withError:[Utils errorFromNWError:sendError]];
                return;
            }
            [self handleTCPOutboundFlow:flow interface:interface];
        });
    }];
}

- (void)handleTCPInboundFlow:(NEAppProxyTCPFlow *)flow interface:(nw_interface_t)interface {
    nw_connection_t connection = nil;
    @synchronized (self) {
        connection = [activeConnections_ objectForKey:[NSValue valueWithPointer:(__bridge void *)flow]];
    }
    if (!connection) {
        spdlog::error("[TCP] No connection found for flow");
        [self cleanupFlow:flow withError:nil];
        return;
    }

    nw_connection_receive(connection, 1, UINT16_MAX,
        ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t receive_error) {
            if (receive_error) {
                spdlog::error("[TCP] Receive error: code={}", nw_error_get_error_code(receive_error));
                [self cleanupFlow:flow withError:[Utils errorFromNWError:receive_error]];
                return;
            }

            if (!content) {
                spdlog::info("[TCP] No inbound data received, cleaning up flow");
                [self cleanupFlow:flow withError:nil];
                return;
            }

            const void *buffer;
            size_t buffer_length;
            dispatch_data_t __unused contiguous = dispatch_data_create_map(content, &buffer, &buffer_length);
            NSData *data = [NSData dataWithBytes:buffer length:buffer_length];

            nw_endpoint_t endpoint = nw_connection_copy_endpoint(connection);
            spdlog::debug("[TCP] received {} bytes from {}", data.length, (nw_endpoint_get_hostname(endpoint) == NULL) ? "<null>" : nw_endpoint_get_hostname(endpoint));

            [flow writeData:data withCompletionHandler:^(NSError * _Nullable error) {
                if (error) {
                    spdlog::error("[TCP] Write error: {}", [[error localizedDescription] UTF8String]);
                    [self cleanupFlow:flow withError:error];
                    return;
                }
                if (is_complete) {
                    [self cleanupFlow:flow withError:nil];
                } else {
                    // Continue receiving data
                    [self handleTCPInboundFlow:flow interface:interface];
                }
            }];
    });
}

- (void)cleanupFlow:(NEAppProxyTCPFlow *)flow withError:(nullable NSError *)error {
    spdlog::debug("[TCP] Cleaning up flow and connection");
    // Derive the flow's connection so removal is never skipped by a caller lacking one.
    @synchronized (self) {
        NSValue *key = [NSValue valueWithPointer:(__bridge void *)flow];
        nw_connection_t connection = [activeConnections_ objectForKey:key];
        if (connection) {
            nw_connection_cancel(connection);
            [activeConnections_ removeObjectForKey:key];
        }
    }
    [flow closeReadWithError:error];
    [flow closeWriteWithError:error];
}

- (void)cleanup {
    // Block new connections, then tear down whatever is live.
    NSMutableArray<NEAppProxyTCPFlow *> *flows = [NSMutableArray array];
    @synchronized (self) {
        stopped_ = YES;
        // Retain flows before callbacks can remove their connections and release them.
        for (NSValue *key in activeConnections_) {
            [flows addObject:(__bridge NEAppProxyTCPFlow *)[key pointerValue]];
        }
    }
    for (NEAppProxyTCPFlow *flow in flows) {
        [self cleanupFlow:flow withError:nil];
    }
}

@end
