#include "legacy_protobuf.h"

extern "C" {
    #include "legacy_protobuf_support/types.pb-c.h"
}


bool LegacyProtobufSupport::loadGuiSettings(const QByteArray &arr, types::GuiSettings &out)
{
    ProtoTypes__GuiSettings *g = proto_types__gui_settings__unpack(NULL, arr.size(), (const uint8_t *)arr.data());
    if (g)
    {
        if (g->has_is_launch_on_startup) out.isLaunchOnStartup = g->is_launch_on_startup;
        if (g->has_is_auto_connect) out.isAutoConnect = g->is_auto_connect;
        if (g->has_is_hide_from_dock) out.isHideFromDock = g->is_hide_from_dock;
        if (g->has_is_show_notifications) out.isShowNotifications = g->is_show_notifications;
        if (g->has_order_location) out.orderLocation = (ORDER_LOCATION_TYPE)g->order_location;
        if (g->has_latency_display) out.latencyDisplay = (LATENCY_DISPLAY_TYPE)g->latency_display;
        if (g->has_is_docked_to_tray) out.isDockedToTray = g->is_docked_to_tray;
        if (g->has_is_minimize_and_close_to_tray) out.isMinimizeAndCloseToTray = g->is_minimize_and_close_to_tray;
        if (g->has_is_start_minimized) out.isStartMinimized = g->is_start_minimized;
        if (g->has_is_show_location_health) out.isShowLocationHealth = g->is_show_location_health;

        if (g->share_secure_hotspot)
        {
            ProtoTypes__ShareSecureHotspot *ss = g->share_secure_hotspot;
            if (ss->has_is_enabled) out.shareSecureHotspot.isEnabled = ss->is_enabled;
            out.shareSecureHotspot.ssid = ss->ssid;
            out.shareSecureHotspot.password = ss->password;
        }

        if (g->share_proxy_gateway)
        {
            ProtoTypes__ShareProxyGateway *sp = g->share_proxy_gateway;
            if (sp->has_is_enabled) out.shareProxyGateway.isEnabled = sp->is_enabled;
            if (sp->has_proxy_sharing_mode) out.shareProxyGateway.proxySharingMode = (PROXY_SHARING_TYPE)sp->proxy_sharing_mode;
        }

        if (g->split_tunneling)
        {
            ProtoTypes__SplitTunneling *st = g->split_tunneling;
            if (st->settings)
            {
                ProtoTypes__SplitTunnelingSettings *sts = st->settings;
                if (sts->has_active) out.splitTunneling.settings.active = sts->active;
                if (sts->has_mode) out.splitTunneling.settings.mode = (SPLIT_TUNNELING_MODE)sts->mode;
            }

            out.splitTunneling.apps.clear();
            for (size_t i = 0; i < st->n_apps; i++)
            {
                ProtoTypes__SplitTunnelingApp *a = st->apps[i];
                types::SplitTunnelingApp app;
                if (a->has_type) app.type = (SPLIT_TUNNELING_APP_TYPE)a->type;
                app.name = a->name;
                app.fullName = a->full_name;
                if (a->has_active) app.active = a->active;
                out.splitTunneling.apps << app;
            }

            out.splitTunneling.networkRoutes.clear();
            for (size_t i = 0; i < st->n_network_routes; i++)
            {
                ProtoTypes__SplitTunnelingNetworkRoute *r = st->network_routes[i];
                types::SplitTunnelingNetworkRoute route;
                if (r->has_type) route.type = (SPLIT_TUNNELING_NETWORK_ROUTE_TYPE)r->type;
                route.name = r->name;
                out.splitTunneling.networkRoutes << route;
            }
        }
        if (g->background_settings)
        {
            ProtoTypes__BackgroundSettings *b = g->background_settings;
            if (b->has_background_type) out.backgroundSettings.backgroundType = (BACKGROUND_TYPE)b->background_type;
            out.backgroundSettings.backgroundImageDisconnected = b->background_image_disconnected;
            out.backgroundSettings.backgroundImageConnected = b->background_image_connected;
        }
        proto_types__gui_settings__free_unpacked(g, NULL);
        return true;
    }
    return false;
}

bool LegacyProtobufSupport::loadEngineSettings(const QByteArray &arr, types::EngineSettings &out)
{
    ProtoTypes__EngineSettings *es = proto_types__engine_settings__unpack(NULL, arr.size(), (const uint8_t *)arr.data());
    if (!es) return false;

    out.d->language = es->language;
    if (es->has_update_channel) out.d->updateChannel = (UPDATE_CHANNEL)es->update_channel;
    if (es->has_is_ignore_ssl_errors) out.d->isIgnoreSslErrors = es->is_ignore_ssl_errors;
    if (es->has_is_close_tcp_sockets) out.d->isTerminateSockets = es->is_close_tcp_sockets;
    if (es->has_is_allow_lan_traffic) out.d->isAllowLanTraffic = es->is_allow_lan_traffic;

    if (es->firewall_settings)
    {
        if (es->firewall_settings->has_mode) out.d->firewallSettings.mode = (FIREWALL_MODE)es->firewall_settings->mode;
        if (es->firewall_settings->has_when)  out.d->firewallSettings.when = (FIREWALL_WHEN)es->firewall_settings->when;
    }

    if (es->connection_settings)
    {
        if (es->connection_settings->has_is_automatic) out.d->connectionSettings.isAutomatic = es->connection_settings->is_automatic;
        if (es->connection_settings->has_port) out.d->connectionSettings.port = es->connection_settings->port;
        if (es->connection_settings->has_protocol) out.d->connectionSettings.protocol = (PROTOCOL)es->connection_settings->protocol;
    }

    if (es->api_resolution)
    {
        if (es->api_resolution->has_is_automatic) out.d->dnsResolutionSettings.setIsAutomatic(es->api_resolution->is_automatic);
        out.d->dnsResolutionSettings.set(es->api_resolution->is_automatic, es->api_resolution->manual_ip);
    }

    if (es->proxy_settings)
    {
        out.d->proxySettings.setAddress(es->proxy_settings->address);
        out.d->proxySettings.setUsername(es->proxy_settings->username);
        out.d->proxySettings.setPassword(es->proxy_settings->password);
        if (es->proxy_settings->has_port) out.d->proxySettings.setPort(es->proxy_settings->port);
        if (es->proxy_settings->has_proxy_option) out.d->proxySettings.setOption((PROXY_OPTION)es->proxy_settings->proxy_option);
    }

    if (es->packet_size)
    {
        if (es->packet_size->has_is_automatic) out.d->packetSize.isAutomatic = es->packet_size->is_automatic;
        if (es->packet_size->has_mtu) out.d->packetSize.mtu = es->packet_size->mtu;
    }

    if (es->mac_addr_spoofing)
    {
        if (es->mac_addr_spoofing->has_is_enabled) out.d->macAddrSpoofing.isEnabled = es->mac_addr_spoofing->is_enabled;
        out.d->macAddrSpoofing.macAddress = es->mac_addr_spoofing->mac_address;
        if (es->mac_addr_spoofing->has_is_auto_rotate) out.d->macAddrSpoofing.isAutoRotate = es->mac_addr_spoofing->is_auto_rotate;


        // NetworkInterface
        if (es->mac_addr_spoofing->selected_network_interface)
        {
            if (es->mac_addr_spoofing->selected_network_interface->has_interface_index) {
                out.d->macAddrSpoofing.selectedNetworkInterface.interfaceIndex = es->mac_addr_spoofing->selected_network_interface->interface_index;
            }
            out.d->macAddrSpoofing.selectedNetworkInterface.interfaceName = es->mac_addr_spoofing->selected_network_interface->interface_name;
            out.d->macAddrSpoofing.selectedNetworkInterface.interfaceGuid = es->mac_addr_spoofing->selected_network_interface->interface_guid;
            out.d->macAddrSpoofing.selectedNetworkInterface.networkOrSsid = es->mac_addr_spoofing->selected_network_interface->network_or_ssid;
            if (es->mac_addr_spoofing->selected_network_interface->has_interface_type) {
                out.d->macAddrSpoofing.selectedNetworkInterface.interfaceType = (NETWORK_INTERACE_TYPE)es->mac_addr_spoofing->selected_network_interface->interface_type;
            }
            if (es->mac_addr_spoofing->selected_network_interface->has_trust_type) {
                out.d->macAddrSpoofing.selectedNetworkInterface.trustType = (NETWORK_TRUST_TYPE)es->mac_addr_spoofing->selected_network_interface->trust_type;
            }
            if (es->mac_addr_spoofing->selected_network_interface->has_active) {
                out.d->macAddrSpoofing.selectedNetworkInterface.active = es->mac_addr_spoofing->selected_network_interface->active;
            }
            out.d->macAddrSpoofing.selectedNetworkInterface.friendlyName = es->mac_addr_spoofing->selected_network_interface->friendly_name;
            if (es->mac_addr_spoofing->selected_network_interface->has_requested) {
                out.d->macAddrSpoofing.selectedNetworkInterface.requested = es->mac_addr_spoofing->selected_network_interface->requested;
            }
            if (es->mac_addr_spoofing->selected_network_interface->has_metric) {
                out.d->macAddrSpoofing.selectedNetworkInterface.metric = es->mac_addr_spoofing->selected_network_interface->metric;
            }
            out.d->macAddrSpoofing.selectedNetworkInterface.physicalAddress = es->mac_addr_spoofing->selected_network_interface->physical_address;
            if (es->mac_addr_spoofing->selected_network_interface->has_mtu) {
                out.d->macAddrSpoofing.selectedNetworkInterface.mtu = es->mac_addr_spoofing->selected_network_interface->mtu;
            }
            if (es->mac_addr_spoofing->selected_network_interface->has_state) {
                out.d->macAddrSpoofing.selectedNetworkInterface.state = es->mac_addr_spoofing->selected_network_interface->state;
            }
            if (es->mac_addr_spoofing->selected_network_interface->has_dw_type) {
                out.d->macAddrSpoofing.selectedNetworkInterface.dwType = es->mac_addr_spoofing->selected_network_interface->dw_type;
            }
            out.d->macAddrSpoofing.selectedNetworkInterface.deviceName = es->mac_addr_spoofing->selected_network_interface->device_name;
            if (es->mac_addr_spoofing->selected_network_interface->has_connector_present) {
                out.d->macAddrSpoofing.selectedNetworkInterface.connectorPresent = es->mac_addr_spoofing->selected_network_interface->connector_present;
            }
            if (es->mac_addr_spoofing->selected_network_interface->has_end_point_interface) {
                out.d->macAddrSpoofing.selectedNetworkInterface.endPointInterface = es->mac_addr_spoofing->selected_network_interface->end_point_interface;
            }
        }

        if (es->mac_addr_spoofing->network_interfaces)
        {
            out.d->macAddrSpoofing.networkInterfaces.clear();
            for (size_t i = 0; i < es->mac_addr_spoofing->network_interfaces->n_networks; ++i)
            {
                types::NetworkInterface networkInterface;
                ProtoTypes__NetworkInterface *ni = es->mac_addr_spoofing->network_interfaces->networks[i];

                if (ni->has_interface_index) {
                    networkInterface.interfaceIndex = ni->interface_index;
                }
                networkInterface.interfaceName = ni->interface_name;
                networkInterface.interfaceGuid = ni->interface_guid;
                networkInterface.networkOrSsid = ni->network_or_ssid;
                if (ni->has_interface_type) {
                    networkInterface.interfaceType = (NETWORK_INTERACE_TYPE)ni->interface_type;
                }
                if (ni->has_trust_type) {
                    networkInterface.trustType = (NETWORK_TRUST_TYPE)ni->trust_type;
                }
                if (ni->has_active) {
                    networkInterface.active = ni->active;
                }
                networkInterface.friendlyName = ni->friendly_name;
                if (ni->has_requested) {
                    networkInterface.requested = ni->requested;
                }
                if (ni->has_metric) {
                    networkInterface.metric = ni->metric;
                }
                networkInterface.physicalAddress = ni->physical_address;
                if (ni->has_mtu) {
                    networkInterface.mtu = ni->mtu;
                }
                if (ni->has_state) {
                    networkInterface.state = ni->state;
                }
                if (ni->has_dw_type) {
                    networkInterface.dwType = ni->dw_type;
                }
                networkInterface.deviceName = ni->device_name;
                if (ni->has_connector_present) {
                    networkInterface.connectorPresent = ni->connector_present;
                }
                if (ni->has_end_point_interface) {
                    networkInterface.endPointInterface = ni->end_point_interface;
                }

                out.d->macAddrSpoofing.networkInterfaces << networkInterface;
            }
        }
    }

    if (es->has_dns_policy) out.d->dnsPolicy = (DNS_POLICY_TYPE)es->dns_policy;
    if (es->has_tap_adapter) out.d->tapAdapter = (TAP_ADAPTER_TYPE)es->tap_adapter;
    out.d->customOvpnConfigsPath = es->customovpnconfigspath;
    if (es->has_is_keep_alive_enabled) out.d->isKeepAliveEnabled = es->is_keep_alive_enabled;

    if (es->dns_while_connected_info)
    {
        if (es->dns_while_connected_info->has_type) out.d->connectedDnsInfo.setType((CONNECTED_DNS_TYPE)es->dns_while_connected_info->type);
        out.d->connectedDnsInfo.setIpAddress(es->dns_while_connected_info->ip_address);
    }

    if (es->has_dns_manager) out.d->dnsManager = (DNS_MANAGER_TYPE)es->dns_manager;

    proto_types__engine_settings__free_unpacked(es, NULL);
    return true;
}

bool LegacyProtobufSupport::loadGuiPersistentState(const QByteArray &arr, types::GuiPersistentState &out)
{
    ProtoTypes__GuiPersistentState *gs = proto_types__gui_persistent_state__unpack(NULL, arr.size(), (const uint8_t *)arr.data());
    if (gs)
    {
        if (gs->has_is_firewall_on) out.isFirewallOn = gs->is_firewall_on;
        if (gs->has_window_offs_x) out.windowOffsX = gs->window_offs_x;
        if (gs->has_window_offs_y) out.windowOffsY = gs->window_offs_y;
        if (gs->has_count_visible_locations) out.countVisibleLocations = gs->count_visible_locations;
        if (gs->has_is_first_login) out.isFirstLogin = gs->is_first_login;
        if (gs->has_is_ignore_cpu_usage_warnings) out.isIgnoreCpuUsageWarnings = gs->is_ignore_cpu_usage_warnings;
        if (gs->lastlocation && gs->lastlocation->has_id && gs->lastlocation->has_type) {
            out.lastLocation = LocationID(gs->lastlocation->id, gs->lastlocation->type, gs->lastlocation->city);
        }
        out.lastExternalIp = gs->last_external_ip;

        if (gs->network_white_list)
        {
            out.networkWhiteList.clear();
            for (int i = 0; i < gs->network_white_list->n_networks; i++)
            {
                types::NetworkInterface networkInterface;

                ProtoTypes__NetworkInterface *ni = gs->network_white_list->networks[i];

                if (ni->has_interface_index) {
                    networkInterface.interfaceIndex = ni->interface_index;
                }
                networkInterface.interfaceName = ni->interface_name;
                networkInterface.interfaceGuid = ni->interface_guid;
                networkInterface.networkOrSsid = ni->network_or_ssid;
                if (ni->has_interface_type) {
                    networkInterface.interfaceType = (NETWORK_INTERACE_TYPE)ni->interface_type;
                }
                if (ni->has_trust_type) {
                    networkInterface.trustType = (NETWORK_TRUST_TYPE)ni->trust_type;
                }
                if (ni->has_active) {
                    networkInterface.active = ni->active;
                }
                networkInterface.friendlyName = ni->friendly_name;
                if (ni->has_requested) {
                    networkInterface.requested = ni->requested;
                }
                if (ni->has_metric) {
                    networkInterface.metric = ni->metric;
                }
                networkInterface.physicalAddress = ni->physical_address;
                if (ni->has_mtu) {
                    networkInterface.mtu = ni->mtu;
                }
                if (ni->has_state) {
                    networkInterface.state = ni->state;
                }
                if (ni->has_dw_type) {
                    networkInterface.dwType = ni->dw_type;
                }
                networkInterface.deviceName = ni->device_name;
                if (ni->has_connector_present) {
                    networkInterface.connectorPresent = ni->connector_present;
                }
                if (ni->has_end_point_interface) {
                    networkInterface.endPointInterface = ni->end_point_interface;
                }

                out.networkWhiteList << networkInterface;
            }
        }
        proto_types__gui_persistent_state__free_unpacked(gs, NULL);
        return true;
    }
    return false;
}


